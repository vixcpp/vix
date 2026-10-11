#ifndef VIX_PROCESS_PROCESS_SPEC_HPP
#define VIX_PROCESS_PROCESS_SPEC_HPP

#include <memory>
#include <optional>
#include <string_view>

#include <vix/Result.hpp>
#include <vix/files/Path.hpp>

#include <vix/process/Environment.hpp>
#include <vix/process/Executable.hpp>
#include <vix/process/Stdio.hpp>

namespace vix::process
{
  class Process;
  class ProcessSpec;
  vix::Result<Process> spawn(ProcessSpec);
  class ProcessSpec
  {
  public:
    ProcessSpec() = delete;
    explicit ProcessSpec(Executable);
    ProcessSpec(const ProcessSpec &);
    ProcessSpec(ProcessSpec &&) noexcept;
    ProcessSpec &operator=(const ProcessSpec &);
    ProcessSpec &operator=(ProcessSpec &&) noexcept;
    ~ProcessSpec();
    [[nodiscard]] vix::Result<void> add_argument(std::string_view);
    void set_environment(Environment);
    [[nodiscard]] vix::Result<void> set_working_directory(vix::files::Path);
    void set_stdin(StandardInput);
    void set_stdout(StandardOutput);
    void set_stderr(StandardError);

  private:
    struct State;
    std::unique_ptr<State> state_;
    friend struct detail_process_access;
    friend class Process;
    friend vix::Result<Process> spawn(ProcessSpec);
  };
}

#endif
