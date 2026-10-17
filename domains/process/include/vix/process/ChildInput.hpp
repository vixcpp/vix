#ifndef VIX_PROCESS_CHILD_INPUT_HPP
#define VIX_PROCESS_CHILD_INPUT_HPP

#include <cstddef>
#include <optional>
#include <span>

#include <vix/Result.hpp>

namespace vix::process
{
  class ChildInput
  {
  public:
    ChildInput() = delete;
    ChildInput(const ChildInput &) = delete;
    ChildInput &operator=(const ChildInput &) = delete;
    ChildInput(ChildInput &&) noexcept;
    ChildInput &operator=(ChildInput &&) = delete;
    ~ChildInput() noexcept;
    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] vix::Result<std::size_t> write_some(std::span<const std::byte>);
    [[nodiscard]] vix::Result<void> close();

  private:
    explicit ChildInput(int descriptor) noexcept;
    [[maybe_unused]] int descriptor_ = -1;
    std::optional<vix::Error> close_error_;
    friend class Process;
  };
}

#endif
