/**
 * @file Options.hpp
 * @author Gaspard Kirira
 *
 * Copyright 2026, Gaspard Kirira.
 * All rights reserved.
 * https://github.com/vixcpp/vix
 *
 * Use of this source code is governed by an MIT license
 * that can be found in the LICENSE file.
 *
 * Vix.cpp
 */

#ifndef VIX_FILES_OPTIONS_HPP
#define VIX_FILES_OPTIONS_HPP

#include <cstdint>
#include <limits>

namespace vix::files
{
  /**
   * @brief Defines the permitted operations on an opened file.
   *
   * @see OpenOptions
   */
  enum class FileAccess
  {
    read,      ///< Allow reading from the file.
    write,     ///< Allow writing to the file.
    read_write ///< Allow both reading and writing.
  };

  /**
   * @brief Defines how a file is opened or created.
   *
   * @details
   * An OpenDisposition determines what happens when the
   * target file already exists or is missing.
   *
   * Use OpenOptions to select a disposition together
   * with the required access mode.
   *
   * @see OpenOptions
   */
  enum class OpenDisposition
  {
    existing,           ///< Open an existing file; fail if missing.
    open_or_create,     ///< Open an existing file or create it if missing.
    create_new,         ///< Create a new file; fail if it already exists.
    truncate_existing,  ///< Open and truncate an existing file.
    create_or_truncate, ///< Create a file or truncate it if it exists.
    append_existing,    ///< Open an existing file for appending.
    append_or_create    ///< Open for appending or create if missing.
  };

  /**
   * @brief Defines the reference position for file seeking.
   *
   * @see File::seek()
   */
  enum class SeekOrigin
  {
    begin,   ///< Offset relative to the beginning of the file.
    current, ///< Offset relative to the current file position.
    end      ///< Offset relative to the end of the file.
  };

  /**
   * @brief Defines how an operation handles its destination.
   *
   * @details
   * Used by WriteOptions and CopyOptions to control
   * whether an existing destination may be replaced.
   *
   * @warning
   * Replacement does not imply atomic file updates.
   */
  enum class DestinationPolicy
  {
    create_new,       ///< Require the destination not to exist.
    replace_existing, ///< Require and replace an existing destination.
    create_or_replace ///< Create if missing or replace if present.
  };

  /**
   * @brief Defines how removal handles a missing target path.
   *
   * @see RemoveTreeOptions
   */
  enum class MissingPathPolicy
  {
    error, ///< Report an error when the target is missing.
    ignore ///< Treat a missing target as successful removal.
  };

  /**
   * @brief Configures how a file is opened.
   *
   * @details
   * OpenOptions combines a FileAccess mode with an
   * OpenDisposition.
   *
   * Use the named factory functions instead of constructing
   * options manually.
   *
   * @par Example
   * @code{.cpp}
   * using namespace vix::files;
   *
   * auto read = OpenOptions::existing(FileAccess::read);
   *
   * auto create = OpenOptions::create_new(FileAccess::write);
   *
   * auto append = OpenOptions::append_or_create();
   * @endcode
   *
   * @note
   * Opening a file can fail because of permissions,
   * filesystem state, or unsupported access combinations.
   *
   * @see FileAccess
   * @see OpenDisposition
   */
  class OpenOptions
  {
  public:
    OpenOptions() = delete;
    OpenOptions(const OpenOptions &) = default;
    OpenOptions(OpenOptions &&) noexcept = default;
    OpenOptions &operator=(const OpenOptions &) = default;
    OpenOptions &operator=(OpenOptions &&) noexcept = default;

    /**
     * @brief Opens an existing file without creating it.
     *
     * @param access Requested file access.
     * @return Options requiring an existing file.
     *
     * @note Fails if the file does not exist.
     */
    [[nodiscard]] static constexpr OpenOptions existing(FileAccess access) noexcept { return {access, OpenDisposition::existing}; }

