/**
 * @file Files.hpp
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
 *
 * @brief Filesystem operations for the Vix Files domain.
 *
 * @details
 * Provides the public entry point for working with files,
 * directories, paths, and filesystem metadata.
 *
 * The domain supports:
 * - Opening files for reading and writing.
 * - Reading binary and UTF-8 file contents.
 * - Writing binary and UTF-8 data.
 * - Inspecting filesystem metadata.
 * - Creating and removing directories.
 * - Iterating over directory entries.
 * - Copying, renaming, and removing files.
 *
 * Operations return vix::Result<T> to distinguish
 * success from expected filesystem failures.
 *
 * @par Quick start
 * @code{.cpp}
 * #include <vix/files/Files.hpp>
 *
 * int main()
 * {
 *     using namespace vix::files;
 *
 *     auto path = Path::from_utf8("hello.txt");
 *     if (!path)
 *         return 1;
 *
 *     auto written = write_utf8(
 *         path.value(),
 *         "Hello from Vix!",
 *         WriteOptions::create_or_replace());
 *
 *     if (!written)
 *         return 1;
 *
 *     auto content = read_utf8(
 *         path.value(),
 *         ReadLimit{1024});
 *
 *     if (!content)
 *         return 1;
 *
 *     return 0;
 * }
 * @endcode
 *
 * @par Explicit policies
 * Operations that can create, replace, or remove data
 * require explicit options where appropriate.
 *
 * This makes potentially destructive behavior visible
 * at the call site.
 *
 * @par Filesystem safety
 * File operations are not generally transactional.
 * A failed write or copy may leave partial changes.
 *
 * Filesystem contents may also change between separate
 * operations.
 *
 * @see Path
 * @see File
 * @see FileInfo
 * @see DirectoryIterator
 * @see OpenOptions
 * @see WriteOptions
 * @see CopyOptions
 */

#ifndef VIX_FILES_FILES_HPP
#define VIX_FILES_FILES_HPP

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <vix/Result.hpp>

#include <vix/files/DirectoryIterator.hpp>
#include <vix/files/Errors.hpp>
#include <vix/files/File.hpp>
#include <vix/files/FileInfo.hpp>
#include <vix/files/Options.hpp>
#include <vix/files/Path.hpp>

namespace vix::files
{
  /**
   * @brief Opens a regular file with explicit access options.
   *
   * @param path Path of the file to open.
   * @param options Access mode and opening disposition.
   *
   * @return An owning File on success, or an Error
   *         if the operation fails.
   *
   * @details
   * OpenOptions determines whether the file must exist,
   * may be created, may be truncated, or is opened
   * for appending.
   *
   * Write-capable opening rejects a final symbolic link.
   * Read-only opening may follow symbolic links.
   *
   * @par Example
   * @code{.cpp}
   * auto opened = vix::files::open(
   *     path,
   *     vix::files::OpenOptions::existing(
   *         vix::files::FileAccess::read));
   *
   * if (opened)
   * {
   *     auto file = opened.take();
   *     // Read from file.
   * }
   * @endcode
   *
   * @note
   * The returned File owns its descriptor and
   * closes it when destroyed.
   *
   * @see File
   * @see OpenOptions
   */
  [[nodiscard]] vix::Result<File> open(
      const Path &path,
      OpenOptions options);

  /**
   * @brief Reads the complete contents of a file as bytes.
   *
   * @param path Path of the file to read.
   * @param limit Maximum number of bytes permitted.
   *
   * @return A vector containing the file contents,
   *         or an Error if reading fails.
   *
   * @details
   * Opens the file for reading and reads until
   * the end of the file.
   *
   * If the contents exceed the specified ReadLimit,
   * the operation fails with errors::size_limit_exceeded.
   *
   * The function does not silently truncate content.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::read_all(
   *     path,
   *     vix::files::ReadLimit{1024 * 1024});
   *
   * if (result)
   * {
   *     auto bytes = result.take();
   *     // Process the bytes.
   * }
   * @endcode
   *
   * @note
   * This function reads binary data without
   * interpreting its encoding.
   *
   * @note
   * Reading requires memory proportional to
   * the size of the returned file contents.
   *
   * @see read_utf8()
   * @see ReadLimit
   */
  [[nodiscard]] vix::Result<std::vector<std::byte>> read_all(
      const Path &path,
      ReadLimit limit);

