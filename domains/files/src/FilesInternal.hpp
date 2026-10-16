#ifndef VIX_FILES_INTERNAL_HPP
#define VIX_FILES_INTERNAL_HPP

#include <string_view>

namespace vix::files
{
  [[nodiscard]] bool detail_valid_utf8(std::string_view value) noexcept;
}

#endif
