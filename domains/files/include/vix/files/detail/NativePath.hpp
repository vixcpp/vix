#ifndef VIX_FILES_DETAIL_NATIVE_PATH_HPP
#define VIX_FILES_DETAIL_NATIVE_PATH_HPP

#include <string>

#include <vix/files/Path.hpp>

namespace vix::files
{
  // Internal sibling-provider bridge; never part of the public Files API.
  const std::string &detail_native_path(const Path &) noexcept;
}

#endif
