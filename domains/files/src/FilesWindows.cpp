#include <vix/files/Files.hpp>
#include <vix/files/detail/NativePath.hpp>
#include <vix/files/detail/ProcessBridge.hpp>
#include "FilesInternal.hpp"

#include <vix/detail/AllocationFailure.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <fcntl.h>
#include <io.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstring>
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
      return detail::allocation_or_terminate([&] { return Error{identity, std::string(message)}; });
    }

    [[nodiscard]] Error errno_error(ErrorIdentity fallback, const char *operation, int value = errno)
    {
      ErrorIdentity identity = fallback;
      switch (value)
      {
      case ENOENT: identity = errors::not_found; break;
      case EEXIST: identity = errors::already_exists; break;
      case EACCES: case EPERM: identity = errors::permission_denied; break;
      case EMFILE: case ENFILE: identity = errors::resource_exhausted; break;
      case ENOSPC: identity = errors::no_space; break;
      case EINVAL: identity = errors::invalid_path; break;
      default: break;
      }
      char description[256]{};
      const auto description_status = ::strerror_s(description, sizeof(description), value);
      const std::string message = description_status == 0 ? description : "unknown C runtime error";
      return detail::allocation_or_terminate([&]
      {
        return Error{identity, std::string(operation) + ": " + message};
      });
    }

    [[nodiscard]] Error filesystem_error(ErrorIdentity fallback, const char *operation,
                                         const std::error_code &ec)
    {
      ErrorIdentity identity = fallback;
      if (ec == std::errc::no_such_file_or_directory) identity = errors::not_found;
      else if (ec == std::errc::file_exists) identity = errors::already_exists;
      else if (ec == std::errc::permission_denied) identity = errors::permission_denied;
      else if (ec == std::errc::not_a_directory) identity = errors::not_a_directory;
      else if (ec == std::errc::is_a_directory) identity = errors::not_a_file;
      else if (ec == std::errc::no_space_on_device) identity = errors::no_space;
      else if (ec == std::errc::filename_too_long) identity = errors::name_too_long;
      else if (ec == std::errc::cross_device_link) identity = errors::not_same_filesystem;
      return detail::allocation_or_terminate([&]
      {
        return Error{identity, std::string(operation) + ": " + ec.message()};
      });
    }

    [[nodiscard]] Error win32_error(ErrorIdentity fallback, const char *operation, DWORD code = ::GetLastError())
    {
      ErrorIdentity identity = fallback;
      switch (code)
      {
      case ERROR_FILE_NOT_FOUND: case ERROR_PATH_NOT_FOUND: identity = errors::not_found; break;
      case ERROR_FILE_EXISTS: case ERROR_ALREADY_EXISTS: identity = errors::already_exists; break;
      case ERROR_ACCESS_DENIED: case ERROR_PRIVILEGE_NOT_HELD: identity = errors::permission_denied; break;
      case ERROR_TOO_MANY_OPEN_FILES: identity = errors::resource_exhausted; break;
      case ERROR_DISK_FULL: identity = errors::no_space; break;
      case ERROR_NOT_SAME_DEVICE: identity = errors::not_same_filesystem; break;
      case ERROR_FILENAME_EXCED_RANGE: identity = errors::name_too_long; break;
      default: break;
      }
      return detail::allocation_or_terminate([&]
      {
        return Error{identity, std::string(operation) + ": Windows error " + std::to_string(code)};
      });
    }

    [[nodiscard]] std::string safe_display(std::string_view value)
    {
      return detail::allocation_or_terminate([&]
      {
        std::string result;
        constexpr char hex[] = "0123456789ABCDEF";
        for (const unsigned char c : value)
        {
          if (c >= 0x20 && c < 0x7f) result.push_back(static_cast<char>(c));
          else { result += "\\x"; result.push_back(hex[c >> 4]); result.push_back(hex[c & 0xf]); }
        }
        return result;
      });
    }

    [[nodiscard]] std::optional<std::wstring> utf8_to_wide(std::string_view path)
    {
      if (path.empty()) return std::wstring{};
      if (path.size() > static_cast<std::size_t>(INT_MAX)) return std::nullopt;
      const int input_size = static_cast<int>(path.size());
      const int wide_size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                                   path.data(), input_size, nullptr, 0);
      if (wide_size <= 0) return std::nullopt;
      auto wide = detail::allocation_or_terminate([&] { return std::wstring(static_cast<std::size_t>(wide_size), L'\0'); });
      if (::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), input_size,
                                wide.data(), wide_size) != wide_size) return std::nullopt;
      return wide;
    }

    [[nodiscard]] std::filesystem::path native_path(std::string_view path)
    {
      auto wide = utf8_to_wide(path);
      if (!wide) files_contract_violation();
      return std::filesystem::path(std::move(*wide));
    }

    [[nodiscard]] std::filesystem::path native_path(const Path &path)
    {
      return native_path(detail_native_path(path));
    }

    [[nodiscard]] bool is_reparse_point(const std::filesystem::path &path) noexcept
    {
      const DWORD attributes = ::GetFileAttributesW(path.c_str());
      return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    }

    [[nodiscard]] bool has_read(FileAccess access) noexcept
    { return access == FileAccess::read || access == FileAccess::read_write; }
    [[nodiscard]] bool has_write(FileAccess access) noexcept
    { return access == FileAccess::write || access == FileAccess::read_write; }

    [[nodiscard]] FileType file_type(std::filesystem::file_type type) noexcept
    {
      if (type == std::filesystem::file_type::regular) return FileType::regular;
      if (type == std::filesystem::file_type::directory) return FileType::directory;
      return FileType::other;
    }

    [[nodiscard]] std::optional<WallInstant> modified_time(const std::filesystem::file_time_type &time) noexcept
    {
      const auto system_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
          time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
      const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(system_time.time_since_epoch()).count();
      return WallInstant::from_unix_nanoseconds(static_cast<std::int64_t>(nanos));
    }

    [[nodiscard]] Result<void> close_descriptor(int &descriptor, std::optional<Error> &saved)
    {
      if (descriptor < 0)
      {
        if (saved) return *saved;
        return Result<void>::success();
      }
      const int closing = std::exchange(descriptor, -1);
      if (::_close(closing) == 0) return Result<void>::success();
      saved.emplace(errno_error(errors::close_failed, "close"));
      return *saved;
    }

    [[nodiscard]] std::optional<std::string> path_utf8(const std::filesystem::path &path)
    {
      const auto &wide = path.native();
      if (wide.empty()) return std::string{};
      if (wide.size() > static_cast<std::size_t>(INT_MAX)) return std::nullopt;
      const int input_size = static_cast<int>(wide.size());
      const int utf8_size = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), input_size,
                                                   nullptr, 0, nullptr, nullptr);
      if (utf8_size <= 0) return std::nullopt;
      auto encoded = detail::allocation_or_terminate([&] { return std::string(static_cast<std::size_t>(utf8_size), '\0'); });
      if (::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), input_size,
                                encoded.data(), utf8_size, nullptr, nullptr) != utf8_size) return std::nullopt;
      return encoded;
    }

    [[nodiscard]] std::string path_utf8_or_empty(const std::filesystem::path &path)
    {
      auto encoded = path_utf8(path);
      if (!encoded) files_contract_violation();
      return std::move(*encoded);
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
    if (!utf8_to_wide(value)) return file_error(errors::invalid_path, "UTF-8 path cannot be converted to a native Windows path");
    return detail::allocation_or_terminate([&] { return Path{std::string(value)}; });
  }
  Result<std::string> Path::to_utf8() const
  {
    if (!detail_valid_utf8(native_)) return file_error(errors::path_not_utf8, "path is not losslessly UTF-8");
    return detail::allocation_or_terminate([&] { return std::string(native_); });
  }
  std::string Path::display() const { return safe_display(native_); }
  bool Path::empty() const noexcept { return native_.empty(); }
  bool Path::is_absolute() const noexcept
  {
    if (native_.empty()) return false;
    const auto separator = [](char value) { return value == '/' || value == '\\'; };
    if (separator(native_[0])) return true;
    return native_.size() >= 3 &&
        ((native_[0] >= 'A' && native_[0] <= 'Z') || (native_[0] >= 'a' && native_[0] <= 'z')) &&
        native_[1] == ':' && separator(native_[2]);
  }
  bool Path::is_relative() const noexcept { return !is_absolute(); }
  Path Path::lexically_normal() const
  {
    return detail::allocation_or_terminate([&]
    {
      return Path{path_utf8_or_empty(native_path(*this).lexically_normal())};
    });
  }
  Path Path::joined(const Path &relative) const
  {
    if (!relative.is_relative()) files_contract_violation();
    return detail::allocation_or_terminate([&]
    {
      return Path{path_utf8_or_empty(native_path(*this) / native_path(relative.native_))};
    });
  }
  std::optional<Path> Path::parent() const
  {
    const auto parent_path = native_path(*this).parent_path();
    if (parent_path.empty()) return std::nullopt;
    return detail::allocation_or_terminate([&] { return std::optional<Path>{Path{path_utf8_or_empty(parent_path)}}; });
  }
  std::optional<Path> Path::filename() const
  {
    const auto filename_path = native_path(*this).filename();
    if (filename_path.empty()) return std::nullopt;
    return detail::allocation_or_terminate([&] { return std::optional<Path>{Path{path_utf8_or_empty(filename_path)}}; });
  }
  std::optional<Path> Path::extension() const
  {
    const auto filename_value = filename();
    if (!filename_value) return std::nullopt;
    const auto extension_path = native_path(filename_value->native_).extension();
    if (extension_path.empty()) return std::nullopt;
    return detail::allocation_or_terminate([&] { return std::optional<Path>{Path{path_utf8_or_empty(extension_path)}}; });
  }
  Path detail_path_from_native(std::string value) { return Path{std::move(value)}; }
  const std::string &detail_native_path(const Path &path) noexcept { return path.native_; }

  DirectoryEntry::DirectoryEntry(Path path, DirectoryEntryTypeHint hint) noexcept
      : path_(std::move(path)), type_hint_(hint) {}

  File::File(int descriptor, FileAccess access, OpenDisposition disposition, bool append) noexcept
      : descriptor_(descriptor), access_(access), disposition_(disposition), append_(append) {}
  File::File(File &&other) noexcept
      : descriptor_(std::exchange(other.descriptor_, -1)), access_(other.access_),
        disposition_(other.disposition_), append_(other.append_), close_error_(std::move(other.close_error_)) {}
  File::~File() { if (descriptor_ >= 0) (void)::_close(descriptor_); }
  bool File::is_open() const noexcept { return descriptor_ >= 0; }
  FileAccess File::access() const noexcept { if (!is_open()) files_contract_violation(); return access_; }
  OpenDisposition File::disposition() const noexcept { if (!is_open()) files_contract_violation(); return disposition_; }
  Result<std::size_t> File::read_some(std::span<std::byte> buffer)
  {
    if (!is_open() || !has_read(access_)) files_contract_violation();
    if (buffer.empty()) return std::size_t{0};
    const auto amount = ::_read(descriptor_, buffer.data(), static_cast<unsigned int>(std::min<std::size_t>(buffer.size(), UINT_MAX)));
    if (amount < 0) return errno_error(errors::read_failed, "read");
    return static_cast<std::size_t>(amount);
  }
  Result<std::size_t> File::write_some(std::span<const std::byte> buffer)
  {
    if (!is_open() || !has_write(access_)) files_contract_violation();
    if (buffer.empty()) return std::size_t{0};
    const auto amount = ::_write(descriptor_, buffer.data(), static_cast<unsigned int>(std::min<std::size_t>(buffer.size(), UINT_MAX)));
    if (amount <= 0) return errno_error(errors::write_failed, "write", amount == 0 ? EIO : errno);
    return static_cast<std::size_t>(amount);
  }
  Result<std::uint64_t> File::seek(std::int64_t offset, SeekOrigin origin)
  {
    if (!is_open()) files_contract_violation();
    const int native_origin = origin == SeekOrigin::begin ? SEEK_SET : origin == SeekOrigin::current ? SEEK_CUR : SEEK_END;
    const auto position = ::_lseeki64(descriptor_, offset, native_origin);
    if (position < 0) return errno == EINVAL ? file_error(errors::invalid_offset, "invalid seek offset") : errno_error(errors::io_failure, "seek");
    return static_cast<std::uint64_t>(position);
  }
  Result<std::uint64_t> File::tell() const
  {
    if (!is_open()) files_contract_violation();
    const auto position = ::_lseeki64(descriptor_, 0, SEEK_CUR);
    if (position < 0) return errno_error(errors::io_failure, "tell");
    return static_cast<std::uint64_t>(position);
  }
  Result<void> File::flush()
  {
    if (!is_open()) files_contract_violation();
    if (!has_write(access_)) return Result<void>::success();
    if (::_commit(descriptor_) != 0) return errno_error(errors::flush_failed, "flush");
    return Result<void>::success();
  }
  Result<void> File::close() { return close_descriptor(descriptor_, close_error_); }

  Result<File> open(const Path &path, OpenOptions options)
  {
    const auto native = native_path(path);
    DWORD desired_access = options.access() == FileAccess::read ? GENERIC_READ
        : options.access() == FileAccess::write ? GENERIC_WRITE : (GENERIC_READ | GENERIC_WRITE);
    DWORD creation = OPEN_EXISTING;
    bool append = false;
    switch (options.disposition())
    {
    case OpenDisposition::existing: creation = OPEN_EXISTING; break;
    case OpenDisposition::open_or_create: creation = OPEN_ALWAYS; break;
    case OpenDisposition::create_new: creation = CREATE_NEW; break;
    case OpenDisposition::truncate_existing: creation = TRUNCATE_EXISTING; break;
    case OpenDisposition::create_or_truncate: creation = CREATE_ALWAYS; break;
    case OpenDisposition::append_existing: creation = OPEN_EXISTING; append = true; break;
    case OpenDisposition::append_or_create: creation = OPEN_ALWAYS; append = true; break;
    }
    constexpr DWORD sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const DWORD attributes = has_write(options.access()) ? FILE_FLAG_OPEN_REPARSE_POINT : FILE_ATTRIBUTE_NORMAL;
    HANDLE handle = ::CreateFileW(native.c_str(), desired_access, sharing, nullptr, creation, attributes, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return win32_error(errors::io_failure, "CreateFileW");

    BY_HANDLE_FILE_INFORMATION native_info{};
    if (!::GetFileInformationByHandle(handle, &native_info))
    {
      const auto error = win32_error(errors::io_failure, "GetFileInformationByHandle");
      ::CloseHandle(handle);
      return error;
    }
    if (has_write(options.access()) && (native_info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
    {
      ::CloseHandle(handle);
      return file_error(errors::symlink_not_allowed, "write open rejects final symlink/reparse point");
    }
    if (::GetFileType(handle) != FILE_TYPE_DISK || (native_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
      ::CloseHandle(handle);
      return file_error(errors::not_a_file, "open requires a regular file");
    }

    int flags = _O_BINARY | _O_NOINHERIT;
    flags |= options.access() == FileAccess::read ? _O_RDONLY : options.access() == FileAccess::write ? _O_WRONLY : _O_RDWR;
    if (append) flags |= _O_APPEND;
    const int descriptor = ::_open_osfhandle(reinterpret_cast<std::intptr_t>(handle), flags);
    if (descriptor < 0)
    {
      const auto error = errno_error(errors::io_failure, "_open_osfhandle");
      ::CloseHandle(handle);
      return error;
    }
    return File{descriptor, options.access(), options.disposition(), append};
  }

  Result<FileInfo> info(const Path &path)
  {
    std::error_code ec;
    const auto status = std::filesystem::status(native_path(path), ec);
    if (ec) return filesystem_error(errors::io_failure, "status", ec);
    std::optional<std::uint64_t> size;
    if (std::filesystem::is_regular_file(status))
    {
      const auto file_size = std::filesystem::file_size(native_path(path), ec);
      if (ec) return filesystem_error(errors::io_failure, "file_size", ec);
      size = file_size;
    }
    const auto modified = std::filesystem::last_write_time(native_path(path), ec);
    return FileInfo{file_type(status.type()), size, ec ? std::nullopt : modified_time(modified)};
  }
  Result<bool> exists(const Path &path)
  {
    std::error_code ec;
    const bool value = std::filesystem::exists(native_path(path), ec);
    if (ec) return filesystem_error(errors::io_failure, "exists", ec);
    return value;
  }
  Result<void> create_directory(const Path &path)
  {
    std::error_code ec;
    if (std::filesystem::create_directory(native_path(path), ec)) return Result<void>::success();
    if (ec) return filesystem_error(errors::create_failed, "create_directory", ec);
    return file_error(errors::already_exists, "directory already exists");
  }
  Result<void> create_directories(const Path &path)
  {
    if (path.empty()) return file_error(errors::invalid_path, "empty path");
    std::error_code ec;
    std::filesystem::create_directories(native_path(path), ec);
    if (ec) return filesystem_error(errors::create_failed, "create_directories", ec);
    if (!std::filesystem::is_directory(native_path(path), ec) || ec)
      return ec ? filesystem_error(errors::create_failed, "is_directory", ec)
                : file_error(errors::not_a_directory, "existing path is not a directory");
    return Result<void>::success();
  }

  struct DirectoryIterator::State
  {
    std::filesystem::directory_iterator iterator;
    std::filesystem::directory_iterator end;
    Path base;
    bool open = true;
    std::optional<Error> close_error;
    State(std::filesystem::directory_iterator value, Path path) noexcept
        : iterator(std::move(value)), base(std::move(path)) {}
  };
  DirectoryIterator::DirectoryIterator(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
  DirectoryIterator::DirectoryIterator(DirectoryIterator &&) noexcept = default;
  DirectoryIterator::~DirectoryIterator() = default;
  bool DirectoryIterator::is_open() const noexcept { return state_ && state_->open; }
  Result<void> DirectoryIterator::close()
  {
    if (!state_) files_contract_violation();
    state_->open = false;
    state_->iterator = {};
    if (state_->close_error) return *state_->close_error;
    return Result<void>::success();
  }
  Result<std::optional<DirectoryEntry>> DirectoryIterator::next()
  {
    if (!is_open()) files_contract_violation();
    if (state_->iterator == state_->end) return std::optional<DirectoryEntry>{};
    return detail::allocation_or_terminate([&]() -> Result<std::optional<DirectoryEntry>>
    {
      const auto entry_path = state_->iterator->path();
      std::error_code ec;
      const auto status = state_->iterator->symlink_status(ec);
      if (ec) return filesystem_error(errors::io_failure, "directory entry status", ec);
      DirectoryEntryTypeHint hint = DirectoryEntryTypeHint::other;
      if (std::filesystem::is_symlink(status) || is_reparse_point(entry_path)) hint = DirectoryEntryTypeHint::symlink;
      else if (std::filesystem::is_directory(status)) hint = DirectoryEntryTypeHint::directory;
      else if (std::filesystem::is_regular_file(status)) hint = DirectoryEntryTypeHint::regular;
      auto entry_utf8 = path_utf8(entry_path);
      if (!entry_utf8) return file_error(errors::path_not_utf8, "native directory entry is not valid Unicode");
      auto converted = Path::from_utf8(*entry_utf8);
      if (!converted) return std::move(converted).error();
      state_->iterator.increment(ec);
      if (ec) return filesystem_error(errors::io_failure, "advance directory iterator", ec);
      return std::optional<DirectoryEntry>{DirectoryEntry{converted.take(), hint}};
    });
  }
  Result<DirectoryIterator> iterate(const Path &path)
  {
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(native_path(path), ec);
    if (ec) return filesystem_error(errors::io_failure, "inspect directory", ec);
    if (std::filesystem::is_symlink(status) || is_reparse_point(native_path(path)))
      return file_error(errors::symlink_not_allowed, "iteration rejects final symlink/reparse point");
    if (!std::filesystem::is_directory(status)) return file_error(errors::not_a_directory, "iteration requires a directory");
    std::filesystem::directory_iterator iterator(native_path(path), ec);
    if (ec) return filesystem_error(errors::io_failure, "iterate", ec);
    return detail::allocation_or_terminate([&]() -> Result<DirectoryIterator>
    {
      return DirectoryIterator{std::make_unique<DirectoryIterator::State>(std::move(iterator), path)};
    });
  }

  Result<void> remove_file(const Path &path)
  {
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(native_path(path), ec);
    if (ec) return filesystem_error(errors::remove_failed, "inspect file", ec);
    if (std::filesystem::is_directory(status)) return file_error(errors::not_a_file, "remove_file rejects directories");
    if (!std::filesystem::remove(native_path(path), ec) || ec)
      return ec ? filesystem_error(errors::remove_failed, "remove", ec) : file_error(errors::not_found, "file does not exist");
    return Result<void>::success();
  }
  Result<void> remove_directory(const Path &path)
  {
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(native_path(path), ec);
    if (ec) return filesystem_error(errors::remove_failed, "inspect directory", ec);
    if (std::filesystem::is_symlink(status) || is_reparse_point(native_path(path)))
      return file_error(errors::not_a_directory, "remove_directory rejects a symlink/reparse point");
    if (!std::filesystem::is_directory(status))
      return file_error(errors::not_a_directory, "remove_directory requires a directory");
    if (!std::filesystem::remove(native_path(path), ec) || ec)
      return ec ? filesystem_error(errors::remove_failed, "remove directory", ec) : file_error(errors::not_found, "directory does not exist");
    return Result<void>::success();
  }
  Result<void> remove_tree(const Path &path, RemoveTreeOptions options)
  {
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(native_path(path), ec);
    if (ec == std::errc::no_such_file_or_directory && options.missing_path_policy() == MissingPathPolicy::ignore)
      return Result<void>::success();
    if (ec) return filesystem_error(errors::remove_failed, "inspect tree", ec);
    if (std::filesystem::is_symlink(status) || is_reparse_point(native_path(path)))
    {
      if (!std::filesystem::remove(native_path(path), ec) || ec)
        return ec ? filesystem_error(errors::remove_failed, "remove reparse point", ec)
                  : file_error(errors::remove_failed, "reparse point was not removed");
      return Result<void>::success();
    }
    std::filesystem::remove_all(native_path(path), ec);
    if (ec) return filesystem_error(errors::remove_failed, "remove_tree", ec);
    return Result<void>::success();
  }
  Result<void> copy_file(const Path &source, const Path &destination, CopyOptions options)
  {
    std::error_code ec;
    const auto source_status = std::filesystem::status(native_path(source), ec);
    if (ec) return filesystem_error(errors::copy_failed, "source status", ec);
    if (!std::filesystem::is_regular_file(source_status)) return file_error(errors::not_a_file, "copy source must be a regular file");
    const auto destination_status = std::filesystem::symlink_status(native_path(destination), ec);
    const bool destination_exists = !ec && destination_status.type() != std::filesystem::file_type::not_found;
    if (ec && ec != std::errc::no_such_file_or_directory) return filesystem_error(errors::copy_failed, "destination status", ec);
    if (destination_exists && (std::filesystem::is_symlink(destination_status) || is_reparse_point(native_path(destination))))
      return file_error(errors::symlink_not_allowed, "copy destination final symlink/reparse point is rejected");

    auto input_result = open(source, OpenOptions::existing(FileAccess::read));
    if (!input_result) return std::move(input_result).error();
    const bool destination_created = options.destination_policy() == DestinationPolicy::create_new ||
        (options.destination_policy() == DestinationPolicy::create_or_replace && !destination_exists);
    const auto output_options = options.destination_policy() == DestinationPolicy::create_new
        ? OpenOptions::create_new(FileAccess::write)
        : options.destination_policy() == DestinationPolicy::replace_existing
            ? OpenOptions::truncate_existing() : OpenOptions::create_or_truncate();
    auto output_result = open(destination, output_options);
    if (!output_result) return std::move(output_result).error();

    File input = input_result.take();
    File output = output_result.take();
    auto buffer = detail::allocation_or_terminate([] { return std::vector<std::byte>(64 * 1024); });
    std::optional<Error> transfer_error;
    while (!transfer_error)
    {
      auto read = input.read_some(buffer);
      if (!read) { transfer_error.emplace(std::move(read).error()); break; }
      if (read.value() == 0) break;
      std::size_t written = 0;
      while (written < read.value())
      {
        auto result = output.write_some(std::span<const std::byte>{buffer}.first(read.value()).subspan(written));
        if (!result) { transfer_error.emplace(std::move(result).error()); break; }
        written += result.value();
      }
    }
    auto input_close = input.close();
    auto output_close = output.close();
    if (!transfer_error && !input_close) transfer_error.emplace(std::move(input_close).error());
    if (!transfer_error && !output_close) transfer_error.emplace(std::move(output_close).error());
    if (!transfer_error) return Result<void>::success();
    if (destination_created)
    {
      std::error_code cleanup_error;
      std::filesystem::remove(native_path(destination), cleanup_error);
      if (cleanup_error)
        return Error{errors::cleanup_failed, "copy destination cleanup failed", std::move(*transfer_error)};
    }
    return std::move(*transfer_error);
  }
  Result<void> rename_replace(const Path &source, const Path &destination, RenameOptions)
  {
    if (!::MoveFileExW(native_path(source).c_str(), native_path(destination).c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
      const auto code = static_cast<int>(::GetLastError());
      ErrorIdentity identity = errors::rename_failed;
      if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND) identity = errors::not_found;
      else if (code == ERROR_ACCESS_DENIED) identity = errors::permission_denied;
      else if (code == ERROR_NOT_SAME_DEVICE) identity = errors::not_same_filesystem;
      return detail::allocation_or_terminate([&]
      {
        return Error{identity, "MoveFileExW: Windows error " + std::to_string(code)};
      });
    }
    return Result<void>::success();
  }

  std::intptr_t detail_duplicate_process_handle(const File &file) noexcept
  {
    if (!file.is_open()) files_contract_violation();
    const auto source = ::_get_osfhandle(file.descriptor_);
    if (source == -1) return -1;
    HANDLE duplicate = nullptr;
    if (!::DuplicateHandle(::GetCurrentProcess(), reinterpret_cast<HANDLE>(source),
                           ::GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS))
      return -1;
    return reinterpret_cast<std::intptr_t>(duplicate);
  }

  bool detail_set_process_handle_inheritable(std::intptr_t handle, bool inheritable) noexcept
  {
    if (handle == -1) return false;
    return ::SetHandleInformation(reinterpret_cast<HANDLE>(handle), HANDLE_FLAG_INHERIT,
                                   inheritable ? HANDLE_FLAG_INHERIT : 0) != 0;
  }

  bool detail_close_process_handle(std::intptr_t handle) noexcept
  {
    if (handle == -1) return false;
    return ::CloseHandle(reinterpret_cast<HANDLE>(handle)) != 0;
  }
} // namespace vix::files