    /**
     * @brief Opens a file or creates it if missing.
     *
     * @param access Requested file access.
     * @return Options for opening or creating a file.
     *
     * @note Does not truncate an existing file.
     */
    [[nodiscard]] static constexpr OpenOptions open_or_create(FileAccess access) noexcept { return {access, OpenDisposition::open_or_create}; }

    /**
     * @brief Creates a file only if it does not exist.
     *
     * @param access Requested file access.
     * @return Options requiring a new file.
     *
     * @note Fails if the file already exists.
     */
    [[nodiscard]] static constexpr OpenOptions create_new(FileAccess access) noexcept { return {access, OpenDisposition::create_new}; }

    /**
     * @brief Opens and truncates an existing file.
     *
     * @return Write-only options for truncating a file.
     *
     * @warning
     * Existing file contents are discarded when
     * the file is successfully opened.
     *
     * @note Fails if the file does not exist.
     */
    [[nodiscard]] static constexpr OpenOptions truncate_existing() noexcept { return {FileAccess::write, OpenDisposition::truncate_existing}; }

    /**
     * @brief Creates a file or truncates an existing one.
     *
     * @return Write-only options for creating or truncating.
     *
     * @warning
     * Opening an existing file truncates its contents.
     */
    [[nodiscard]] static constexpr OpenOptions create_or_truncate() noexcept { return {FileAccess::write, OpenDisposition::create_or_truncate}; }

    /**
     * @brief Opens an existing file for appending.
     *
     * @return Write-only options for appending.
     *
     * @note
     * Fails if the file does not exist.
     * Writes are directed to the end of the file.
     */
    [[nodiscard]] static constexpr OpenOptions append_existing() noexcept { return {FileAccess::write, OpenDisposition::append_existing}; }

    /**
     * @brief Opens a file for appending or creates it.
     *
     * @return Write-only options for append-or-create.
     *
     * @note
     * Existing contents are preserved.
     * Writes are directed to the end of the file.
     */
    [[nodiscard]] static constexpr OpenOptions append_or_create() noexcept { return {FileAccess::write, OpenDisposition::append_or_create}; }

    /**
     * @brief Returns the configured access mode.
     *
     * @return The requested FileAccess.
     */
    [[nodiscard]] constexpr FileAccess access() const noexcept { return access_; }

    /**
     * @brief Returns the configured opening disposition.
     *
     * @return The selected OpenDisposition.
     */
    [[nodiscard]] constexpr OpenDisposition disposition() const noexcept { return disposition_; }

  private:
    constexpr OpenOptions(FileAccess access, OpenDisposition disposition) noexcept : access_(access), disposition_(disposition) {}
    FileAccess access_;
    OpenDisposition disposition_;
  };

  /**
   * @brief Defines the maximum number of bytes allowed in a read.
   *
   * @details
   * ReadLimit limits the total data accepted by read_all()
   * and read_utf8().
   *
   * If a file contains more bytes than the configured
   * limit, the operation returns an error instead of
   * silently truncating the content.
   *
   * @par Example
   * @code{.cpp}
   * using namespace vix::files;
   *
   * ReadLimit limit{1024 * 1024};
   *
   * // Allow at most 1 MiB of file data.
   * @endcode
   *
   * @note
   * ReadLimit::maximum() uses the largest uint64_t value.
   * It does not guarantee that enough memory is available
   * to read files of arbitrary size.
   *
   * @see read_all()
   * @see read_utf8()
   */
  class ReadLimit
  {
  public:
    ReadLimit() = delete;

    /**
     * @brief Constructs an explicit byte limit.
     *
     * @param bytes Maximum permitted number of bytes.
     *
     * @note A limit of zero permits only empty content.
     */
    constexpr explicit ReadLimit(std::uint64_t bytes) noexcept : bytes_(bytes) {}

    ReadLimit(const ReadLimit &) = default;
    ReadLimit(ReadLimit &&) noexcept = default;
    ReadLimit &operator=(const ReadLimit &) = default;
    ReadLimit &operator=(ReadLimit &&) noexcept = default;

