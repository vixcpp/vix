#ifndef VIX_FILES_DETAIL_PROCESS_BRIDGE_HPP
#define VIX_FILES_DETAIL_PROCESS_BRIDGE_HPP

#include <vix/files/File.hpp>

#include <cstdint>

namespace vix::files
{
  /**
   * Internal provider bridge.  It is not a Files public API and exists only
   * so a sibling native provider can duplicate a borrowed File for child
   * stdio without exposing native handles to Files users.
   *
   * POSIX descriptor bridge used by the current POSIX Process provider.
   */
#ifndef _WIN32
  int detail_duplicate_native_handle(const File &) noexcept;
#endif

  /**
   * Platform-neutral private bridge for a future native child-stdio provider.
   * The opaque integer carries a POSIX descriptor or Windows HANDLE. It is
   * initially non-inheritable, owned by the caller, and must be released
   * exactly once with the matching close function. A child-spawn provider
   * must opt into inheritance explicitly. Failure returns -1 and leaves the
   * platform error state.
   */
  std::intptr_t detail_duplicate_process_handle(const File &) noexcept;
  [[nodiscard]] bool detail_set_process_handle_inheritable(std::intptr_t, bool) noexcept;
  [[nodiscard]] bool detail_close_process_handle(std::intptr_t) noexcept;
} // namespace vix::files

#endif
