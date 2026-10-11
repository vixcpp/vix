#ifndef VIX_PROCESS_STDIO_HPP
#define VIX_PROCESS_STDIO_HPP

#include <vix/files/File.hpp>

namespace vix::process
{
  class Process;
  class ProcessSpec;
  vix::Result<Process> spawn(ProcessSpec);
  class StandardInput
  {
  public:
    StandardInput() = delete;
    StandardInput(const StandardInput &) noexcept = default;
    StandardInput(StandardInput &&) noexcept = default;
    StandardInput &operator=(const StandardInput &) noexcept = default;
    StandardInput &operator=(StandardInput &&) noexcept = default;
    [[nodiscard]] static StandardInput inherit() noexcept;
    [[nodiscard]] static StandardInput null() noexcept;
    [[nodiscard]] static StandardInput pipe() noexcept;
    [[nodiscard]] static StandardInput from_file(vix::files::File &) noexcept;

  private:
    enum class Kind
    {
      inherit,
      null,
      pipe,
      file
    };
    explicit StandardInput(Kind kind, vix::files::File *file = nullptr) noexcept : kind_(kind), file_(file) {}
    Kind kind_;
    vix::files::File *file_;
    friend struct detail_process_access;
    friend vix::Result<Process> spawn(ProcessSpec);
  };

  class StandardOutput
  {
  public:
    StandardOutput() = delete;
    StandardOutput(const StandardOutput &) noexcept = default;
    StandardOutput(StandardOutput &&) noexcept = default;
    StandardOutput &operator=(const StandardOutput &) noexcept = default;
    StandardOutput &operator=(StandardOutput &&) noexcept = default;
    [[nodiscard]] static StandardOutput inherit() noexcept;
    [[nodiscard]] static StandardOutput null() noexcept;
    [[nodiscard]] static StandardOutput pipe() noexcept;
    [[nodiscard]] static StandardOutput to_file(vix::files::File &) noexcept;

  private:
    enum class Kind
    {
      inherit,
      null,
      pipe,
      file
    };
    explicit StandardOutput(Kind kind, vix::files::File *file = nullptr) noexcept : kind_(kind), file_(file) {}
    Kind kind_;
    vix::files::File *file_;
    friend struct detail_process_access;
    friend vix::Result<Process> spawn(ProcessSpec);
  };

  class StandardError
  {
  public:
    StandardError() = delete;
    StandardError(const StandardError &) noexcept = default;
    StandardError(StandardError &&) noexcept = default;
    StandardError &operator=(const StandardError &) noexcept = default;
    StandardError &operator=(StandardError &&) noexcept = default;
    [[nodiscard]] static StandardError inherit() noexcept;
    [[nodiscard]] static StandardError null() noexcept;
    [[nodiscard]] static StandardError pipe() noexcept;
    [[nodiscard]] static StandardError to_file(vix::files::File &) noexcept;
    [[nodiscard]] static StandardError merge_with_stdout() noexcept;

  private:
    enum class Kind
    {
      inherit,
      null,
      pipe,
      file,
      merge_stdout
    };
    explicit StandardError(Kind kind, vix::files::File *file = nullptr) noexcept : kind_(kind), file_(file) {}
    Kind kind_;
    vix::files::File *file_;
    friend struct detail_process_access;
    friend vix::Result<Process> spawn(ProcessSpec);
  };
}

#endif
