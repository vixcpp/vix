/**
 * @file Errors.hpp
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
 * @brief Stable error identities for filesystem operations.
 *
 * @details
 * Defines the programmatic error identities used by the
 * Vix Files domain.
 *
 * Every identity belongs to the "vix.files" domain and
 * has a unique, non-zero numeric code.
 *
 * Applications should compare error identities rather than
 * relying on diagnostic messages or platform error codes.
 *
 * @par Example
 * @code{.cpp}
 * #include <vix/Error.hpp>
 * #include <vix/files/Errors.hpp>
 *
 * constexpr auto missing = vix::files::errors::not_found;
 *
 * vix::Error error(missing, "File not found");
 *
 * if (error.identity() == vix::files::errors::not_found)
 * {
 *     // Handle the missing file.
 * }
 * @endcode
 *
 * @par Stability
 * Published identities must retain their domain, code,
 * and meaning across compatible releases.
 *
 * New errors should receive new codes rather than
 * reusing existing identities for different failures.
 *
 * @note
 * These constants identify failures; they do not
 * represent successful operations.
 *
 * @see vix::ErrorIdentity
 * @see vix::Error
 */

#ifndef VIX_FILES_ERRORS_HPP
#define VIX_FILES_ERRORS_HPP

#include <vix/ErrorIdentity.hpp>

namespace vix::files::errors
{
  /**
   * @brief The requested file or directory was not found.
   *
   * Code: 1.
   */
  inline constexpr auto not_found = ErrorIdentity::define<"vix.files">(1);

  /**
   * @brief A filesystem entry already exists.
   *
   * Code: 2.
   *
   * Typically relevant when an operation requires
   * creating a new destination.
   */
  inline constexpr auto already_exists = ErrorIdentity::define<"vix.files">(2);

  /**
   * @brief Access was denied by filesystem permissions.
   *
   * Code: 3.
   */
  inline constexpr auto permission_denied = ErrorIdentity::define<"vix.files">(3);

  /**
   * @brief A supplied filesystem path is invalid.
   *
   * Code: 4.
   *
   * The path cannot be accepted for the requested operation.
   */
  inline constexpr auto invalid_path = ErrorIdentity::define<"vix.files">(4);

  /**
   * @brief A path cannot be represented as valid UTF-8.
   *
   * Code: 5.
   *
   * Relevant when converting a native filesystem path
   * into UTF-8 text.
   */
  inline constexpr auto path_not_utf8 = ErrorIdentity::define<"vix.files">(5);

  /**
   * @brief A symbolic link is not allowed by the operation.
   *
   * Code: 6.
   *
   * Indicates that the operation's symbolic-link policy
   * prohibits the encountered link.
   */
  inline constexpr auto symlink_not_allowed = ErrorIdentity::define<"vix.files">(6);

  /**
   * @brief The target is not an expected file.
   *
   * Code: 7.
   */
  inline constexpr auto not_a_file = ErrorIdentity::define<"vix.files">(7);

  /**
   * @brief The target is not a directory.
   *
   * Code: 8.
   */
  inline constexpr auto not_a_directory = ErrorIdentity::define<"vix.files">(8);

  /**
   * @brief A requested file offset is invalid.
   *
   * Code: 9.
   *
   * Relevant to file positioning and seeking operations.
   */
  inline constexpr auto invalid_offset = ErrorIdentity::define<"vix.files">(9);

  /**
   * @brief File data exceeds an allowed size limit.
   *
   * Code: 10.
   *
   * Can occur when reading content with an explicit
   * ReadLimit.
   */
  inline constexpr auto size_limit_exceeded = ErrorIdentity::define<"vix.files">(10);

  /**
   * @brief Data contains an invalid UTF-8 sequence.
   *
   * Code: 11.
   *
   * Relevant to operations requiring valid UTF-8
   * file content.
   */
  inline constexpr auto invalid_utf8 = ErrorIdentity::define<"vix.files">(11);

