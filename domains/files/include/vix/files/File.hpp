#ifndef VIX_FILES_FILE_HPP
#define VIX_FILES_FILE_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <vix/Result.hpp>

#include <vix/files/Options.hpp>
#include <vix/files/Path.hpp>

namespace vix::files
{
  class File
  {
  public:
    File() = delete;
    File(const File &) = delete;
    File &operator=(const File &) = delete;
    File(File &&) noexcept;
    File &operator=(File &&) = delete;
    ~File();

    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] FileAccess access() const noexcept;
    [[nodiscard]] OpenDisposition disposition() const noexcept;
    [[nodiscard]] vix::Result<std::size_t> read_some(std::span<std::byte> buffer);
    [[nodiscard]] vix::Result<std::size_t> write_some(std::span<const std::byte> buffer);
    [[nodiscard]] vix::Result<std::uint64_t> seek(std::int64_t offset, SeekOrigin origin);
    [[nodiscard]] vix::Result<std::uint64_t> tell() const;
    [[nodiscard]] vix::Result<void> flush();
    [[nodiscard]] vix::Result<void> close();

  private:
    File(int descriptor, FileAccess access, OpenDisposition disposition, bool append) noexcept;
    int descriptor_ = -1;
    FileAccess access_ = FileAccess::read;
    OpenDisposition disposition_ = OpenDisposition::existing;
    bool append_ = false;
    std::optional<vix::Error> close_error_;
    friend vix::Result<File> open(const Path &, OpenOptions);
#ifndef _WIN32
    friend int detail_duplicate_native_handle(const File &) noexcept;
#endif
    friend std::intptr_t detail_duplicate_process_handle(const File &) noexcept;
  };
} // namespace vix::files

#endif
