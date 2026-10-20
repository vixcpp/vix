/**
 * @file File.hpp
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
  /**
   * @brief Provides controlled access to an open file.
   *
   * @details
   * File represents an opened regular file and owns its
   * underlying operating-system file descriptor.
   *
   * It supports:
   * - Reading and writing bytes.
   * - Moving the current file position.
   * - Querying the current position.
   * - Flushing written data.
   * - Explicitly closing the file.
   *
   * Create a File using vix::files::open() with an
   * appropriate OpenOptions value.
   *
   * @par Example
   * @code{.cpp}
   * #include <array>
   * #include <vix/files/Files.hpp>
   *
   * int main()
   * {
   *     using namespace vix::files;
   *
   *     auto path = Path::from_utf8("data.txt");
   *     if (!path)
   *         return 1;
   *
   *     auto opened = open(
   *         path.value(),
   *         OpenOptions::existing(FileAccess::read));
   *
   *     if (!opened)
   *         return 1;
   *
   *     auto file = opened.take();
   *
   *     std::array<std::byte, 1024> buffer{};
   *     auto result = file.read_some(buffer);
   *
   *     if (!result)
   *         return 1;
   *
   *     auto bytes_read = result.take();
   *     // Process buffer[0 .. bytes_read).
   *
   *     if (!file.close())
   *         return 1;
   *
   *     return 0;
   * }
   * @endcode
   *
   * @par Ownership
   * File has exclusive ownership of its native descriptor.
   *
   * A File cannot be copied, but it can be move-constructed.
   * Moving transfers ownership to the new File.
   *
   * The destructor releases an open descriptor.
   *
   * @par Reading and writing
   * read_some() and write_some() may process fewer bytes
   * than requested.
   *
   * Applications must inspect the returned byte count
   * and repeat operations when necessary.
   *
   * @par Error handling
   * Expected filesystem failures are reported through
   * vix::Result<T>.
   *
   * Accessing a closed file or performing an operation
   * incompatible with its access mode violates the
   * File contract and terminates the process.
   *
   * @par Resource lifetime
   * Use close() when closing errors must be observed.
   *
   * The destructor also releases an open descriptor,
   * but does not report closing errors.
   *
   * @note
   * File operations use the underlying platform's
   * filesystem behavior and error reporting.
   *
   * @see vix::files::open()
   * @see OpenOptions
   * @see FileAccess
   * @see OpenDisposition
   */
  class File
  {
  public:
    /**
     * @brief Default construction is not supported.
     *
     * Obtain a File using vix::files::open().
     */
    File() = delete;

    /**
     * @brief Copy construction is not supported.
     *
     * A File exclusively owns its native descriptor.
     */
    File(const File &) = delete;

    /**
     * @brief Copy assignment is not supported.
     */
    File &operator=(const File &) = delete;

    /**
     * @brief Transfers ownership of an open file.
     *
     * @details
     * Move construction transfers the native descriptor
     * and associated file state to the destination.
     *
     * The moved-from File no longer owns the descriptor.
     *
     * @note
     * Use is_open() to check whether a File still
     * owns an open descriptor.
     */
    File(File &&) noexcept;

    /**
     * @brief Move assignment is not supported.
     */
    File &operator=(File &&) = delete;

    /**
     * @brief Destroys the File and releases its descriptor.
     *
     * @details
     * If the File is still open, its native descriptor
     * is closed during destruction.
     *
     * @note
     * Destructor-based cleanup does not report
     * closing errors.
     *
     * Use close() explicitly when the result of
     * closing must be observed.
     */
    ~File();

    /**
     * @brief Checks whether the file is open.
     *
     * @return true if this object owns an open descriptor,
     *         false otherwise.
     *
     * @details
     * Returns false after a successful or failed close
     * attempt that releases ownership of the descriptor,
     * or after ownership has been moved away.
     *
     * @see close()
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Returns the file's configured access mode.
     *
     * @return The FileAccess selected when opening the file.
     *
     * @pre The file must be open.
     *
     * @warning
     * Calling access() on a closed or moved-from File
     * terminates the process.
     *
     * @see FileAccess
     */
    [[nodiscard]] FileAccess access() const noexcept;

    /**
     * @brief Returns the file's opening disposition.
     *
     * @return The OpenDisposition used to open the file.
     *
     * @pre The file must be open.
     *
     * @warning
     * Calling disposition() on a closed or moved-from File
     * terminates the process.
     *
     * @see OpenDisposition
     */
    [[nodiscard]] OpenDisposition disposition() const noexcept;

    /**
     * @brief Reads bytes from the current file position.
     *
     * @param buffer Writable buffer receiving the data.
     *
     * @return The number of bytes read on success,
     *         or an Error if reading fails.
     *
     * @details
     * Reads up to buffer.size() bytes and advances
     * the file position by the number actually read.
     *
     * A successful read may return fewer bytes
     * than requested.
     *
     * A return value of zero means either:
     * - The end of the file was reached.
     * - The supplied buffer was empty.
     *
     * @par Example
     * @code{.cpp}
     * std::array<std::byte, 4096> buffer{};
     *
     * auto result = file.read_some(buffer);
     *
     * if (result)
     * {
     *     auto count = result.take();
     *
     *     if (count == 0)
     *     {
     *         // End of file or empty buffer.
     *     }
     * }
     * @endcode
     *
     * @pre
     * The file must be open with read or read_write access.
     *
     * @warning
     * Reading from a closed or write-only File
     * terminates the process.
     *
     * @note
     * This operation does not guarantee that the
     * requested buffer is completely filled.
     *
     * @see write_some()
     * @see FileAccess
     */
    [[nodiscard]] vix::Result<std::size_t> read_some(std::span<std::byte> buffer);

    /**
     * @brief Writes bytes at the current file position.
     *
     * @param buffer Read-only buffer containing the data.
     *
     * @return The number of bytes written on success,
     *         or an Error if writing fails.
     *
     * @details
     * Attempts to write up to buffer.size() bytes.
     *
     * A successful write may transfer fewer bytes
     * than requested.
     *
     * Applications requiring complete output must
     * handle partial writes.
     *
     * An empty buffer succeeds with a count of zero.
     *
     * When opened in append mode, writes are directed
     * to the end of the file.
     *
     * @par Example
     * @code{.cpp}
     * std::array<std::byte, 3> data{
     *     std::byte{0x41},
     *     std::byte{0x42},
     *     std::byte{0x43}
     * };
     *
     * auto result = file.write_some(data);
     *
     * if (result)
     * {
     *     auto written = result.take();
     *     // Check whether all bytes were written.
     * }
     * @endcode
     *
     * @pre
     * The file must be open with write or read_write access.
     *
     * @warning
     * Writing to a closed or read-only File
     * terminates the process.
     *
     * @note
     * An operational write failure may occur after
     * some bytes have already been written.
     *
     * @see read_some()
     * @see flush()
     */
    [[nodiscard]] vix::Result<std::size_t> write_some(std::span<const std::byte> buffer);

    /**
     * @brief Changes the current file position.
     *
     * @param offset Signed byte offset relative to origin.
     * @param origin Reference position for the offset.
     *
     * @return The resulting absolute byte position,
     *         or an Error if seeking fails.
     *
     * @details
     * SeekOrigin determines the reference position:
     * - begin: beginning of the file.
     * - current: current file position.
     * - end: end of the file.
     *
     * Negative offsets are permitted when the resulting
     * position is valid.
     *
     * Seeking beyond the end of a regular file does
     * not itself extend the file.
     *
     * @par Example
     * @code{.cpp}
     * auto result = file.seek(0, SeekOrigin::begin);
     *
     * if (result)
     * {
     *     auto position = result.take();
     *     // Position is zero.
     * }
     * @endcode
     *
     * @pre The file must be open.
     *
     * @warning
     * Seeking on a closed File terminates the process.
     *
     * @note
     * Invalid offsets and platform I/O failures
     * are reported through Result.
     *
     * @see tell()
     * @see SeekOrigin
     */
    [[nodiscard]] vix::Result<std::uint64_t> seek(std::int64_t offset, SeekOrigin origin);

    /**
     * @brief Returns the current file position.
     *
     * @return The current absolute byte offset,
     *         or an Error if the position cannot be obtained.
     *
     * @details
     * The returned position is measured in bytes
     * from the beginning of the file.
     *
     * @par Example
     * @code{.cpp}
     * auto result = file.tell();
     *
     * if (result)
     * {
     *     auto position = result.take();
     * }
     * @endcode
     *
     * @pre The file must be open.
     *
     * @warning
     * Calling tell() on a closed File terminates
     * the process.
     *
     * @see seek()
     */
    [[nodiscard]] vix::Result<std::uint64_t> tell() const;

    /**
     * @brief Flushes written file data.
     *
     * @return Success if flushing completes,
     *         or an Error if flushing fails.
     *
     * @details
     * For writable files, requests synchronization
     * through the platform's native file API.
     *
     * On POSIX systems, this uses fsync().
     * On Windows, this uses _commit().
     *
     * For a read-only File, flush() succeeds
     * without requesting a native synchronization.
     *
     * @par Example
     * @code{.cpp}
     * auto result = file.flush();
     *
     * if (!result)
     * {
     *     // Handle the flush error.
     * }
     * @endcode
     *
     * @pre The file must be open.
     *
     * @warning
     * Flushing a closed File terminates the process.
     *
     * @note
     * Successful flushing does not provide a general
     * transaction or atomic replacement guarantee.
     *
     * @see write_some()
     * @see close()
     */
    [[nodiscard]] vix::Result<void> flush();

    /**
     * @brief Closes the file and releases its descriptor.
     *
     * @return Success if closing succeeds, or an Error
     *         if the native close operation fails.
     *
     * @details
     * Closing releases ownership of the native descriptor.
     *
     * The operation is idempotent:
     * - Closing an open file attempts native closure.
     * - Closing an already-closed file succeeds if no
     *   previous close error was recorded.
     * - If a previous close attempt failed, subsequent
     *   calls return the saved error.
     *
     * Once a close attempt has been made, the File
     * no longer owns the descriptor, even if the
     * native close operation reported an error.
     *
     * @par Example
     * @code{.cpp}
     * auto result = file.close();
     *
     * if (!result)
     * {
     *     // Handle result.error().
     * }
     *
     * // The File no longer owns an open descriptor.
     * @endcode
     *
     * @note
     * Closing errors should be checked explicitly
     * when they matter to the application.
     *
     * @warning
     * Do not call read_some(), write_some(), seek(),
     * tell(), or flush() after the file is closed.
     *
     * @see is_open()
     * @see flush()
     */
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