    /**
     * @brief Returns the configured byte limit.
     *
     * @return Maximum permitted number of bytes.
     */
    [[nodiscard]] constexpr std::uint64_t bytes() const noexcept { return bytes_; }

    /**
     * @brief Compares two read limits.
     *
     * @return true if both limits contain the same byte count.
     */
    [[nodiscard]] constexpr bool operator==(const ReadLimit &) const noexcept = default;

    /**
     * @brief Returns the largest representable read limit.
     *
     * @return ReadLimit containing UINT64_MAX.
     *
     * @warning
     * This removes a practical application-level size bound.
     * Prefer an explicit limit for untrusted file content.
     */
    [[nodiscard]] static constexpr ReadLimit maximum() noexcept { return ReadLimit{std::numeric_limits<std::uint64_t>::max()}; }

  private:
    std::uint64_t bytes_;
  };

  /**
   * @brief Configures destination handling for file writes.
   *
   * @details
   * WriteOptions controls whether write_all() and
   * write_utf8() create or replace a destination file.
   *
   * @par Example
   * @code{.cpp}
   * using namespace vix::files;
   *
   * auto options = WriteOptions::create_or_replace();
   * @endcode
   *
   * @warning
   * Replacing a file is not a transactional operation.
   * A failed write may leave partially written contents.
   *
   * @see DestinationPolicy
   * @see write_all()
   * @see write_utf8()
   */
  class WriteOptions
  {
  public:
    WriteOptions() = delete;

    /**
     * @brief Requires the destination file to be absent.
     *
     * @return Options that create a new file.
     *
     * @note Fails if the destination already exists.
     */
    [[nodiscard]] static constexpr WriteOptions create_new() noexcept { return WriteOptions{DestinationPolicy::create_new}; }

    /**
     * @brief Replaces the contents of an existing file.
     *
     * @return Options that require an existing destination.
     *
     * @warning Existing contents are truncated.
     */
    [[nodiscard]] static constexpr WriteOptions replace_existing() noexcept { return WriteOptions{DestinationPolicy::replace_existing}; }

    /**
     * @brief Creates or replaces the destination file.
     *
     * @return Options allowing creation or replacement.
     *
     * @warning Existing contents are truncated.
     */
    [[nodiscard]] static constexpr WriteOptions create_or_replace() noexcept { return WriteOptions{DestinationPolicy::create_or_replace}; }

    WriteOptions(const WriteOptions &) = default;
    WriteOptions(WriteOptions &&) noexcept = default;
    WriteOptions &operator=(const WriteOptions &) = default;
    WriteOptions &operator=(WriteOptions &&) noexcept = default;

    /**
     * @brief Returns the selected destination policy.
     *
     * @return The configured DestinationPolicy.
     */
    [[nodiscard]] constexpr DestinationPolicy destination_policy() const noexcept { return policy_; }

  private:
    constexpr explicit WriteOptions(DestinationPolicy policy) noexcept : policy_(policy) {}
    DestinationPolicy policy_;
  };

  /**
   * @brief Configures destination handling for file copying.
   *
   * @details
   * CopyOptions determines whether copy_file() must create
   * a new destination or may replace an existing file.
   *
   * @par Example
   * @code{.cpp}
   * using namespace vix::files;
   *
   * auto options = CopyOptions::create_new();
   * @endcode
   *
   * @warning
   * Copying is not guaranteed to be atomic.
   * A failed copy may leave a partially written destination.
   *
   * @see DestinationPolicy
   * @see copy_file()
   */
  class CopyOptions
  {
  public:
    CopyOptions() = delete;

    /**
     * @brief Copies only when the destination is absent.
     *
     * @return Options requiring a new destination.
     */
    [[nodiscard]] static constexpr CopyOptions create_new() noexcept { return CopyOptions{DestinationPolicy::create_new}; }

