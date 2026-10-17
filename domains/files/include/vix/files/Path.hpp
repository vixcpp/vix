#ifndef VIX_FILES_PATH_HPP
#define VIX_FILES_PATH_HPP

#include <optional>
#include <string>
#include <string_view>

#include <vix/Result.hpp>

namespace vix::files
{
  class Path
  {
  public:
    Path() = delete;
    Path(const Path &);
    Path(Path &&) noexcept;
    Path &operator=(const Path &);
    Path &operator=(Path &&) noexcept;
    ~Path() = default;

    [[nodiscard]] static vix::Result<Path> from_utf8(std::string_view value);
    [[nodiscard]] vix::Result<std::string> to_utf8() const;
    [[nodiscard]] std::string display() const;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool is_absolute() const noexcept;
    [[nodiscard]] bool is_relative() const noexcept;
    [[nodiscard]] Path lexically_normal() const;
    [[nodiscard]] Path joined(const Path &relative) const;
    [[nodiscard]] std::optional<Path> parent() const;
    [[nodiscard]] std::optional<Path> filename() const;
    [[nodiscard]] std::optional<Path> extension() const;

    friend bool operator==(const Path &, const Path &) noexcept;
    friend bool operator!=(const Path &, const Path &) noexcept;

  private:
    explicit Path(std::string native) noexcept;
    std::string native_;

    friend class File;
    friend class DirectoryIterator;
    friend class DirectoryEntry;
    friend Path detail_path_from_native(std::string);
    friend const std::string &detail_native_path(const Path &) noexcept;
  };
} // namespace vix::files

#endif
