#ifndef VIX_FILES_OPTIONS_HPP
#define VIX_FILES_OPTIONS_HPP

#include <cstdint>
#include <limits>

namespace vix::files
{
  enum class FileAccess
  {
    read,
    write,
    read_write
  };
  enum class OpenDisposition
  {
    existing,
    open_or_create,
    create_new,
    truncate_existing,
    create_or_truncate,
    append_existing,
    append_or_create
  };
  enum class SeekOrigin
  {
    begin,
    current,
    end
  };
  enum class DestinationPolicy
  {
    create_new,
    replace_existing,
    create_or_replace
  };
  enum class MissingPathPolicy
  {
    error,
    ignore
  };

  class OpenOptions
  {
  public:
    OpenOptions() = delete;
    OpenOptions(const OpenOptions &) = default;
    OpenOptions(OpenOptions &&) noexcept = default;
    OpenOptions &operator=(const OpenOptions &) = default;
    OpenOptions &operator=(OpenOptions &&) noexcept = default;

    [[nodiscard]] static constexpr OpenOptions existing(FileAccess access) noexcept { return {access, OpenDisposition::existing}; }
    [[nodiscard]] static constexpr OpenOptions open_or_create(FileAccess access) noexcept { return {access, OpenDisposition::open_or_create}; }
    [[nodiscard]] static constexpr OpenOptions create_new(FileAccess access) noexcept { return {access, OpenDisposition::create_new}; }
    [[nodiscard]] static constexpr OpenOptions truncate_existing() noexcept { return {FileAccess::write, OpenDisposition::truncate_existing}; }
    [[nodiscard]] static constexpr OpenOptions create_or_truncate() noexcept { return {FileAccess::write, OpenDisposition::create_or_truncate}; }
    [[nodiscard]] static constexpr OpenOptions append_existing() noexcept { return {FileAccess::write, OpenDisposition::append_existing}; }
    [[nodiscard]] static constexpr OpenOptions append_or_create() noexcept { return {FileAccess::write, OpenDisposition::append_or_create}; }
    [[nodiscard]] constexpr FileAccess access() const noexcept { return access_; }
    [[nodiscard]] constexpr OpenDisposition disposition() const noexcept { return disposition_; }

  private:
    constexpr OpenOptions(FileAccess access, OpenDisposition disposition) noexcept : access_(access), disposition_(disposition) {}
    FileAccess access_;
    OpenDisposition disposition_;
  };

  class ReadLimit
  {
  public:
    ReadLimit() = delete;
    constexpr explicit ReadLimit(std::uint64_t bytes) noexcept : bytes_(bytes) {}
    ReadLimit(const ReadLimit &) = default;
    ReadLimit(ReadLimit &&) noexcept = default;
    ReadLimit &operator=(const ReadLimit &) = default;
    ReadLimit &operator=(ReadLimit &&) noexcept = default;
    [[nodiscard]] constexpr std::uint64_t bytes() const noexcept { return bytes_; }
    [[nodiscard]] constexpr bool operator==(const ReadLimit &) const noexcept = default;
    [[nodiscard]] static constexpr ReadLimit maximum() noexcept { return ReadLimit{std::numeric_limits<std::uint64_t>::max()}; }

  private:
    std::uint64_t bytes_;
  };

  class WriteOptions
  {
  public:
    WriteOptions() = delete;
    [[nodiscard]] static constexpr WriteOptions create_new() noexcept { return WriteOptions{DestinationPolicy::create_new}; }
    [[nodiscard]] static constexpr WriteOptions replace_existing() noexcept { return WriteOptions{DestinationPolicy::replace_existing}; }
    [[nodiscard]] static constexpr WriteOptions create_or_replace() noexcept { return WriteOptions{DestinationPolicy::create_or_replace}; }
    WriteOptions(const WriteOptions &) = default;
    WriteOptions(WriteOptions &&) noexcept = default;
    WriteOptions &operator=(const WriteOptions &) = default;
    WriteOptions &operator=(WriteOptions &&) noexcept = default;
    [[nodiscard]] constexpr DestinationPolicy destination_policy() const noexcept { return policy_; }

  private:
    constexpr explicit WriteOptions(DestinationPolicy policy) noexcept : policy_(policy) {}
    DestinationPolicy policy_;
  };

  class CopyOptions
  {
  public:
    CopyOptions() = delete;
    [[nodiscard]] static constexpr CopyOptions create_new() noexcept { return CopyOptions{DestinationPolicy::create_new}; }
    [[nodiscard]] static constexpr CopyOptions replace_existing() noexcept { return CopyOptions{DestinationPolicy::replace_existing}; }
    [[nodiscard]] static constexpr CopyOptions create_or_replace() noexcept { return CopyOptions{DestinationPolicy::create_or_replace}; }
    CopyOptions(const CopyOptions &) = default;
    CopyOptions(CopyOptions &&) noexcept = default;
    CopyOptions &operator=(const CopyOptions &) = default;
    CopyOptions &operator=(CopyOptions &&) noexcept = default;
    [[nodiscard]] constexpr DestinationPolicy destination_policy() const noexcept { return policy_; }

  private:
    constexpr explicit CopyOptions(DestinationPolicy policy) noexcept : policy_(policy) {}
    DestinationPolicy policy_;
  };

  class RenameOptions
  {
  public:
    RenameOptions() = delete;
    [[nodiscard]] static constexpr RenameOptions replace() noexcept { return RenameOptions{tag{}}; }
    RenameOptions(const RenameOptions &) = default;
    RenameOptions(RenameOptions &&) noexcept = default;
    RenameOptions &operator=(const RenameOptions &) = default;
    RenameOptions &operator=(RenameOptions &&) noexcept = default;

  private:
    struct tag
    {
    };
    constexpr explicit RenameOptions(tag) noexcept {}
  };

  class RemoveTreeOptions
  {
  public:
    RemoveTreeOptions() = delete;
    [[nodiscard]] static constexpr RemoveTreeOptions require_existing() noexcept { return RemoveTreeOptions{MissingPathPolicy::error}; }
    [[nodiscard]] static constexpr RemoveTreeOptions ignore_missing() noexcept { return RemoveTreeOptions{MissingPathPolicy::ignore}; }
    RemoveTreeOptions(const RemoveTreeOptions &) = default;
    RemoveTreeOptions(RemoveTreeOptions &&) noexcept = default;
    RemoveTreeOptions &operator=(const RemoveTreeOptions &) = default;
    RemoveTreeOptions &operator=(RemoveTreeOptions &&) noexcept = default;
    [[nodiscard]] constexpr MissingPathPolicy missing_path_policy() const noexcept { return policy_; }

  private:
    constexpr explicit RemoveTreeOptions(MissingPathPolicy policy) noexcept : policy_(policy) {}
    MissingPathPolicy policy_;
  };
} // namespace vix::files

#endif