  /**
   * @brief Reads a complete file as UTF-8 text.
   *
   * @param path Path of the file to read.
   * @param limit Maximum permitted file size in bytes.
   *
   * @return The file contents as a UTF-8 string,
   *         or an Error if reading or validation fails.
   *
   * @details
   * Reads the complete file and validates its contents
   * as UTF-8.
   *
   * Invalid UTF-8 produces errors::invalid_utf8.
   *
   * Files exceeding the requested limit produce
   * errors::size_limit_exceeded.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::read_utf8(
   *     path,
   *     vix::files::ReadLimit{4096});
   *
   * if (result)
   * {
   *     std::string text = result.take();
   * }
   * @endcode
   *
   * @note
   * This function validates UTF-8 but does not
   * parse or interpret a text file's contents.
   *
   * @see read_all()
   * @see write_utf8()
   */
  [[nodiscard]] vix::Result<std::string> read_utf8(
      const Path &path,
      ReadLimit limit);

  /**
   * @brief Writes all supplied bytes to a file.
   *
   * @param path Destination file path.
   * @param data Bytes to write.
   * @param options Destination creation or replacement policy.
   *
   * @return Success when all bytes are written and
   *         the file closes successfully, or an Error.
   *
   * @details
   * Opens the destination according to WriteOptions
   * and repeatedly writes until all bytes are transferred.
   *
   * The operation handles partial writes internally.
   *
   * @par Example
   * @code{.cpp}
   * std::array<std::byte, 3> bytes{
   *     std::byte{0x41},
   *     std::byte{0x42},
   *     std::byte{0x43}
   * };
   *
   * auto result = vix::files::write_all(
   *     path,
   *     bytes,
   *     vix::files::WriteOptions::create_new());
   * @endcode
   *
   * @warning
   * Replacing an existing file truncates its contents.
   *
   * An error after opening may leave a partially
   * written destination.
   *
   * This operation is not transactional or atomic.
   *
   * @note
   * A final symbolic link is not accepted as
   * a write destination.
   *
   * @see WriteOptions
   * @see write_utf8()
   */
  [[nodiscard]] vix::Result<void> write_all(
      const Path &path,
      std::span<const std::byte> data,
      WriteOptions options);

  /**
   * @brief Writes UTF-8 text to a file.
   *
   * @param path Destination file path.
   * @param text UTF-8 encoded text to write.
   * @param options Destination creation or replacement policy.
   *
   * @return Success when writing completes,
   *         or an Error if validation or writing fails.
   *
   * @details
   * Validates the supplied text before opening
   * the destination for writing.
   *
   * Invalid UTF-8 produces errors::invalid_utf8.
   *
   * Valid text is written using write_all().
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::write_utf8(
   *     path,
   *     "Hello, Vix!",
   *     vix::files::WriteOptions::create_or_replace());
   * @endcode
   *
   * @note
   * Invalid UTF-8 is rejected before the destination
   * is opened or truncated.
   *
   * @warning
   * A later filesystem error may still leave
   * partial changes.
   *
   * @see write_all()
   * @see read_utf8()
   */
  [[nodiscard]] vix::Result<void> write_utf8(
      const Path &path,
      std::string_view text,
      WriteOptions options);

  /**
   * @brief Retrieves filesystem metadata for a path.
   *
   * @param path Path to inspect.
   *
   * @return FileInfo on success, or an Error
   *         if metadata retrieval fails.
   *
   * @details
   * Retrieves the filesystem object's type,
   * optional size, and optional last modification time.
   *
   * Symbolic links are followed when obtaining
   * metadata about their targets.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::info(path);
   *
   * if (result)
   * {
   *     const auto &metadata = result.value();
   *
   *     if (metadata.size())
   *     {
   *         auto bytes = *metadata.size();
   *     }
   * }
   * @endcode
   *
   * @note
   * Metadata represents an observation of the
   * filesystem, not a permanent snapshot.
   *
   * @see FileInfo
   * @see exists()
   */
  [[nodiscard]] vix::Result<FileInfo> info(
      const Path &path);

