#include <vix/files/Files.hpp>
#include <vix/files/detail/NativePath.hpp>
#include <vix/files/detail/ProcessBridge.hpp>
#include "FilesInternal.hpp"

#include <vix/detail/AllocationFailure.hpp>

#include <cerrno>
#include <climits>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string_view>
#include <utility>

namespace vix::files
{
  namespace
  {
    [[noreturn]] void files_contract_violation() noexcept { std::terminate(); }

    [[nodiscard]] Error file_error(ErrorIdentity identity, std::string_view message)
    {
      return detail::allocation_or_terminate([&]
      {
        return Error{identity, std::string(message)};
      });
    }

    [[nodiscard]] Error native_error(ErrorIdentity fallback, const char *operation, int error_number = errno)
    {
      ErrorIdentity identity = fallback;
      switch (error_number)
      {
      case ENOENT: identity = errors::not_found; break;
      case EEXIST: identity = errors::already_exists; break;
      case EACCES: case EPERM: identity = errors::permission_denied; break;
#ifdef ENOTCAPABLE
      case ENOTCAPABLE: identity = errors::permission_denied; break;
#endif
      case ELOOP: identity = errors::symlink_not_allowed; break;
      case ENOTDIR: identity = errors::not_a_directory; break;
      case EISDIR: identity = errors::not_a_file; break;
      case EMFILE: case ENFILE: identity = errors::resource_exhausted; break;
      case ENOSPC: identity = errors::no_space; break;
      case EROFS: identity = errors::read_only_filesystem; break;
      case ENAMETOOLONG: identity = errors::name_too_long; break;
#ifdef EDQUOT
      case EDQUOT: identity = errors::quota_exhausted; break;
#endif
      case EINTR: identity = errors::interrupted; break;
      case EIO: identity = errors::device_failure; break;
      default: break;
      }
      return detail::allocation_or_terminate([&]
      {
        return Error{identity, std::string(operation) + ": " + std::strerror(error_number)};
      });
    }

    [[nodiscard]] std::string safe_display(std::string_view value)
    {
      return detail::allocation_or_terminate([&]
      {
        std::string result;
        result.reserve(value.size());
        constexpr char hex[] = "0123456789ABCDEF";
        for (const unsigned char c : value)
        {
          if (c >= 0x20 && c < 0x7f) result.push_back(static_cast<char>(c));
          else { result += "\\x"; result.push_back(hex[c >> 4]); result.push_back(hex[c & 0xf]); }
        }
        return result;
      });
    }

    [[nodiscard]] std::string join_native(std::string_view left, std::string_view right)
    {
      return detail::allocation_or_terminate([&]
      {
        if (left.empty()) return std::string(right);
        std::string out(left);
        if (out.back() != '/') out.push_back('/');
        out.append(right);
        return out;
      });
    }

    [[nodiscard]] DirectoryEntryTypeHint entry_hint(unsigned char type) noexcept
    {
      switch (type)
      {
      case DT_REG: return DirectoryEntryTypeHint::regular;
      case DT_DIR: return DirectoryEntryTypeHint::directory;
      case DT_LNK: return DirectoryEntryTypeHint::symlink;
      case DT_UNKNOWN: return DirectoryEntryTypeHint::unknown;
      default: return DirectoryEntryTypeHint::other;
      }
    }

    [[nodiscard]] FileType file_type(mode_t mode) noexcept
    {
      if (S_ISREG(mode)) return FileType::regular;
      if (S_ISDIR(mode)) return FileType::directory;
      return FileType::other;
    }

