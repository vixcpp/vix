/**
 * @file DirectoryIterator.hpp
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

#ifndef VIX_FILES_DIRECTORY_ITERATOR_HPP
#define VIX_FILES_DIRECTORY_ITERATOR_HPP

#include <memory>
#include <optional>

#include <vix/Result.hpp>

#include <vix/files/FileInfo.hpp>

namespace vix::files
{
  /**
   * @brief Iterates over the entries of a directory.
   *
   * @details
   * DirectoryIterator provides sequential access to the
   * immediate entries of a filesystem directory.
   *
   * Create an iterator using vix::files::iterate(), then
   * call next() repeatedly to retrieve directory entries.
   *
   * Each call to next() returns:
   * - A DirectoryEntry when an entry is available.
   * - std::nullopt when the directory is exhausted.
   * - An Error if directory iteration fails.
   *
   * @par Example
   * @code{.cpp}
   * #include <vix/files/Files.hpp>
   *
   * int main()
   * {
   *     using namespace vix::files;
   *
   *     auto path = Path::from_utf8("documents");
   *     if (!path)
   *         return 1;
   *
   *     auto opened = iterate(path.value());
   *     if (!opened)
   *         return 1;
   *
   *     auto iterator = opened.take();
   *
   *     while (true)
   *     {
   *         auto result = iterator.next();
   *
   *         if (!result)
   *             return 1;
   *
   *         auto entry = result.take();
   *
   *         if (!entry)
   *             break;
   *
   *         auto name = entry->path().display();
   *         // Process the directory entry.
   *     }
   *
   *     if (!iterator.close())
   *         return 1;
   *
   *     return 0;
   * }
   * @endcode
   *
   * @par Iteration behavior
   * DirectoryIterator visits immediate directory entries.
   *
   * It does not recursively traverse subdirectories.
   * The special "." and ".." entries are skipped.
   *
   * Entry ordering is not guaranteed.
   *
   * Reaching the end of the directory does not
   * automatically close the iterator.
   *
   * @par Entry information
   * Each DirectoryEntry contains a path and a type hint.
   *
   * The type hint may identify a regular file,
   * directory, symbolic link, or another entry type.
   *
   * Type hints are not authoritative filesystem metadata.
   *
   * @par Ownership
   * DirectoryIterator exclusively owns its iteration state.
   *
   * It cannot be copied or move-assigned, but it can
   * be move-constructed.
   *
   * The destructor releases any remaining resources.
   *
   * @par Error handling
   * Operational failures are returned through
   * vix::Result<T>.
   *
   * Calling next() on a closed or moved-from iterator
   * violates the contract and terminates the process.
   *
   * @par Resource lifetime
   * Use close() to release resources explicitly
   * and observe possible closing errors.
   *
   * Otherwise, the destructor performs cleanup
   * without reporting closing errors.
   *
   * @note
   * Directory contents may change during iteration.
   * The iterator does not provide a stable snapshot.
   *
   * @see vix::files::iterate()
   * @see DirectoryEntry
   * @see DirectoryEntryTypeHint
   */
  class DirectoryIterator
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * Obtain an iterator using vix::files::iterate().
     */
    DirectoryIterator() = delete;

    /**
     * @brief Copy construction is not supported.
     *
     * The iterator exclusively owns its directory state.
     */
    DirectoryIterator(const DirectoryIterator &) = delete;

    /**
     * @brief Copy assignment is not supported.
     */
    DirectoryIterator &operator=(const DirectoryIterator &) = delete;

    /**
     * @brief Transfers ownership of a directory iterator.
     *
     * @details
     * Move construction transfers the iteration state
     * to the destination object.
     *
     * The moved-from iterator no longer owns that state.
     *
     * @note
     * Use is_open() to check whether an iterator
     * currently owns an open directory traversal.
     */
    DirectoryIterator(DirectoryIterator &&) noexcept;

    /**
     * @brief Move assignment is not supported.
     */
    DirectoryIterator &operator=(DirectoryIterator &&) = delete;

    /**
     * @brief Destroys the iterator and releases its resources.
     *
     * @details
     * Any remaining open directory traversal is closed
     * during destruction.
     *
     * @note
     * Destructor-based cleanup does not report
     * operational closing errors.
     *
     * Use close() explicitly when such errors matter.
     */
    ~DirectoryIterator();

    /**
     * @brief Checks whether the iterator is open.
     *
     * @return true if the iterator owns an open directory
     *         traversal, false otherwise.
     *
     * @details
     * Returns false after close() or after ownership
     * has been transferred to another iterator.
     *
     * @note
     * Reaching the end of iteration does not
     * necessarily make is_open() return false.
     *
     * @see close()
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Retrieves the next directory entry.
     *
     * @return A Result containing:
     * - A DirectoryEntry when an entry is available.
     * - std::nullopt when iteration is complete.
     * - An Error if iteration fails.
     *
     * @details
     * Advances through the directory and returns
     * one entry at a time.
     *
     * The returned entry contains its filesystem path
     * and an approximate type classification.
     *
     * The "." and ".." entries are excluded.
     *
     * @par Example
     * @code{.cpp}
     * auto result = iterator.next();
     *
     * if (!result)
     * {
     *     // Handle result.error().
     * }
     * else
     * {
     *     auto entry = result.take();
     *
     *     if (entry)
     *     {
     *         // Process entry->path().
     *     }
     *     else
     *     {
     *         // End of directory.
     *     }
     * }
     * @endcode
     *
     * @pre
     * The iterator must be open.
     *
     * @warning
     * Calling next() on a closed or moved-from
     * iterator terminates the process.
     *
     * @note
     * A successful end-of-directory result is not
     * an error and does not automatically close
     * the iterator.
     *
     * @note
     * Entry ordering is platform-dependent.
     *
     * @see DirectoryEntry
     * @see close()
     */
    [[nodiscard]] vix::Result<std::optional<DirectoryEntry>> next();

    /**
     * @brief Closes the directory iterator.
     *
     * @return Success if closing completes, or
     *         an Error if native closing fails.
     *
     * @details
     * Releases the resources associated with
     * the directory traversal.
     *
     * Closing is idempotent for a valid iterator:
     * - The first call releases its resources.
     * - Subsequent calls succeed if no previous
     *   closing error was recorded.
     * - A recorded closing error is preserved.
     *
     * After closing, is_open() returns false.
     *
     * @par Example
     * @code{.cpp}
     * auto result = iterator.close();
     *
     * if (!result)
     * {
     *     // Handle result.error().
     * }
     * @endcode
     *
     * @pre
     * The iterator must retain valid ownership state.
     *
     * @warning
     * Calling close() on a moved-from iterator
     * violates the contract and terminates the process.
     *
     * @warning
     * Calling next() after close() violates
     * the iterator contract.
     *
     * @note
     * Explicitly closing an iterator allows
     * applications to observe closing failures
     * that a destructor cannot report.
     *
     * @see is_open()
     * @see next()
     */
    [[nodiscard]] vix::Result<void> close();

  private:
    struct State;

    explicit DirectoryIterator(std::unique_ptr<State>) noexcept;

    std::unique_ptr<State> state_;

    friend vix::Result<DirectoryIterator> iterate(const Path &);
  };
} // namespace vix::files

#endif // VIX_FILES_DIRECTORY_ITERATOR_HPP