  /**
   * @brief Checks whether a filesystem path exists.
   *
   * @param path Path to examine.
   *
   * @return true if the target exists,
   *         false if it does not exist,
   *         or an Error if the check fails.
   *
   * @details
   * Existence checking follows symbolic links.
   *
   * A dangling symbolic link may therefore
   * report false.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::exists(path);
   *
   * if (result)
   * {
   *     bool found = result.take();
   * }
   * @endcode
   *
   * @note
   * Existence checks do not reserve the path.
   * The filesystem may change immediately afterward.
   *
   * @see info()
   */
  [[nodiscard]] vix::Result<bool> exists(
      const Path &path);

  /**
   * @brief Creates a single directory.
   *
   * @param path Directory path to create.
   *
   * @return Success when creation completes,
   *         or an Error if creation fails.
   *
   * @details
   * Creates the final directory component.
   *
   * Parent directories must already exist.
   *
   * If the target already exists, the operation
   * reports an error.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::create_directory(path);
   * @endcode
   *
   * @note
   * Directory permissions and creation behavior
   * are subject to platform and filesystem rules.
   *
   * @see create_directories()
   */
  [[nodiscard]] vix::Result<void> create_directory(
      const Path &path);

  /**
   * @brief Creates a directory and any missing parents.
   *
   * @param path Directory path to create.
   *
   * @return Success when the directory hierarchy exists,
   *         or an Error if creation fails.
   *
   * @details
   * Creates missing intermediate directories.
   *
   * Existing directory components are accepted.
   *
   * An existing component that is not a directory
   * causes the operation to fail.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::create_directories(path);
   * @endcode
   *
   * @note
   * An empty path is rejected.
   *
   * @warning
   * On failure, some intermediate directories
   * may already have been created.
   *
   * @see create_directory()
   */
  [[nodiscard]] vix::Result<void> create_directories(
      const Path &path);

  /**
   * @brief Opens a directory for sequential iteration.
   *
   * @param path Directory path to traverse.
   *
   * @return An owning DirectoryIterator on success,
   *         or an Error if opening fails.
   *
   * @details
   * Iterates over immediate directory entries
   * without recursively traversing subdirectories.
   *
   * The final path component must be a directory,
   * not a symbolic link.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::iterate(path);
   *
   * if (result)
   * {
   *     auto iterator = result.take();
   *
   *     while (true)
   *     {
   *         auto next = iterator.next();
   *         if (!next)
   *             break;
   *
   *         auto entry = next.take();
   *         if (!entry)
   *             break;
   *
   *         // Process entry->path().
   *     }
   * }
   * @endcode
   *
   * @note
   * Directory iteration order is unspecified.
   * Directory contents may change during traversal.
   *
   * @see DirectoryIterator
   * @see DirectoryEntry
   */
  [[nodiscard]] vix::Result<DirectoryIterator> iterate(
      const Path &path);

  /**
   * @brief Removes a file or non-directory entry.
   *
   * @param path Path of the entry to remove.
   *
   * @return Success when removal completes,
   *         or an Error if removal fails.
   *
   * @details
   * Removes a filesystem entry without recursively
   * traversing directories.
   *
   * A final symbolic link is removed as an entry;
   * its target is not removed.
   *
   * Directories are rejected.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::remove_file(path);
   * @endcode
   *
   * @warning
   * This operation is destructive and does not
   * provide rollback.
   *
   * @see remove_directory()
   * @see remove_tree()
   */
  [[nodiscard]] vix::Result<void> remove_file(
      const Path &path);

