#ifndef VIX_PROCESS_ENVIRONMENT_HPP
#define VIX_PROCESS_ENVIRONMENT_HPP

#include <memory>
#include <string_view>

#include <vix/Result.hpp>

namespace vix::process
{
  class Process;
  class ProcessSpec;
  vix::Result<Process> spawn(ProcessSpec);
  class Environment
  {
  public:
    Environment() = delete;
    Environment(const Environment &);
    Environment(Environment &&) noexcept;
    Environment &operator=(const Environment &);
    Environment &operator=(Environment &&) noexcept;
    ~Environment();
    [[nodiscard]] static Environment inherit() noexcept;
    [[nodiscard]] static Environment empty() noexcept;
    [[nodiscard]] vix::Result<Environment> with(std::string_view name, std::string_view value) const;
    [[nodiscard]] vix::Result<Environment> without(std::string_view name) const;

  private:
    struct State;
    explicit Environment(std::shared_ptr<const State>) noexcept;
    std::shared_ptr<const State> state_;
    friend class ProcessSpec;
    friend vix::Result<Process> spawn(ProcessSpec);
    friend struct detail_process_access;
  };
}

#endif