    /**
     * @brief Copies over an existing destination.
     *
     * @return Options requiring an existing destination.
     *
     * @warning Existing destination contents are replaced.
     */
    [[nodiscard]] static constexpr CopyOptions replace_existing() noexcept { return CopyOptions{DestinationPolicy::replace_existing}; }

    /**
     * @brief Copies to a new or existing destination.
     *
     * @return Options allowing creation or replacement.
     */
    [[nodiscard]] static constexpr CopyOptions create_or_replace() noexcept { return CopyOptions{DestinationPolicy::create_or_replace}; }

    CopyOptions(const CopyOptions &) = default;
    CopyOptions(CopyOptions &&) noexcept = default;
    CopyOptions &operator=(const CopyOptions &) = default;
    CopyOptions &operator=(CopyOptions &&) noexcept = default;

    /**
     * @brief Returns the selected destination policy.
     *
     * @return The configured DestinationPolicy.
     */
    [[nodiscard]] constexpr DestinationPolicy destination_policy() const noexcept { return policy_; }

  private:
    constexpr explicit CopyOptions(DestinationPolicy policy) noexcept : policy_(policy) {}
    DestinationPolicy policy_;
  };

  /**
   * @brief Configures replacement behavior for renaming.
   *
   * @details
   * RenameOptions provides the explicit replacement policy
   * used by rename_replace().
   *
   * The current API exposes one supported policy: replace().
   *
   * @note
   * Replacement remains subject to filesystem and
   * platform restrictions.
   *
   * @see rename_replace()
   */
  class RenameOptions
  {
  public:
    RenameOptions() = delete;

    /**
     * @brief Selects destination replacement semantics.
     *
     * @return Options permitting replacement during renaming.
     *
     * @note
     * Renaming across different filesystems is not supported
     * by the domain's rename operation.
     */
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

  /**
   * @brief Configures missing-path handling for recursive removal.
   *
   * @details
   * RemoveTreeOptions controls how remove_tree() behaves
   * when its target path does not exist.
   *
   * It does not change the recursive nature of removal.
   *
   * @par Example
   * @code{.cpp}
   * using namespace vix::files;
   *
   * auto options = RemoveTreeOptions::ignore_missing();
   * @endcode
   *
   * @warning
   * remove_tree() is destructive. These options do not
   * provide recovery, rollback, or transactional guarantees.
   *
   * @see MissingPathPolicy
   * @see remove_tree()
   */
  class RemoveTreeOptions
  {
  public:
    RemoveTreeOptions() = delete;

    /**
     * @brief Requires the target path to exist.
     *
     * @return Options that report a missing target as an error.
     */
    [[nodiscard]] static constexpr RemoveTreeOptions require_existing() noexcept { return RemoveTreeOptions{MissingPathPolicy::error}; }

    /**
     * @brief Ignores an already-missing target path.
     *
     * @return Options treating a missing target as success.
     *
     * @note
     * Other removal failures, such as permission errors,
     * are still reported.
     */
    [[nodiscard]] static constexpr RemoveTreeOptions ignore_missing() noexcept { return RemoveTreeOptions{MissingPathPolicy::ignore}; }

    RemoveTreeOptions(const RemoveTreeOptions &) = default;
    RemoveTreeOptions(RemoveTreeOptions &&) noexcept = default;
    RemoveTreeOptions &operator=(const RemoveTreeOptions &) = default;
    RemoveTreeOptions &operator=(RemoveTreeOptions &&) noexcept = default;

    /**
     * @brief Returns the configured missing-path policy.
     *
     * @return The selected MissingPathPolicy.
     */
    [[nodiscard]] constexpr MissingPathPolicy missing_path_policy() const noexcept { return policy_; }

  private:
    constexpr explicit RemoveTreeOptions(MissingPathPolicy policy) noexcept : policy_(policy) {}
    MissingPathPolicy policy_;
  };
} // namespace vix::files

#endif