  /**
   * @brief Removes an empty directory.
   *
   * @param path Directory path to remove.
   *
   * @return Success when removal completes,
   *         or an Error if removal fails.
   *
   * @details
   * The target must be a directory that can be
   * removed by the underlying filesystem.
   *
   * A non-empty directory normally causes failure.
   *
   * This operation does not remove directory contents.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::remove_directory(path);
   * @endcode
   *
   * @note
   * Use remove_tree() for recursive removal.
   *
   * @see remove_tree()
   */
  [[nodiscard]] vix::Result<void> remove_directory(
      const Path &path);

  /**
   * @brief Recursively removes a filesystem tree.
   *
   * @param path Root path to remove.
   * @param options Policy for handling a missing target.
   *
   * @return Success when removal completes,
   *         or an Error if removal fails.
   *
   * @details
   * Removes a directory and its contents recursively.
   *
   * A final symbolic link is removed without
   * recursively following its target.
   *
   * RemoveTreeOptions determines whether a missing
   * root path is treated as success or failure.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::remove_tree(
   *     path,
   *     vix::files::RemoveTreeOptions::ignore_missing());
   * @endcode
   *
   * @warning
   * This operation permanently removes filesystem
   * entries and has no rollback mechanism.
   *
   * A failure may leave a partially removed tree.
   *
   * @note
   * A final symbolic link does not cause traversal
   * of the linked directory.
   *
   * @see RemoveTreeOptions
   * @see remove_file()
   * @see remove_directory()
   */
  [[nodiscard]] vix::Result<void> remove_tree(
      const Path &path,
      RemoveTreeOptions options);

  /**
   * @brief Copies the contents of a regular file.
   *
   * @param source Source file path.
   * @param destination Destination file path.
   * @param options Destination creation or replacement policy.
   *
   * @return Success when copying completes,
   *         or an Error if copying fails.
   *
   * @details
   * Copies file contents from source to destination.
   *
   * The source must resolve to a regular file.
   * A symbolic link may be followed at the source.
   *
   * A final symbolic link at the destination
   * is rejected.
   *
   * CopyOptions determines whether the destination
   * must be absent or may be replaced.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::copy_file(
   *     source,
   *     destination,
   *     vix::files::CopyOptions::create_new());
   * @endcode
   *
   * @par Failure handling
   * When copying fails after creating a new
   * destination, Vix attempts to remove it.
   *
   * If cleanup also fails, errors::cleanup_failed
   * preserves the original failure as its cause.
   *
   * @warning
   * Copying is not atomic.
   *
   * Replacing an existing destination may leave
   * partially copied contents if the operation fails.
   *
   * @see CopyOptions
   * @see DestinationPolicy
   */
  [[nodiscard]] vix::Result<void> copy_file(
      const Path &source,
      const Path &destination,
      CopyOptions options);

  /**
   * @brief Renames or moves a filesystem entry with replacement.
   *
   * @param source Path of the existing entry.
   * @param destination Target path.
   * @param options Rename replacement policy.
   *
   * @return Success when renaming completes,
   *         or an Error if renaming fails.
   *
   * @details
   * Renames an entry using the platform's native
   * filesystem rename operation.
   *
   * Replacement of an existing destination is
   * requested, subject to filesystem restrictions.
   *
   * The source and destination must be on
   * the same filesystem.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::rename_replace(
   *     source,
   *     destination);
   * @endcode
   *
   * @par Explicit options
   * The default options are equivalent to
   * RenameOptions::replace().
   *
   * @warning
   * This operation does not provide a universal
   * cross-platform atomic replacement guarantee.
   *
   * Replacing directories or other filesystem
   * objects is subject to platform restrictions.
   *
   * @note
   * Cross-filesystem renaming is not supported.
   * Use copy_file() followed by an appropriate
   * removal operation when a copy-based move
   * is acceptable.
   *
   * @see RenameOptions
   * @see copy_file()
   */
  [[nodiscard]] vix::Result<void> rename_replace(
      const Path &source,
      const Path &destination,
      RenameOptions options = RenameOptions::replace());

} // namespace vix::files

#endif
