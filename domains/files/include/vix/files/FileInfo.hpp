#ifndef VIX_FILES_FILE_INFO_HPP
#define VIX_FILES_FILE_INFO_HPP

#include <cstdint>
#include <optional>

#include <vix/WallInstant.hpp>

#include <vix/files/Path.hpp>

namespace vix::files
{
  enum class FileType { regular, directory, other };
  enum class DirectoryEntryTypeHint { regular, directory, symlink, other, unknown };

  class FileInfo
  {
  public:
    FileInfo(FileType type, std::optional<std::uint64_t> size, std::optional<vix::WallInstant> last_modified) noexcept
        : type_(type), size_(size), last_modified_(last_modified) {}
    [[nodiscard]] FileType type() const noexcept { return type_; }
    [[nodiscard]] const std::optional<std::uint64_t> &size() const noexcept { return size_; }
    [[nodiscard]] const std::optional<vix::WallInstant> &last_modified() const noexcept { return last_modified_; }
  private:
    FileType type_;
    std::optional<std::uint64_t> size_;
    std::optional<vix::WallInstant> last_modified_;
  };

  class DirectoryEntry
  {
  public:
    DirectoryEntry(Path path, DirectoryEntryTypeHint type_hint) noexcept;
    DirectoryEntry(const DirectoryEntry &) = default;
    DirectoryEntry(DirectoryEntry &&) noexcept = default;
    DirectoryEntry &operator=(const DirectoryEntry &) = default;
    DirectoryEntry &operator=(DirectoryEntry &&) noexcept = default;
    [[nodiscard]] const Path &path() const noexcept { return path_; }
    [[nodiscard]] DirectoryEntryTypeHint type_hint() const noexcept { return type_hint_; }
  private:
    Path path_;
    DirectoryEntryTypeHint type_hint_;
  };
} // namespace vix::files

#endif