  /**
   * @brief A file read operation failed.
   *
   * Code: 12.
   */
  inline constexpr auto read_failed = ErrorIdentity::define<"vix.files">(12);

  /**
   * @brief A file write operation failed.
   *
   * Code: 13.
   *
   * A failed write does not necessarily imply that
   * no bytes were written.
   */
  inline constexpr auto write_failed = ErrorIdentity::define<"vix.files">(13);

  /**
   * @brief Closing a filesystem resource failed.
   *
   * Code: 14.
   */
  inline constexpr auto close_failed = ErrorIdentity::define<"vix.files">(14);

  /**
   * @brief Flushing file data failed.
   *
   * Code: 15.
   */
  inline constexpr auto flush_failed = ErrorIdentity::define<"vix.files">(15);

  /**
   * @brief Creating a filesystem entry failed.
   *
   * Code: 16.
   */
  inline constexpr auto create_failed = ErrorIdentity::define<"vix.files">(16);

  /**
   * @brief Removing a filesystem entry failed.
   *
   * Code: 17.
   */
  inline constexpr auto remove_failed = ErrorIdentity::define<"vix.files">(17);

  /**
   * @brief Renaming a filesystem entry failed.
   *
   * Code: 18.
   */
  inline constexpr auto rename_failed = ErrorIdentity::define<"vix.files">(18);

  /**
   * @brief Copying a file failed.
   *
   * Code: 19.
   *
   * A failed copy may leave a partially written
   * destination.
   */
  inline constexpr auto copy_failed = ErrorIdentity::define<"vix.files">(19);

  /**
   * @brief Cleanup of a filesystem resource failed.
   *
   * Code: 20.
   *
   * Indicates a failure while performing cleanup work.
   */
  inline constexpr auto cleanup_failed = ErrorIdentity::define<"vix.files">(20);

  /**
   * @brief Paths are located on different filesystems.
   *
   * Code: 21.
   *
   * Relevant when an operation requires both paths
   * to belong to the same filesystem.
   */
  inline constexpr auto not_same_filesystem = ErrorIdentity::define<"vix.files">(21);

  /**
   * @brief A required system resource is exhausted.
   *
   * Code: 22.
   */
  inline constexpr auto resource_exhausted = ErrorIdentity::define<"vix.files">(22);

  /**
   * @brief The filesystem has insufficient free space.
   *
   * Code: 23.
   */
  inline constexpr auto no_space = ErrorIdentity::define<"vix.files">(23);

  /**
   * @brief The filesystem does not permit modification.
   *
   * Code: 24.
   *
   * The target filesystem is read-only.
   */
  inline constexpr auto read_only_filesystem = ErrorIdentity::define<"vix.files">(24);

  /**
   * @brief A filesystem path or name exceeds a supported limit.
   *
   * Code: 25.
   */
  inline constexpr auto name_too_long = ErrorIdentity::define<"vix.files">(25);

  /**
   * @brief A filesystem storage quota has been exhausted.
   *
   * Code: 26.
   */
  inline constexpr auto quota_exhausted = ErrorIdentity::define<"vix.files">(26);

  /**
   * @brief A filesystem operation was interrupted.
   *
   * Code: 27.
   *
   * This identity does not represent Vix cancellation
   * completion.
   */
  inline constexpr auto interrupted = ErrorIdentity::define<"vix.files">(27);

  /**
   * @brief A filesystem device reported a failure.
   *
   * Code: 28.
   */
  inline constexpr auto device_failure = ErrorIdentity::define<"vix.files">(28);

  /**
   * @brief A filesystem input/output operation failed.
   *
   * Code: 29.
   *
   * Identifies an I/O failure when the domain reports
   * this general error identity.
   */
  inline constexpr auto io_failure = ErrorIdentity::define<"vix.files">(29);

} // namespace vix::files::errors

#endif // VIX_FILES_ERRORS_HPP
