#ifndef VIX_PROCESS_CHILD_OUTPUT_HPP
#define VIX_PROCESS_CHILD_OUTPUT_HPP

#include <cstddef>
#include <optional>
#include <span>

#include <vix/Result.hpp>

namespace vix::process
{
  class ChildOutput
  {
  public:
    ChildOutput() = delete;
    ChildOutput(const ChildOutput &) = delete;
    ChildOutput &operator=(const ChildOutput &) = delete;
    ChildOutput(ChildOutput &&) noexcept;
    ChildOutput &operator=(ChildOutput &&) = delete;
    ~ChildOutput() noexcept;
    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] vix::Result<std::size_t> read_some(std::span<std::byte>);
    [[nodiscard]] vix::Result<void> close();

  private:
    explicit ChildOutput(int descriptor) noexcept;
    int descriptor_ = -1;
    std::optional<vix::Error> close_error_;
    friend class Process;
  };
}

#endif
