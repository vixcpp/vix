#ifndef VIX_PROCESS_EXECUTABLE_HPP
#define VIX_PROCESS_EXECUTABLE_HPP

#include <memory>
#include <string_view>

#include <vix/Result.hpp>
#include <vix/files/Path.hpp>

namespace vix::process
{
  class Process;
  class ProcessSpec;
  vix::Result<Process> spawn(ProcessSpec);
  class Executable
  {
  public:
    Executable() = delete;
    Executable(const Executable &);
    Executable(Executable &&) noexcept;
    Executable &operator=(const Executable &);
    Executable &operator=(Executable &&) noexcept;
    ~Executable();
    [[nodiscard]] static vix::Result<Executable> path(vix::files::Path);
    [[nodiscard]] static vix::Result<Executable> search_path(std::string_view);

  private:
    struct State;
    explicit Executable(std::shared_ptr<const State>) noexcept;
    std::shared_ptr<const State> state_;
    friend class ProcessSpec;
    friend vix::Result<Process> spawn(ProcessSpec);
    friend struct detail_process_access;
  };
}

#endif
