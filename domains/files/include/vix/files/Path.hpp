/**
 * @file Path.hpp
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

#ifndef VIX_FILES_PATH_HPP
#define VIX_FILES_PATH_HPP

#include <optional>
#include <string>
#include <string_view>

#include <vix/Result.hpp>

namespace vix::files
{
  /**
   * @brief Represents a filesystem path.
   *
   * @details
   * Path provides a portable way to represent and manipulate
   * file and directory paths.
   *
   * It supports:
   * - Creating paths from UTF-8 strings.
   * - Converting paths to UTF-8 or display text.
   * - Checking absolute and relative paths.
   * - Joining paths.
   * - Normalizing path components.
   * - Extracting parent directories, filenames and extensions.
   *
   * Use from_utf8() to create a Path from application text.
   *
   * @par Example
   * @code{.cpp}
   * auto result = vix::files::Path::from_utf8(
   *     "data/config.json");
   *
   * if (!result)
   * {
   *     // Handle result.error().
   *     return;
   * }
   *
   * auto path = result.take();
   *
   * if (auto name = path.filename())
   * {
   *     // Use the filename.
   * }
   * @endcode
   *
   * @par Path operations
   * Path manipulation is lexical.
   *
   * Operations such as joined(), parent(), filename(),
   * and lexically_normal() do not require a file or
   * directory to exist.
   *
   * They do not access the filesystem to resolve
   * symbolic links or verify path existence.
   *
   * @par Platform behavior
   * Path interpretation follows the supported platform's
   * filesystem conventions.
   *
   * Absolute path detection, separators and lexical
   * normalization may differ between POSIX and Windows.
   *
   * @par Ownership
   * Path is a value type supporting copying and moving.
   * It owns its path representation.
   *
   * @note
   * A Path is not a filesystem capability or a security
   * boundary. Lexical normalization does not guarantee
   * that a path remains within a particular directory.
   *
   * @see from_utf8()
   * @see joined()
   * @see lexically_normal()
   */
  class Path
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * Use from_utf8() to create a path from UTF-8 text.
     */
    Path() = delete;

    /**
     * @brief Copies a path.
     *
     * @param other Path to copy.
     */
    Path(const Path &);

    /**
     * @brief Move-constructs a path.
     *
     * @param other Path whose representation is transferred.
     */
    Path(Path &&) noexcept;

    /**
     * @brief Copies another path into this path.
     *
     * @param other Path to copy.
     * @return Reference to this Path.
     */
    Path &operator=(const Path &);

    /**
     * @brief Move-assigns another path.
     *
     * @param other Path whose representation is transferred.
     * @return Reference to this Path.
     */
    Path &operator=(Path &&) noexcept;

    /**
     * @brief Destroys the path.
     */
    ~Path() = default;

    /**
     * @brief Creates a path from UTF-8 text.
     *
     * @param value UTF-8 encoded path string.
     *
     * @return A Path on success, or an Error if the
     *         supplied path cannot be accepted.
     *
     * @details
     * The input must contain valid UTF-8 and must
     * not contain embedded NUL characters.
     *
     * On Windows, the path must also be convertible
     * to the native Windows path representation.
     *
     * An empty string is accepted.
     *
     * @par Example
     * @code{.cpp}
     * auto result = vix::files::Path::from_utf8(
     *     "documents/report.txt");
     *
     * if (result)
     * {
     *     auto path = result.take();
     * }
     * @endcode
     *
     * @note
     * This function does not check whether the path exists.
     *
     * @see to_utf8()
     */
    [[nodiscard]] static vix::Result<Path> from_utf8(std::string_view value);

    /**
     * @brief Converts the path to UTF-8 text.
     *
     * @return The path as a UTF-8 string, or an Error
     *         when lossless conversion is unavailable.
     *
     * @details
     * This operation is intended for obtaining a valid
     * UTF-8 representation of the path.
     *
     * A path obtained from the filesystem may contain
     * native names that cannot be represented as UTF-8.
     *
     * @par Example
     * @code{.cpp}
     * auto result = path.to_utf8();
     *
     * if (result)
     * {
     *     std::string text = result.take();
     * }
     * @endcode
     *
     * @see from_utf8()
     * @see display()
     */
    [[nodiscard]] vix::Result<std::string> to_utf8() const;

    /**
     * @brief Returns a printable representation of the path.
     *
     * @return A string suitable for diagnostic display.
     *
     * @details
     * Bytes outside printable ASCII are represented
     * using hexadecimal escape sequences.
     *
     * This allows native path contents to be displayed
     * without requiring valid UTF-8.
     *
     * @par Example
     * @code{.cpp}
     * auto text = path.display();
     *
     * // Use text in a diagnostic message.
     * @endcode
     *
     * @note
     * The display representation is not intended for
     * reconstructing a path or performing filesystem
     * operations.
     *
     * Use to_utf8() when actual UTF-8 path text is needed.
     *
     * @see to_utf8()
     */
    [[nodiscard]] std::string display() const;

    /**
     * @brief Checks whether the path is empty.
     *
     * @return true if the stored path contains no characters,
     *         false otherwise.
     *
     * @note
     * An empty path is not the same as the current
     * directory represented by ".".
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @brief Checks whether the path is absolute.
     *
     * @return true if the path is absolute according
     *         to the platform's path rules.
     *
     * @details
     * POSIX paths beginning with '/' are absolute.
     *
     * Windows also recognizes supported drive-rooted
     * paths and paths beginning with a separator.
     *
     * @see is_relative()
     */
    [[nodiscard]] bool is_absolute() const noexcept;

    /**
     * @brief Checks whether the path is relative.
     *
     * @return true if the path is not absolute.
     *
     * @note
     * An empty path is considered relative.
     *
     * @see is_absolute()
     */
    [[nodiscard]] bool is_relative() const noexcept;

    /**
     * @brief Returns a lexically normalized path.
     *
     * @return A new Path with normalized components.
     *
     * @details
     * Lexical normalization simplifies path spelling,
     * including redundant separators and "." or ".."
     * components where applicable.
     *
     * The original Path remains unchanged.
     *
     * @par Example
     * @code{.cpp}
     * auto result = vix::files::Path::from_utf8(
     *     "data/./config/../settings.json");
     *
     * if (result)
     * {
     *     auto normalized =
     *         result.take().lexically_normal();
     * }
     * @endcode
     *
     * @note
     * Normalization does not access the filesystem
     * or resolve symbolic links.
     *
     * @warning
     * A normalized path is not necessarily safe for
     * use within a restricted directory.
     *
     * @see joined()
     */
    [[nodiscard]] Path lexically_normal() const;

    /**
     * @brief Joins this path with a relative path.
     *
     * @param relative Relative path to append.
     *
     * @return A new Path containing the joined path.
     *
     * @pre
     * The supplied path must be relative.
     *
     * @par Example
     * @code{.cpp}
     * auto base = vix::files::Path::from_utf8(
     *     "documents");
     *
     * auto child = vix::files::Path::from_utf8(
     *     "report.txt");
     *
     * if (base && child)
     * {
     *     auto combined = base.value().joined(
     *         child.value());
     * }
     * @endcode
     *
     * @warning
     * Passing an absolute path violates the contract
     * and terminates the process.
     *
     * @warning
     * Joining a relative path containing ".." does not
     * guarantee containment within the base directory.
     *
     * @note
     * This function does not verify that the resulting
     * path exists.
     *
     * @see lexically_normal()
     */
    [[nodiscard]] Path joined(const Path &relative) const;

    /**
     * @brief Returns the parent portion of the path.
     *
     * @return The parent Path, or std::nullopt
     *         when no parent portion is available.
     *
     * @par Example
     * @code{.cpp}
     * auto result = vix::files::Path::from_utf8(
     *     "documents/report.txt");
     *
     * if (result)
     * {
     *     auto parent = result.value().parent();
     *     // Parent: documents
     * }
     * @endcode
     *
     * @note
     * A path consisting only of a filename may
     * have no parent.
     *
     * @see filename()
     */
    [[nodiscard]] std::optional<Path> parent() const;

    /**
     * @brief Returns the filename portion of the path.
     *
     * @return The final filename component, or
     *         std::nullopt if none is available.
     *
     * @par Example
     * @code{.cpp}
     * auto result = vix::files::Path::from_utf8(
     *     "documents/report.txt");
     *
     * if (result)
     * {
     *     auto name = result.value().filename();
     *     // Filename: report.txt
     * }
     * @endcode
     *
     * @note
     * The filename is returned as a Path, not
     * as a string.
     *
     * Use to_utf8() to obtain UTF-8 text.
     *
     * @see parent()
     * @see extension()
     */
    [[nodiscard]] std::optional<Path> filename() const;

    /**
     * @brief Returns the filename extension.
     *
     * @return The extension as a Path, or
     *         std::nullopt when no extension exists.
     *
     * @details
     * The extension includes its leading dot.
     *
     * @par Example
     * @code{.cpp}
     * auto result = vix::files::Path::from_utf8(
     *     "archive.tar.gz");
     *
     * if (result)
     * {
     *     auto ext = result.value().extension();
     *     // Extension: .gz
     * }
     * @endcode
     *
     * @note
     * A leading dot alone does not necessarily
     * indicate a filename extension.
     *
     * @note
     * Extension extraction is lexical and does
     * not inspect file contents.
     *
     * @see filename()
     */
    [[nodiscard]] std::optional<Path> extension() const;

    /**
     * @brief Compares two paths for equality.
     *
     * @param left First path.
     * @param right Second path.
     *
     * @return true if their stored path representations
     *         are equal, false otherwise.
     *
     * @note
     * Equality is lexical and does not establish
     * whether two paths refer to the same filesystem object.
     *
     * Equivalent spellings, symbolic links, or filesystem
     * case rules are not resolved by this comparison.
     */
    friend bool operator==(const Path &, const Path &) noexcept;

    /**
     * @brief Compares two paths for inequality.
     *
     * @param left First path.
     * @param right Second path.
     *
     * @return true if the stored path representations
     *         differ, false otherwise.
     *
     * @see operator==()
     */
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

#endif // VIX_FILES_PATH_HPP
