/**
 * @file FileInfo.hpp
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

#ifndef VIX_FILES_FILE_INFO_HPP
#define VIX_FILES_FILE_INFO_HPP

#include <cstdint>
#include <optional>

#include <vix/WallInstant.hpp>

#include <vix/files/Path.hpp>

namespace vix::files
{
  /**
   * @brief Classifies a filesystem object.
   *
   * @details
   * Used by FileInfo to identify the general type
   * of a filesystem object.
   *
   * @see FileInfo
   */
  enum class FileType
  {
    regular,   ///< A regular file.
    directory, ///< A directory.
    other      ///< Another filesystem object type.
  };

  /**
   * @brief Provides an approximate type for a directory entry.
   *
   * @details
   * Directory iteration may provide a type hint without
   * performing a separate metadata lookup.
   *
   * A hint is not authoritative and may be unavailable,
   * incomplete, or outdated.
   *
   * Use info() when current filesystem metadata is needed.
   *
   * @see DirectoryEntry
   * @see FileInfo
   */
  enum class DirectoryEntryTypeHint
  {
    regular,   ///< Entry appears to be a regular file.
    directory, ///< Entry appears to be a directory.
    symlink,   ///< Entry appears to be a symbolic link.
    other,     ///< Entry appears to have another type.
    unknown    ///< Entry type could not be determined.
  };

  /**
   * @brief Describes the metadata of a filesystem object.
   *
   * @details
   * FileInfo provides information about a file,
   * directory, or another filesystem object.
   *
   * It contains:
   * - The filesystem object type.
   * - An optional file size in bytes.
   * - An optional last modification timestamp.
   *
   * Obtain FileInfo using vix::files::info().
   *
   * @par Example
   * @code{.cpp}
   * #include <vix/files/Files.hpp>
   *
   * auto path = vix::files::Path::from_utf8("data.txt");
   *
   * if (path)
   * {
   *     auto result = vix::files::info(path.value());
   *
   *     if (result)
   *     {
   *         const auto &metadata = result.value();
   *
   *         if (metadata.type() == vix::files::FileType::regular)
   *         {
   *             if (metadata.size())
   *             {
   *                 auto bytes = *metadata.size();
   *                 // Use the file size.
   *             }
   *         }
   *     }
   * }
   * @endcode
   *
   * @par Optional metadata
   * Some metadata may not be available for every
   * filesystem object or platform.
   *
   * Always check size() and last_modified()
   * before accessing their values.
   *
   * @par Value semantics
   * FileInfo stores its metadata by value.
   * It does not own or keep a file open.
   *
   * @note
   * FileInfo is a snapshot of metadata.
   * Filesystem contents and attributes may change
   * after the information is obtained.
   *
   * @see FileType
   * @see vix::files::info()
   */
  class FileInfo
  {
  public:
    /**
     * @brief Constructs filesystem metadata.
     *
     * @param type Type of the filesystem object.
     * @param size Optional size in bytes.
     * @param last_modified Optional modification timestamp.
     *
     * @note
     * std::nullopt indicates unavailable metadata.
     */
    FileInfo(FileType type,
             std::optional<std::uint64_t> size,
             std::optional<vix::WallInstant> last_modified) noexcept
        : type_(type),
          size_(size),
          last_modified_(last_modified)
    {
    }

    /**
     * @brief Returns the filesystem object type.
     *
     * @return The FileType classification.
     */
    [[nodiscard]] FileType type() const noexcept
    {
      return type_;
    }

    /**
     * @brief Returns the optional file size.
     *
     * @return Reference to the optional size in bytes.
     *
     * @details
     * File size is available for supported regular files.
     * It may be unavailable for directories or other
     * filesystem objects.
     *
     * @par Example
     * @code{.cpp}
     * if (metadata.size())
     * {
     *     auto bytes = *metadata.size();
     * }
     * @endcode
     *
     * @note
     * An absent size does not mean the file is empty.
     *
     * The returned reference is valid while the
     * FileInfo object remains alive and unchanged.
     */
    [[nodiscard]] const std::optional<std::uint64_t> &size() const noexcept
    {
      return size_;
    }

    /**
     * @brief Returns the last modification timestamp.
     *
     * @return Reference to the optional WallInstant.
     *
     * @details
     * The timestamp represents the filesystem object's
     * last reported modification time.
     *
     * Availability depends on the filesystem and platform.
     *
     * @par Example
     * @code{.cpp}
     * if (metadata.last_modified())
     * {
     *     auto timestamp =
     *         metadata.last_modified()->unix_nanosecond_count();
     * }
     * @endcode
     *
     * @note
     * The timestamp uses wall-clock time.
     * It is not suitable for measuring elapsed
     * execution time or enforcing deadlines.
     *
     * The returned reference must not outlive
     * the FileInfo object.
     *
     * @see vix::WallInstant
     */
    [[nodiscard]] const std::optional<vix::WallInstant> &last_modified() const noexcept
    {
      return last_modified_;
    }

  private:
    FileType type_;
    std::optional<std::uint64_t> size_;
    std::optional<vix::WallInstant> last_modified_;
  };

  /**
   * @brief Represents an entry discovered during directory iteration.
   *
   * @details
   * DirectoryEntry provides the path and an optional-quality
   * type classification for a directory entry.
   *
   * Entries are obtained through DirectoryIterator::next().
   *
   * Each entry contains:
   * - A Path identifying the discovered entry.
   * - A DirectoryEntryTypeHint describing its apparent type.
   *
   * @par Example
   * @code{.cpp}
   * auto path = entry.path();
   *
   * auto name = path.filename();
   *
   * if (entry.type_hint() ==
   *     vix::files::DirectoryEntryTypeHint::directory)
   * {
   *     // The entry appears to be a directory.
   * }
   * @endcode
   *
   * @par Type hints
   * A type hint is intended for lightweight classification.
   *
   * It does not guarantee that the entry still exists
   * or that its type remains unchanged.
   *
   * When a definitive metadata lookup is required,
   * use vix::files::info().
   *
   * @par Ownership
   * DirectoryEntry owns its Path value.
   *
   * It supports copying and moving independently
   * of the directory iterator that produced it.
   *
   * @note
   * DirectoryEntry does not hold an open file handle.
   *
   * @see DirectoryIterator
   * @see DirectoryEntryTypeHint
   * @see vix::files::info()
   */
  class DirectoryEntry
  {
  public:
    /**
     * @brief Constructs a directory entry.
     *
     * @param path Path of the discovered entry.
     * @param type_hint Apparent filesystem object type.
     *
     * @note
     * The supplied path is stored by value.
     */
    DirectoryEntry(Path path,
                   DirectoryEntryTypeHint type_hint) noexcept;

    /**
     * @brief Copies a directory entry.
     */
    DirectoryEntry(const DirectoryEntry &) = default;

    /**
     * @brief Move-constructs a directory entry.
     */
    DirectoryEntry(DirectoryEntry &&) noexcept = default;

    /**
     * @brief Copy-assigns a directory entry.
     *
     * @return Reference to this entry.
     */
    DirectoryEntry &operator=(const DirectoryEntry &) = default;

    /**
     * @brief Move-assigns a directory entry.
     *
     * @return Reference to this entry.
     */
    DirectoryEntry &operator=(DirectoryEntry &&) noexcept = default;

    /**
     * @brief Returns the entry's filesystem path.
     *
     * @return Read-only reference to the stored Path.
     *
     * @details
     * The path identifies the discovered entry,
     * rather than only its filename component.
     *
     * Use Path::filename() to obtain the final
     * path component.
     *
     * @note
     * The returned reference must not outlive
     * this DirectoryEntry object.
     *
     * @see Path
     */
    [[nodiscard]] const Path &path() const noexcept
    {
      return path_;
    }

    /**
     * @brief Returns the entry's apparent filesystem type.
     *
     * @return A DirectoryEntryTypeHint.
     *
     * @details
     * The hint may identify a regular file, directory,
     * symbolic link, another type, or an unknown type.
     *
     * @warning
     * Do not treat this hint as authoritative metadata.
     * Use info() if an updated metadata lookup is needed.
     */
    [[nodiscard]] DirectoryEntryTypeHint type_hint() const noexcept
    {
      return type_hint_;
    }

  private:
    Path path_;
    DirectoryEntryTypeHint type_hint_;
  };
} // namespace vix::files

#endif
