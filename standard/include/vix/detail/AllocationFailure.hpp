#ifndef VIX_DETAIL_ALLOCATION_FAILURE_HPP
#define VIX_DETAIL_ALLOCATION_FAILURE_HPP

#include <exception>
#include <new>
#include <utility>

namespace vix::detail
{
  /**
   * Process heap exhaustion is not an operational Vix Error. Vix-controlled
   * allocations terminate deterministically in both exception-enabled and
   * exception-disabled builds rather than manufacturing a recoverable Error.
   */
  [[noreturn]] inline void allocation_failure() noexcept
  {
    std::terminate();
  }

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  template <typename Function>
  decltype(auto) allocation_or_terminate(Function &&function)
  {
    try
    {
      return std::forward<Function>(function)();
    }
    catch (const std::bad_alloc &)
    {
      allocation_failure();
    }
  }
#else
  template <typename Function>
  decltype(auto) allocation_or_terminate(Function &&function)
  {
    return std::forward<Function>(function)();
  }
#endif
} // namespace vix::detail

#endif // VIX_DETAIL_ALLOCATION_FAILURE_HPP
