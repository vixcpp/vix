#ifndef VIX_PROCESS_PROCESS_HPP
#define VIX_PROCESS_PROCESS_HPP

#include <memory>
#include <optional>

#include <vix/Cancellation.hpp>
#include <vix/Completion.hpp>
#include <vix/Deadline.hpp>
#include <vix/Result.hpp>

#include <vix/process/ChildInput.hpp>
#include <vix/process/ChildOutput.hpp>
#include <vix/process/ExitStatus.hpp>
#include <vix/process/ProcessSpec.hpp>

namespace vix::process
{
  class WaitOptions
  {
  public:
    WaitOptions(vix::CancellationToken cancellation = {},
                std::optional<vix::Deadline> deadline = std::nullopt) noexcept;

  private:
    vix::CancellationToken cancellation_;
    std::optional<vix::Deadline> deadline_;
    friend class Process;
  };

  namespace detail
  {
    struct ProcessRecord;
  }

  class Process
  {
  public:
    Process() = delete;
    Process(const Process &) = delete;
    Process &operator=(const Process &) = delete;
    Process(Process &&) noexcept;
    Process &operator=(Process &&) = delete;
    ~Process() noexcept;

    [[nodiscard]] ProcessId id() const noexcept;
    [[nodiscard]] vix::Result<ExitStatus> wait();
    [[nodiscard]] vix::Completion<ExitStatus> wait(WaitOptions);
    [[nodiscard]] vix::Result<void> terminate();
    [[nodiscard]] vix::Result<ChildInput> take_stdin();
    [[nodiscard]] vix::Result<ChildOutput> take_stdout();
    [[nodiscard]] vix::Result<ChildOutput> take_stderr();

  private:
    Process(std::shared_ptr<detail::ProcessRecord>, int stdin_descriptor,
            int stdout_descriptor, int stderr_descriptor) noexcept;
    std::shared_ptr<detail::ProcessRecord> record_;
    [[maybe_unused]] int stdin_descriptor_{-1};
    [[maybe_unused]] int stdout_descriptor_{-1};
    [[maybe_unused]] int stderr_descriptor_{-1};
    friend vix::Result<Process> spawn(ProcessSpec);
  };

  [[nodiscard]] vix::Result<Process> spawn(ProcessSpec);
}

#endif
