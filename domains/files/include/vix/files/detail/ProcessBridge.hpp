#ifndef VIX_FILES_DETAIL_PROCESS_BRIDGE_HPP
#define VIX_FILES_DETAIL_PROCESS_BRIDGE_HPP

#include <vix/files/File.hpp>

namespace vix::files
{
  /**
   * Internal provider bridge.  It is not a Files public API and exists only
   * so a sibling native provider can duplicate a borrowed File for child
   * stdio without exposing native handles to Files users.
   *
   * Returns -1 and leaves errno set on native duplication failure.
   */
  int detail_duplicate_native_handle(const File &) noexcept;
} // namespace vix::files

#endif
