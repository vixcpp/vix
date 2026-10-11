#ifndef VIX_PROCESS_EXIT_STATUS_HPP
#define VIX_PROCESS_EXIT_STATUS_HPP

#include <cstdint>
#include <optional>
#include <string>

namespace vix::process
{
  class ProcessId
  {
  public:
    ProcessId() = delete;
    ProcessId(const ProcessId &) noexcept = default;
    ProcessId(ProcessId &&) noexcept = default;
    ProcessId &operator=(const ProcessId &) noexcept = default;
    ProcessId &operator=(ProcessId &&) noexcept = default;
    [[nodiscard]] std::string display() const;
    friend bool operator==(ProcessId, ProcessId) noexcept = default;

  private:
    ProcessId(std::uint64_t generation, std::uint64_t native) noexcept : generation_(generation), native_(native) {}
    std::uint64_t generation_;
    std::uint64_t native_;
    friend struct detail_process_record_access;
  };

  enum class ExitKind
  {
    exited,
    abnormal
  };

  class ExitStatus
  {
  public:
    ExitStatus() = delete;
    ExitStatus(const ExitStatus &) noexcept = default;
    ExitStatus(ExitStatus &&) noexcept = default;
    ExitStatus &operator=(const ExitStatus &) noexcept = default;
    ExitStatus &operator=(ExitStatus &&) noexcept = default;
    [[nodiscard]] ExitKind kind() const noexcept { return kind_; }
    [[nodiscard]] std::optional<std::uint32_t> exit_code() const noexcept { return code_; }
    [[nodiscard]] bool succeeded() const noexcept { return kind_ == ExitKind::exited && code_ && *code_ == 0; }

  private:
    ExitStatus(ExitKind kind, std::optional<std::uint32_t> code) noexcept : kind_(kind), code_(code) {}
    ExitKind kind_;
    std::optional<std::uint32_t> code_;
    friend struct detail_process_record_access;
  };
}

#endif