    [[nodiscard]] std::optional<WallInstant> modified_time(const struct stat &value) noexcept
    {
#if defined(__APPLE__)
      const auto seconds = value.st_mtimespec.tv_sec;
      const auto nanos = value.st_mtimespec.tv_nsec;
#else
      const auto seconds = value.st_mtim.tv_sec;
      const auto nanos = value.st_mtim.tv_nsec;
#endif
      if (seconds > std::numeric_limits<std::int64_t>::max() / 1'000'000'000LL ||
          seconds < std::numeric_limits<std::int64_t>::min() / 1'000'000'000LL) return std::nullopt;
      return WallInstant::from_unix_nanoseconds(static_cast<std::int64_t>(seconds) * 1'000'000'000LL + nanos);
    }

    [[nodiscard]] bool has_read(FileAccess access) noexcept { return access == FileAccess::read || access == FileAccess::read_write; }
    [[nodiscard]] bool has_write(FileAccess access) noexcept { return access == FileAccess::write || access == FileAccess::read_write; }

    [[nodiscard]] Result<void> close_descriptor(int &descriptor, std::optional<Error> &saved)
    {
      if (descriptor < 0)
      {
        if (saved) return *saved;
        return Result<void>::success();
      }
      const int closing = descriptor;
      descriptor = -1;
      if (::close(closing) == 0) return Result<void>::success();
      saved.emplace(native_error(errors::close_failed, "close"));
      return *saved;
    }
  } // namespace

  Path::Path(std::string native) noexcept : native_(std::move(native)) {}
  Path::Path(const Path &other) : native_(detail::allocation_or_terminate([&] { return std::string(other.native_); })) {}
  Path::Path(Path &&) noexcept = default;
  Path &Path::operator=(const Path &other)
  {
    if (this != &other) native_ = detail::allocation_or_terminate([&] { return std::string(other.native_); });
    return *this;
  }
  Path &Path::operator=(Path &&) noexcept = default;
  bool operator==(const Path &left, const Path &right) noexcept { return left.native_ == right.native_; }
  bool operator!=(const Path &left, const Path &right) noexcept { return !(left == right); }
  Result<Path> Path::from_utf8(std::string_view value)
  {
    if (value.find('\0') != std::string_view::npos) return file_error(errors::invalid_path, "path contains NUL");
    if (!detail_valid_utf8(value)) return file_error(errors::path_not_utf8, "path is not valid UTF-8");
    return detail::allocation_or_terminate([&] { return Path{std::string(value)}; });
  }
  Result<std::string> Path::to_utf8() const
  {
    if (!detail_valid_utf8(native_)) return file_error(errors::path_not_utf8, "path is not losslessly UTF-8");
    return detail::allocation_or_terminate([&] { return std::string(native_); });
  }
  std::string Path::display() const { return safe_display(native_); }
  bool Path::empty() const noexcept { return native_.empty(); }
  bool Path::is_absolute() const noexcept { return !native_.empty() && native_.front() == '/'; }
  bool Path::is_relative() const noexcept { return !is_absolute(); }
  Path Path::lexically_normal() const
  {
    return detail::allocation_or_terminate([&]
    {
      std::vector<std::string_view> parts;
      std::size_t start = 0;
      while (start <= native_.size())
      {
        const auto end = native_.find('/', start);
        const auto part = std::string_view{native_}.substr(
            start,
            end == std::string::npos ? native_.size() - start : end - start);
        if (!part.empty() && part != ".")
        {
          if (part == ".." && !parts.empty() && parts.back() != "..") parts.pop_back();
          else if (part != ".." || !is_absolute()) parts.push_back(part);
        }
        if (end == std::string::npos) break;
        start = end + 1;
      }
      std::string result = is_absolute() ? "/" : "";
      for (const auto part : parts)
      {
        if (!result.empty() && result.back() != '/') result.push_back('/');
        result.append(part);
      }
      if (result.empty()) result = is_absolute() ? "/" : ".";
      return Path{std::move(result)};
    });
  }
  Path Path::joined(const Path &relative) const
  {
    if (!relative.is_relative()) files_contract_violation();
    return Path{join_native(native_, relative.native_)};
  }
  std::optional<Path> Path::parent() const
  {
    if (native_.empty()) return std::nullopt;
    const auto end = native_.find_last_not_of('/');
    if (end == std::string::npos) return std::nullopt;
    const auto slash = native_.find_last_of('/', end);
    if (slash == std::string::npos) return std::nullopt;
    return detail::allocation_or_terminate([&] { return std::optional<Path>{Path{native_.substr(0, slash == 0 ? 1 : slash)}}; });
  }
  std::optional<Path> Path::filename() const
  {
    const auto end = native_.find_last_not_of('/');
    if (end == std::string::npos) return std::nullopt;
    const auto slash = native_.find_last_of('/', end);
    return detail::allocation_or_terminate([&] { return std::optional<Path>{Path{native_.substr(slash == std::string::npos ? 0 : slash + 1, end - (slash == std::string::npos ? 0 : slash + 1) + 1)}}; });
  }
  std::optional<Path> Path::extension() const
  {
    const auto base = filename();
    if (!base) return std::nullopt;
    const auto dot = base->native_.find_last_of('.');
    if (dot == std::string::npos || dot == 0) return std::nullopt;
    return detail::allocation_or_terminate([&] { return std::optional<Path>{Path{base->native_.substr(dot)}}; });
  }
  Path detail_path_from_native(std::string value) { return Path{std::move(value)}; }
  const std::string &detail_native_path(const Path &path) noexcept { return path.native_; }

  DirectoryEntry::DirectoryEntry(Path path, DirectoryEntryTypeHint hint) noexcept : path_(std::move(path)), type_hint_(hint) {}

  File::File(int descriptor, FileAccess access, OpenDisposition disposition, bool append) noexcept
      : descriptor_(descriptor), access_(access), disposition_(disposition), append_(append) {}
  File::File(File &&other) noexcept
      : descriptor_(std::exchange(other.descriptor_, -1)),
        access_(other.access_),
        disposition_(other.disposition_),
        append_(other.append_),
        close_error_(std::move(other.close_error_))
  {
  }
  File::~File() { if (descriptor_ >= 0) (void)::close(descriptor_); }
  bool File::is_open() const noexcept { return descriptor_ >= 0; }
  FileAccess File::access() const noexcept { if (!is_open()) files_contract_violation(); return access_; }
  OpenDisposition File::disposition() const noexcept { if (!is_open()) files_contract_violation(); return disposition_; }
  Result<std::size_t> File::read_some(std::span<std::byte> buffer)
  {
    if (!is_open() || !has_read(access_)) files_contract_violation();
    if (buffer.empty()) return std::size_t{0};
    const auto amount = ::read(descriptor_, buffer.data(), buffer.size());
    if (amount < 0) return native_error(errors::read_failed, "read");
    return static_cast<std::size_t>(amount);
  }
  Result<std::size_t> File::write_some(std::span<const std::byte> buffer)
  {
    if (!is_open() || !has_write(access_)) files_contract_violation();
    if (buffer.empty()) return std::size_t{0};
    const auto amount = ::write(descriptor_, buffer.data(), buffer.size());
    if (amount <= 0) return native_error(errors::write_failed, "write", amount == 0 ? EIO : errno);
    return static_cast<std::size_t>(amount);
  }
  Result<std::uint64_t> File::seek(std::int64_t offset, SeekOrigin origin)
  {
    if (!is_open()) files_contract_violation();
    int native_origin = origin == SeekOrigin::begin ? SEEK_SET : origin == SeekOrigin::current ? SEEK_CUR : SEEK_END;
    const auto position = ::lseek(descriptor_, static_cast<off_t>(offset), native_origin);
    if (position < 0) return errno == EINVAL ? file_error(errors::invalid_offset, "invalid seek offset") : native_error(errors::io_failure, "seek");
    return static_cast<std::uint64_t>(position);
  }
  Result<std::uint64_t> File::tell() const
  {
    if (!is_open()) files_contract_violation();
    const auto position = ::lseek(descriptor_, 0, SEEK_CUR);
    if (position < 0) return native_error(errors::io_failure, "tell");
    return static_cast<std::uint64_t>(position);
  }
  Result<void> File::flush()
  {
    if (!is_open()) files_contract_violation();
    if (!has_write(access_)) return Result<void>::success();
    if (::fsync(descriptor_) != 0) return native_error(errors::flush_failed, "fsync");
    return Result<void>::success();
  }
  Result<void> File::close() { return close_descriptor(descriptor_, close_error_); }

  Result<File> open(const Path &path, OpenOptions options)
  {
    int flags = 0;
    flags |= options.access() == FileAccess::read ? O_RDONLY : options.access() == FileAccess::write ? O_WRONLY : O_RDWR;
    bool writable = has_write(options.access());
    if (writable) flags |= O_NOFOLLOW;
    bool append = false;
    switch (options.disposition())
    {
    case OpenDisposition::existing: break;
    case OpenDisposition::open_or_create: flags |= O_CREAT; break;
    case OpenDisposition::create_new: flags |= O_CREAT | O_EXCL; break;
    case OpenDisposition::truncate_existing: flags |= O_TRUNC; break;
    case OpenDisposition::create_or_truncate: flags |= O_CREAT | O_TRUNC; break;
    case OpenDisposition::append_existing: flags |= O_APPEND; append = true; break;
    case OpenDisposition::append_or_create: flags |= O_APPEND | O_CREAT; append = true; break;
    }
    const int descriptor = ::open(detail_native_path(path).c_str(), flags, 0666);
    if (descriptor < 0) return native_error(errors::io_failure, "open");
    struct stat status{};
    if (::fstat(descriptor, &status) != 0) { const auto error = native_error(errors::io_failure, "fstat"); ::close(descriptor); return error; }
    if (!S_ISREG(status.st_mode)) { ::close(descriptor); return file_error(errors::not_a_file, "open requires a regular file"); }
    return File{descriptor, options.access(), options.disposition(), append};
  }

  Result<FileInfo> info(const Path &path)
  {
    struct stat status{};
    if (::stat(detail_native_path(path).c_str(), &status) != 0) return native_error(errors::io_failure, "stat");
    std::optional<std::uint64_t> size;
    if (S_ISREG(status.st_mode) && status.st_size >= 0) size = static_cast<std::uint64_t>(status.st_size);
    return FileInfo{file_type(status.st_mode), size, modified_time(status)};
  }

  Result<bool> exists(const Path &path)
  {
    struct stat status{};
    if (::stat(detail_native_path(path).c_str(), &status) == 0) return true;
    if (errno == ENOENT || errno == ENOTDIR) return false;
    return native_error(errors::io_failure, "stat");
  }

  Result<void> create_directory(const Path &path)
  {
    if (::mkdir(detail_native_path(path).c_str(), 0777) == 0) return Result<void>::success();
    return native_error(errors::create_failed, "mkdir");
  }

  Result<void> create_directories(const Path &path)
  {
    if (path.empty()) return file_error(errors::invalid_path, "empty path");
    std::string current = path.is_absolute() ? "/" : "";
    std::size_t start = 0;
    const auto &native = detail_native_path(path);
    while (start <= native.size())
    {
      const auto end = native.find('/', start);
      const auto part = native.substr(start, end == std::string::npos ? native.size() - start : end - start);
      if (!part.empty())
      {
        current = join_native(current, part);
        if (::mkdir(current.c_str(), 0777) != 0 && errno != EEXIST) return native_error(errors::create_failed, "mkdir");
        struct stat status{};
        if (::stat(current.c_str(), &status) != 0) return native_error(errors::create_failed, "stat");
        if (!S_ISDIR(status.st_mode)) return file_error(errors::not_a_directory, "existing path component is not a directory");
      }
      if (end == std::string::npos) break;
      start = end + 1;
    }
    return Result<void>::success();
  }

  struct DirectoryIterator::State
  {
    DIR *directory = nullptr;
    Path base;
    std::optional<Error> close_error;
    State(DIR *value, Path path) noexcept : directory(value), base(std::move(path)) {}
  };
  DirectoryIterator::DirectoryIterator(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
  DirectoryIterator::DirectoryIterator(DirectoryIterator &&) noexcept = default;
  DirectoryIterator::~DirectoryIterator() { if (state_ && state_->directory) (void)::closedir(state_->directory); }
  bool DirectoryIterator::is_open() const noexcept { return state_ && state_->directory; }
  Result<void> DirectoryIterator::close()
  {
    if (!state_) { files_contract_violation(); }
    if (!state_->directory)
    {
      if (state_->close_error) return *state_->close_error;
      return Result<void>::success();
    }
    DIR *directory = state_->directory;
    state_->directory = nullptr;
    if (::closedir(directory) == 0) return Result<void>::success();
    state_->close_error.emplace(native_error(errors::close_failed, "closedir"));
    return *state_->close_error;
  }
  Result<std::optional<DirectoryEntry>> DirectoryIterator::next()
  {
    if (!is_open()) files_contract_violation();
    errno = 0;
    while (auto *entry = ::readdir(state_->directory))
    {
      std::string_view name(entry->d_name);
      if (name == "." || name == "..") continue;
      return detail::allocation_or_terminate([&]() -> Result<std::optional<DirectoryEntry>>
      {
        return std::optional<DirectoryEntry>{DirectoryEntry{Path{join_native(detail_native_path(state_->base), name)}, entry_hint(entry->d_type)}};
      });
    }
    if (errno != 0) return native_error(errors::io_failure, "readdir");
    return std::optional<DirectoryEntry>{};
  }
  Result<DirectoryIterator> iterate(const Path &path)
  {
    struct stat status{};
    if (::lstat(detail_native_path(path).c_str(), &status) != 0) return native_error(errors::io_failure, "lstat");
    if (S_ISLNK(status.st_mode)) return file_error(errors::symlink_not_allowed, "iteration rejects final symlink");
    if (!S_ISDIR(status.st_mode)) return file_error(errors::not_a_directory, "iteration requires a directory");
    DIR *directory = ::opendir(detail_native_path(path).c_str());
    if (!directory) return native_error(errors::io_failure, "opendir");
    return detail::allocation_or_terminate([&]() -> Result<DirectoryIterator>
    {
      return DirectoryIterator{std::make_unique<DirectoryIterator::State>(directory, path)};
    });
  }

  Result<void> remove_file(const Path &path)
  {
    struct stat status{};
    if (::lstat(detail_native_path(path).c_str(), &status) != 0) return native_error(errors::remove_failed, "lstat");
    if (S_ISDIR(status.st_mode)) return file_error(errors::not_a_file, "remove_file rejects directories");
    if (::unlink(detail_native_path(path).c_str()) == 0) return Result<void>::success();
    return native_error(errors::remove_failed, "unlink");
  }
  Result<void> remove_directory(const Path &path)
  {
    if (::rmdir(detail_native_path(path).c_str()) == 0) return Result<void>::success();
    return native_error(errors::remove_failed, "rmdir");
  }

  namespace
  {
    Result<void> remove_tree_impl(const std::string &path)
    {
      struct stat status{};
      if (::lstat(path.c_str(), &status) != 0) return native_error(errors::remove_failed, "lstat");
      if (!S_ISDIR(status.st_mode) || S_ISLNK(status.st_mode))
      {
        if (::unlink(path.c_str()) == 0) return Result<void>::success();
        return native_error(errors::remove_failed, "unlink");
      }
      DIR *directory = ::opendir(path.c_str());
      if (!directory) return native_error(errors::remove_failed, "opendir");
      while (auto *entry = ::readdir(directory))
      {
        std::string_view name(entry->d_name);
        if (name == "." || name == "..") continue;
        auto removed = remove_tree_impl(join_native(path, name));
        if (!removed) { ::closedir(directory); return std::move(removed).error(); }
      }
      if (errno != 0) { const auto error = native_error(errors::remove_failed, "readdir"); ::closedir(directory); return error; }
      if (::closedir(directory) != 0) return native_error(errors::remove_failed, "closedir");
      if (::rmdir(path.c_str()) == 0) return Result<void>::success();
      return native_error(errors::remove_failed, "rmdir");
    }
  }
  Result<void> remove_tree(const Path &path, RemoveTreeOptions options)
  {
    struct stat status{};
    if (::lstat(detail_native_path(path).c_str(), &status) != 0)
    {
      if (errno == ENOENT && options.missing_path_policy() == MissingPathPolicy::ignore) return Result<void>::success();
      return native_error(errors::remove_failed, "lstat");
    }
    return remove_tree_impl(detail_native_path(path));
  }

  Result<void> copy_file(const Path &source, const Path &destination, CopyOptions options)
  {
    struct stat source_status{};
    if (::stat(detail_native_path(source).c_str(), &source_status) != 0) return native_error(errors::copy_failed, "stat source");
    if (!S_ISREG(source_status.st_mode)) return file_error(errors::not_a_file, "copy source must resolve to a regular file");
    struct stat destination_status{};
    const int destination_lstat = ::lstat(detail_native_path(destination).c_str(), &destination_status);
    const int destination_lstat_errno = errno;
    if (destination_lstat == 0 && S_ISLNK(destination_status.st_mode))
      return file_error(errors::symlink_not_allowed, "copy destination final symlink is rejected");
    if (destination_lstat != 0 && destination_lstat_errno != ENOENT)
      return native_error(errors::copy_failed, "lstat destination", destination_lstat_errno);

    bool destination_created = false;
    if (options.destination_policy() == DestinationPolicy::create_new)
      destination_created = true;
    else if (options.destination_policy() == DestinationPolicy::create_or_replace && destination_lstat_errno == ENOENT)
      destination_created = true;

    auto input = open(source, OpenOptions::existing(FileAccess::read));
    if (!input) return std::move(input).error();
    OpenOptions output_options = options.destination_policy() == DestinationPolicy::create_new
        ? OpenOptions::create_new(FileAccess::write)
        : options.destination_policy() == DestinationPolicy::replace_existing
            ? OpenOptions::truncate_existing()
            : OpenOptions::create_or_truncate();
    auto output = open(destination, output_options);
    if (!output) return std::move(output).error();
    File reader = input.take();
    File writer = output.take();
    auto buffer = detail::allocation_or_terminate([] { return std::vector<std::byte>(64 * 1024); });
    Error transfer_error{errors::copy_failed};
    bool failed = false;
    while (!failed)
    {
      auto read = reader.read_some(buffer);
      if (!read) { transfer_error = std::move(read).error(); failed = true; break; }
      if (read.value() == 0) break;
      std::size_t position = 0;
      while (position < read.value())
      {
        auto write = writer.write_some(std::span<const std::byte>{buffer}.first(read.value()).subspan(position));
        if (!write) { transfer_error = std::move(write).error(); failed = true; break; }
        position += write.value();
      }
    }
    auto reader_closed = reader.close();
    auto writer_closed = writer.close();
    if (!failed && !reader_closed) { transfer_error = std::move(reader_closed).error(); failed = true; }
    if (!failed && !writer_closed) { transfer_error = std::move(writer_closed).error(); failed = true; }
    if (!failed) return Result<void>::success();
    if (destination_created && ::unlink(detail_native_path(destination).c_str()) != 0)
      return detail::allocation_or_terminate([&]
      {
        return Error{errors::cleanup_failed, std::string("copy destination cleanup failed"), std::move(transfer_error)};
      });
    return transfer_error;
  }

  Result<void> rename_replace(const Path &source, const Path &destination, RenameOptions)
  {
    if (::rename(detail_native_path(source).c_str(), detail_native_path(destination).c_str()) == 0) return Result<void>::success();
    if (errno == EXDEV) return file_error(errors::not_same_filesystem, "rename requires one filesystem");
    return native_error(errors::rename_failed, "rename");
  }

  int detail_duplicate_native_handle(const File &file) noexcept
  {
    if (!file.is_open())
    {
      files_contract_violation();
    }
    return ::fcntl(file.descriptor_, F_DUPFD_CLOEXEC, 0);
  }

  std::intptr_t detail_duplicate_process_handle(const File &file) noexcept
  {
    return static_cast<std::intptr_t>(detail_duplicate_native_handle(file));
  }

  bool detail_set_process_handle_inheritable(std::intptr_t handle, bool inheritable) noexcept
  {
    if (handle < 0) return false;
    const int descriptor = static_cast<int>(handle);
    const int flags = ::fcntl(descriptor, F_GETFD);
    if (flags < 0) return false;
    const int updated = inheritable ? (flags & ~FD_CLOEXEC) : (flags | FD_CLOEXEC);
    return ::fcntl(descriptor, F_SETFD, updated) == 0;
  }

  bool detail_close_process_handle(std::intptr_t handle) noexcept
  {
    if (handle < 0) return false;
    return ::close(static_cast<int>(handle)) == 0;
  }
} // namespace vix::files
