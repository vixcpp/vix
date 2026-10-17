#include <vix/process/ProcessDomain.hpp>

#include <vix/detail/AllocationFailure.hpp>
#include <vix/files/detail/NativePath.hpp>
#include <vix/files/detail/ProcessBridge.hpp>

#include <atomic>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <signal.h>
#include <string>
#include <string_view>
#if defined(__APPLE__)
#include <spawn.h>
#include <AvailabilityMacros.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

extern char **environ;

namespace vix::process
{
  struct detail_process_record_access
  {
    [[nodiscard]] static ProcessId id(std::uint64_t generation, std::uint64_t native) noexcept { return ProcessId{generation, native}; }
    [[nodiscard]] static ExitStatus exited(std::uint32_t code) noexcept { return ExitStatus{ExitKind::exited, code}; }
    [[nodiscard]] static ExitStatus abnormal() noexcept { return ExitStatus{ExitKind::abnormal, std::nullopt}; }
    static void set_native(ProcessId &id, std::uint64_t native) noexcept { id.native_ = native; }
  };

  namespace
  {
    [[noreturn]] void contract_violation() noexcept { std::terminate(); }

    [[nodiscard]] Error error(ErrorIdentity identity, std::string_view text)
    {
      return vix::detail::allocation_or_terminate([&] { return Error{identity, std::string(text)}; });
    }

    [[nodiscard]] Error native_error(ErrorIdentity fallback, const char *operation, int value = errno)
    {
      ErrorIdentity identity = fallback;
      switch (value)
      {
      case ENOENT:
        if (fallback == errors::spawn_failed) identity = errors::executable_not_found;
        break;
      case EACCES: case EPERM: identity = errors::access_denied; break;
      case EMFILE: case ENFILE: case EAGAIN: identity = errors::resource_exhausted; break;
      case EPIPE: identity = errors::broken_pipe; break;
      default: break;
      }
      return vix::detail::allocation_or_terminate([&]
      {
        return Error{identity, std::string(operation) + ": " + std::strerror(value)};
      });
    }

    [[nodiscard]] bool valid_utf8(std::string_view value) noexcept
    {
      for (std::size_t i = 0; i < value.size();)
      {
        unsigned char lead = static_cast<unsigned char>(value[i]);
        if (lead <= 0x7f) { ++i; continue; }
        std::size_t count = 0; std::uint32_t code = 0;
        if (lead >= 0xc2 && lead <= 0xdf) { count = 2; code = lead & 0x1f; }
        else if (lead >= 0xe0 && lead <= 0xef) { count = 3; code = lead & 0x0f; }
        else if (lead >= 0xf0 && lead <= 0xf4) { count = 4; code = lead & 0x07; }
        else return false;
        if (i + count > value.size()) return false;
        for (std::size_t j = 1; j < count; ++j)
        {
          const auto next = static_cast<unsigned char>(value[i + j]);
          if ((next & 0xc0) != 0x80) return false;
          code = (code << 6) | (next & 0x3f);
        }
        if ((count == 3 && code < 0x800) || (count == 4 && code < 0x10000) ||
            (code >= 0xd800 && code <= 0xdfff) || code > 0x10ffff) return false;
        i += count;
      }
      return true;
    }

    void close_noexcept(int &descriptor) noexcept
    {
      if (descriptor >= 0) { (void)::close(descriptor); descriptor = -1; }
    }

    [[nodiscard]] int create_pipe_cloexec(int descriptors[2]) noexcept
    {
#if defined(__linux__) && defined(O_CLOEXEC)
      return ::pipe2(descriptors, O_CLOEXEC);
#endif
      if (::pipe(descriptors) != 0) return -1;
      for (int index = 0; index < 2; ++index)
      {
        const int descriptor = descriptors[index];
        const int flags = ::fcntl(descriptor, F_GETFD);
        if (flags < 0 || ::fcntl(descriptor, F_SETFD, flags | FD_CLOEXEC) != 0)
        {
          const int saved = errno;
          close_noexcept(descriptors[0]);
          close_noexcept(descriptors[1]);
          errno = saved;
          return -1;
        }
      }
      return 0;
    }

    [[nodiscard]] Result<void> close_cached(int &descriptor, std::optional<Error> &saved)
    {
      if (descriptor < 0) return saved ? Result<void>{*saved} : Result<void>::success();
      const int closing = std::exchange(descriptor, -1);
      if (::close(closing) == 0) return Result<void>::success();
      saved.emplace(native_error(errors::close_failed, "close"));
      return *saved;
    }

    struct ExecFailure { int stage; int error_number; };
    enum : int { exec_stage = 1, cwd_stage = 2, stdio_stage = 3 };
  }

  struct Executable::State
  {
    bool search{};
    files::Path path;
    std::string name;
    State(bool search_value, files::Path path_value, std::string name_value)
      : search(search_value), path(std::move(path_value)), name(std::move(name_value)) {}
  };

  struct Environment::State
  {
    struct Change { std::string name; std::optional<std::string> value; };
    bool inherited{};
    std::vector<Change> changes;
    explicit State(bool inherited_value) : inherited(inherited_value) {}
  };

  struct ProcessSpec::State
  {
    Executable executable;
    std::vector<std::string> arguments;
    Environment environment;
    std::optional<files::Path> working_directory;
    StandardInput input{StandardInput::inherit()};
    StandardOutput output{StandardOutput::inherit()};
    StandardError error{StandardError::inherit()};
    explicit State(Executable executable_value) : executable(std::move(executable_value)), environment(Environment::inherit()) {}
  };

  namespace detail
  {
    struct ProcessRecord
    {
      explicit ProcessRecord(ProcessId value) noexcept : id(std::move(value)) {}
      std::mutex mutex;
      pid_t pid{-1};
      ProcessId id;
      std::optional<ExitStatus> status;
      std::optional<Error> lost;
      bool abandoned{};
    };

    class Reaper
    {
    public:
      static Reaper &instance()
      {
        // The service is deliberately process-lifetime: joining it during
        // static destruction could block forever on an abandoned live child.
        // Its state is heap-owned and intentionally never destroyed, so the
        // detached worker cannot observe torn-down function-static members.
        static Reaper *value = vix::detail::allocation_or_terminate([] { return new Reaper; });
        return *value;
      }

      [[nodiscard]] Result<void> prepare(const std::shared_ptr<ProcessRecord> &record)
      {
        std::lock_guard lock(mutex_);
        if (!started_)
        {
          pthread_attr_t attributes;
          if (const int code = ::pthread_attr_init(&attributes); code != 0)
            return error(errors::resource_exhausted, "cannot initialize process reaper attributes");
          if (const int code = ::pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED); code != 0)
          {
            (void)::pthread_attr_destroy(&attributes);
            return error(errors::resource_exhausted, "cannot configure detached process reaper");
          }
          const int create_code = ::pthread_create(&thread_, &attributes, &Reaper::entry, this);
          (void)::pthread_attr_destroy(&attributes);
          if (create_code != 0)
            return error(errors::resource_exhausted, "cannot start process reaper");
          started_ = true;
        }
        // Allocation here is intentionally before fork; bad_alloc follows the
        // Standard fatal allocation policy, never resource_exhausted.
        records_.push_back(record);
        return Result<void>::success();
      }

      void remove(const std::shared_ptr<ProcessRecord> &record) noexcept
      {
        std::lock_guard lock(mutex_);
        for (auto it = records_.begin(); it != records_.end(); ++it)
          if (it->get() == record.get()) { records_.erase(it); return; }
      }

      void abandon(const std::shared_ptr<ProcessRecord> &record) noexcept
      {
        { std::lock_guard record_lock(record->mutex); record->abandoned = true; }
        wake_.notify_one();
      }

    private:
      Reaper() = default;
      static void *entry(void *self) noexcept { static_cast<Reaper *>(self)->run(); return nullptr; }
      void run() noexcept
      {
        std::unique_lock wait_lock(wake_mutex_);
        for (;;)
        {
          wake_.wait_for(wait_lock, std::chrono::milliseconds(10));
          wait_lock.unlock();
          {
            std::lock_guard lock(mutex_);
            for (auto it = records_.begin(); it != records_.end();)
            {
              auto &record = *it;
              bool remove_record = false;
              {
                std::lock_guard record_lock(record->mutex);
                if (record->abandoned && !record->status && !record->lost)
                {
                  int native_status = 0;
                  const auto result = ::waitpid(record->pid, &native_status, WNOHANG);
                  if (result == record->pid)
                  {
                    record->status.emplace(WIFEXITED(native_status)
                      ? detail_process_record_access::exited(static_cast<std::uint32_t>(WEXITSTATUS(native_status)))
                      : detail_process_record_access::abnormal());
                  }
                  else if (result < 0 && errno == ECHILD)
                    record->lost.emplace(error(errors::observation_lost, "child observation was lost"));
                }
                remove_record = record->abandoned && (record->status || record->lost);
              }
              if (remove_record) it = records_.erase(it); else ++it;
            }
          }
          wait_lock.lock();
        }
      }
      std::mutex mutex_;
      std::vector<std::shared_ptr<ProcessRecord>> records_;
      pthread_t thread_{};
      bool started_{};
      std::mutex wake_mutex_;
      std::condition_variable wake_;
    };
  } // namespace detail

  std::string ProcessId::display() const
  {
    return vix::detail::allocation_or_terminate([&] { return std::to_string(generation_) + ":" + std::to_string(native_); });
  }

  Executable::Executable(std::shared_ptr<const State> state) noexcept : state_(std::move(state)) {}
  Executable::Executable(const Executable &) = default;
  Executable::Executable(Executable &&) noexcept = default;
  Executable &Executable::operator=(const Executable &) = default;
  Executable &Executable::operator=(Executable &&) noexcept = default;
  Executable::~Executable() = default;
  Result<Executable> Executable::path(files::Path value)
  {
    if (value.empty()) return error(errors::invalid_argument, "executable path is empty");
    return vix::detail::allocation_or_terminate([&] { return Executable{std::make_shared<State>(false, std::move(value), "")}; });
  }
  Result<Executable> Executable::search_path(std::string_view name)
  {
    if (name.empty() || name.find('\0') != std::string_view::npos || !valid_utf8(name) ||
        name.find('/') != std::string_view::npos || name.find('\\') != std::string_view::npos)
      return error(errors::invalid_argument, "invalid PATH executable name");
    auto empty = files::Path::from_utf8("");
    // This branch cannot fail, but preserves Path's deleted default constructor.
    if (!empty) return empty.error();
    return vix::detail::allocation_or_terminate([&] { return Executable{std::make_shared<State>(true, empty.take(), std::string(name))}; });
  }

  Environment::Environment(std::shared_ptr<const State> state) noexcept : state_(std::move(state)) {}
  Environment::Environment(const Environment &) = default;
  Environment::Environment(Environment &&) noexcept = default;
  Environment &Environment::operator=(const Environment &) = default;
  Environment &Environment::operator=(Environment &&) noexcept = default;
  Environment::~Environment() = default;
  Environment Environment::inherit() noexcept { return Environment{vix::detail::allocation_or_terminate([] { return std::make_shared<State>(true); })}; }
  Environment Environment::empty() noexcept { return Environment{vix::detail::allocation_or_terminate([] { return std::make_shared<State>(false); })}; }
  Result<Environment> Environment::with(std::string_view name, std::string_view value) const
  {
    if (name.empty() || name.find('=') != std::string_view::npos || name.find('\0') != std::string_view::npos ||
        value.find('\0') != std::string_view::npos || !valid_utf8(name) || !valid_utf8(value))
      return error(errors::invalid_environment, "invalid environment overlay");
    return vix::detail::allocation_or_terminate([&]() -> Result<Environment>
    {
      auto state = std::make_shared<State>(*state_);
      bool changed = false;
      for (auto &entry : state->changes) if (entry.name == name) { entry.value = std::string(value); changed = true; }
      if (!changed) state->changes.push_back({std::string(name), std::string(value)});
      return Environment{std::move(state)};
    });
  }
  Result<Environment> Environment::without(std::string_view name) const
  {
    if (name.empty() || name.find('=') != std::string_view::npos || name.find('\0') != std::string_view::npos || !valid_utf8(name))
      return error(errors::invalid_environment, "invalid environment variable name");
    return vix::detail::allocation_or_terminate([&]() -> Result<Environment>
    {
      auto state = std::make_shared<State>(*state_);
      bool changed = false;
      for (auto &entry : state->changes) if (entry.name == name) { entry.value.reset(); changed = true; }
      if (!changed) state->changes.push_back({std::string(name), std::nullopt});
      return Environment{std::move(state)};
    });
  }

  StandardInput StandardInput::inherit() noexcept { return StandardInput{Kind::inherit}; }
  StandardInput StandardInput::null() noexcept { return StandardInput{Kind::null}; }
  StandardInput StandardInput::pipe() noexcept { return StandardInput{Kind::pipe}; }
  StandardInput StandardInput::from_file(files::File &file) noexcept { return StandardInput{Kind::file, &file}; }
  StandardOutput StandardOutput::inherit() noexcept { return StandardOutput{Kind::inherit}; }
  StandardOutput StandardOutput::null() noexcept { return StandardOutput{Kind::null}; }
  StandardOutput StandardOutput::pipe() noexcept { return StandardOutput{Kind::pipe}; }
  StandardOutput StandardOutput::to_file(files::File &file) noexcept { return StandardOutput{Kind::file, &file}; }
  StandardError StandardError::inherit() noexcept { return StandardError{Kind::inherit}; }
  StandardError StandardError::null() noexcept { return StandardError{Kind::null}; }
  StandardError StandardError::pipe() noexcept { return StandardError{Kind::pipe}; }
  StandardError StandardError::to_file(files::File &file) noexcept { return StandardError{Kind::file, &file}; }
  StandardError StandardError::merge_with_stdout() noexcept { return StandardError{Kind::merge_stdout}; }

  ProcessSpec::ProcessSpec(Executable executable) : state_(vix::detail::allocation_or_terminate([&] { return std::make_unique<State>(std::move(executable)); })) {}
  ProcessSpec::ProcessSpec(const ProcessSpec &other) : state_(vix::detail::allocation_or_terminate([&] { return std::make_unique<State>(*other.state_); })) {}
  ProcessSpec::ProcessSpec(ProcessSpec &&) noexcept = default;
  ProcessSpec &ProcessSpec::operator=(const ProcessSpec &other)
  {
    if (this != &other) state_ = vix::detail::allocation_or_terminate([&] { return std::make_unique<State>(*other.state_); });
    return *this;
  }
  ProcessSpec &ProcessSpec::operator=(ProcessSpec &&) noexcept = default;
  ProcessSpec::~ProcessSpec() = default;
  Result<void> ProcessSpec::add_argument(std::string_view value)
  {
    if (value.find('\0') != std::string_view::npos || !valid_utf8(value)) return error(errors::invalid_argument, "invalid process argument");
    return vix::detail::allocation_or_terminate([&]() -> Result<void> { state_->arguments.emplace_back(value); return Result<void>::success(); });
  }
  void ProcessSpec::set_environment(Environment value) { state_->environment = std::move(value); }
  Result<void> ProcessSpec::set_working_directory(files::Path value)
  {
    if (value.empty()) return error(errors::invalid_working_directory, "working directory is empty");
    state_->working_directory = std::move(value); return Result<void>::success();
  }
  void ProcessSpec::set_stdin(StandardInput value) { state_->input = value; }
  void ProcessSpec::set_stdout(StandardOutput value) { state_->output = value; }
  void ProcessSpec::set_stderr(StandardError value) { state_->error = value; }

  ChildInput::ChildInput(int descriptor) noexcept : descriptor_(descriptor) {}
  ChildInput::ChildInput(ChildInput &&other) noexcept : descriptor_(std::exchange(other.descriptor_, -1)), close_error_(std::move(other.close_error_)) {}
  ChildInput::~ChildInput() noexcept { close_noexcept(descriptor_); }
  bool ChildInput::is_open() const noexcept { return descriptor_ >= 0; }
  Result<void> ChildInput::close() { if (!is_open() && !close_error_) contract_violation(); return close_cached(descriptor_, close_error_); }
  Result<std::size_t> ChildInput::write_some(std::span<const std::byte> buffer)
  {
    if (!is_open()) contract_violation();
    if (buffer.empty()) return std::size_t{0};
    sigset_t set, old, pending_before, pending_after;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    const bool mask_changed = ::pthread_sigmask(SIG_BLOCK, &set, &old) == 0;
    const bool was_blocked = mask_changed && sigismember(&old, SIGPIPE);
    const bool was_pending = mask_changed && ::sigpending(&pending_before) == 0 &&
                             sigismember(&pending_before, SIGPIPE);
    const ssize_t count = ::write(descriptor_, buffer.data(), buffer.size()); const int saved = errno;
    // Consume only the SIGPIPE generated by this write. sigwait is portable
    // across the supported POSIX hosts; sigtimedwait is not declared by all
    // macOS SDKs. Preserve any signal that was already pending for the caller.
    if (count < 0 && saved == EPIPE && mask_changed && !was_blocked && !was_pending &&
        ::sigpending(&pending_after) == 0 && sigismember(&pending_after, SIGPIPE))
    {
      int received_signal = 0;
      (void)::sigwait(&set, &received_signal);
    }
    if (mask_changed) (void)::pthread_sigmask(SIG_SETMASK, &old, nullptr);
    if (count < 0) return native_error(saved == EPIPE ? errors::broken_pipe : errors::spawn_failed, "pipe write", saved);
    if (count == 0) return error(errors::spawn_failed, "pipe made no progress");
    return static_cast<std::size_t>(count);
  }
  ChildOutput::ChildOutput(int descriptor) noexcept : descriptor_(descriptor) {}
  ChildOutput::ChildOutput(ChildOutput &&other) noexcept : descriptor_(std::exchange(other.descriptor_, -1)), close_error_(std::move(other.close_error_)) {}
  ChildOutput::~ChildOutput() noexcept { close_noexcept(descriptor_); }
  bool ChildOutput::is_open() const noexcept { return descriptor_ >= 0; }
  Result<void> ChildOutput::close() { if (!is_open() && !close_error_) contract_violation(); return close_cached(descriptor_, close_error_); }
  Result<std::size_t> ChildOutput::read_some(std::span<std::byte> buffer)
  {
    if (!is_open()) contract_violation();
    if (buffer.empty()) return std::size_t{0};
    const auto count = ::read(descriptor_, buffer.data(), buffer.size());
    if (count < 0) return native_error(errors::pipe_read_failed, "pipe read");
    return static_cast<std::size_t>(count);
  }

  WaitOptions::WaitOptions(CancellationToken cancellation, std::optional<Deadline> deadline) noexcept
    : cancellation_(std::move(cancellation)), deadline_(deadline) {}

  namespace
  {
    [[nodiscard]] Result<std::optional<ExitStatus>> probe(const std::shared_ptr<detail::ProcessRecord> &record)
    {
      std::lock_guard lock(record->mutex);
      if (record->status) return record->status;
      if (record->lost) return *record->lost;
      int status = 0; const auto result = ::waitpid(record->pid, &status, WNOHANG);
      if (result == 0) return std::optional<ExitStatus>{};
      if (result == record->pid)
      {
        record->status.emplace(WIFEXITED(status) ? detail_process_record_access::exited(static_cast<std::uint32_t>(WEXITSTATUS(status)))
                                                     : detail_process_record_access::abnormal());
        return record->status;
      }
      if (errno == ECHILD)
      {
        record->lost.emplace(error(errors::observation_lost, "child observation was lost"));
        return *record->lost;
      }
      if (errno == EINTR) return std::optional<ExitStatus>{};
      return native_error(errors::wait_failed, "waitpid");
    }
  }

  Process::Process(std::shared_ptr<detail::ProcessRecord> record, int input, int output, int error_fd) noexcept
    : record_(std::move(record)), stdin_descriptor_(input), stdout_descriptor_(output), stderr_descriptor_(error_fd) {}
  Process::Process(Process &&other) noexcept
    : record_(std::move(other.record_)), stdin_descriptor_(std::exchange(other.stdin_descriptor_, -1)),
      stdout_descriptor_(std::exchange(other.stdout_descriptor_, -1)), stderr_descriptor_(std::exchange(other.stderr_descriptor_, -1)) {}
  Process::~Process() noexcept
  {
    close_noexcept(stdin_descriptor_); close_noexcept(stdout_descriptor_); close_noexcept(stderr_descriptor_);
    if (!record_) return;
    bool finished; { std::lock_guard lock(record_->mutex); finished = record_->status || record_->lost; }
    if (finished) detail::Reaper::instance().remove(record_); else detail::Reaper::instance().abandon(record_);
  }
  ProcessId Process::id() const noexcept { if (!record_) contract_violation(); std::lock_guard lock(record_->mutex); return record_->id; }
  Result<ExitStatus> Process::wait()
  {
    if (!record_) contract_violation();
    for (;;)
    {
      auto status = probe(record_); if (!status) return status.error(); if (status.value()) return *status.value();
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }
  Completion<ExitStatus> Process::wait(WaitOptions options)
  {
    if (!record_) contract_violation();
    for (;;)
    {
      auto status = probe(record_); if (!status) return status.error(); if (status.value()) return *status.value();
      if (options.cancellation_.is_cancel_requested()) return Completion<ExitStatus>::cancelled();
      if (options.deadline_ && options.deadline_->reached()) return error(errors::wait_timed_out, "wait deadline reached");
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }
  Result<void> Process::terminate()
  {
    if (!record_) contract_violation();
    std::lock_guard lock(record_->mutex);
    if (record_->status) return error(errors::already_exited, "process already exited");
    // Once observation is irrecoverably lost, the native pid must never be
    // acted upon: it may already have been reused by an unrelated process.
    if (record_->lost) return *record_->lost;
    if (::kill(record_->pid, SIGKILL) == 0) return Result<void>::success();
    if (errno == ESRCH) return error(errors::already_exited, "process already exited");
    return native_error(errors::termination_failed, "terminate process");
  }
  Result<ChildInput> Process::take_stdin()
  {
    if (!record_) contract_violation();
    if (stdin_descriptor_ == -2) return error(errors::pipe_already_transferred, "stdin pipe already transferred");
    if (stdin_descriptor_ < 0) return error(errors::stdio_not_piped, "stdin is not piped");
    const int descriptor = std::exchange(stdin_descriptor_, -2); return ChildInput{descriptor};
  }
  Result<ChildOutput> Process::take_stdout()
  {
    if (!record_) contract_violation();
    if (stdout_descriptor_ == -2) return error(errors::pipe_already_transferred, "stdout pipe already transferred");
    if (stdout_descriptor_ < 0) return error(errors::stdio_not_piped, "stdout is not piped");
    const int descriptor = std::exchange(stdout_descriptor_, -2); return ChildOutput{descriptor};
  }
  Result<ChildOutput> Process::take_stderr()
  {
    if (!record_) contract_violation();
    if (stderr_descriptor_ == -2) return error(errors::pipe_already_transferred, "stderr pipe already transferred");
    if (stderr_descriptor_ < 0) return error(errors::stdio_not_piped, "stderr is not piped");
    const int descriptor = std::exchange(stderr_descriptor_, -2); return ChildOutput{descriptor};
  }

  // Spawn is intentionally kept POSIX-local. Preparation builds all
  // allocatable native state and installs the reaping record before spawning.
  Result<Process> spawn(ProcessSpec spec)
  {
#if !defined(__linux__) && !defined(__ANDROID__)
    // pipe()+fcntl() is the portable fallback where atomic pipe2(O_CLOEXEC)
    // is unavailable. Serialize the fork fallback; modern macOS uses
    // POSIX_SPAWN_CLOEXEC_DEFAULT to restrict inherited child descriptors.
    static std::mutex provider_spawn_mutex;
    std::unique_lock spawn_lock(provider_spawn_mutex, std::defer_lock);
#if defined(__APPLE__) && defined(POSIX_SPAWN_CLOEXEC_DEFAULT) && \
    defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101500
    bool cloexec_default_available = false;
    if (__builtin_available(macOS 10.15, *))
      cloexec_default_available = true;
    if (!cloexec_default_available)
      spawn_lock.lock();
#else
    spawn_lock.lock();
#endif
#endif
    auto &state = *spec.state_;
    static std::atomic<std::uint64_t> next_generation{1};
    const auto identity = detail_process_record_access::id(next_generation.fetch_add(1, std::memory_order_relaxed), 0);
    auto record = vix::detail::allocation_or_terminate([&] { return std::make_shared<detail::ProcessRecord>(identity); });
    if (auto prepared = detail::Reaper::instance().prepare(record); !prepared) return prepared.error();

    int in_pipe[2]{-1, -1}, out_pipe[2]{-1, -1}, err_pipe[2]{-1, -1}, report_pipe[2]{-1, -1};
    auto cleanup = [&]() noexcept { close_noexcept(in_pipe[0]); close_noexcept(in_pipe[1]); close_noexcept(out_pipe[0]); close_noexcept(out_pipe[1]); close_noexcept(err_pipe[0]); close_noexcept(err_pipe[1]); close_noexcept(report_pipe[0]); close_noexcept(report_pipe[1]); detail::Reaper::instance().remove(record); };
    if (state.input.kind_ == StandardInput::Kind::pipe && create_pipe_cloexec(in_pipe) != 0) { auto e=native_error(errors::spawn_failed,"pipe"); cleanup(); return e; }
    if (state.output.kind_ == StandardOutput::Kind::pipe && create_pipe_cloexec(out_pipe) != 0) { auto e=native_error(errors::spawn_failed,"pipe"); cleanup(); return e; }
    if (state.error.kind_ == StandardError::Kind::pipe && create_pipe_cloexec(err_pipe) != 0) { auto e=native_error(errors::spawn_failed,"pipe"); cleanup(); return e; }
    if (create_pipe_cloexec(report_pipe) != 0) { auto e=native_error(errors::spawn_failed,"pipe"); cleanup(); return e; }

    auto validate_file = [](files::File *file, bool read) -> int
    { if (!file || !file->is_open()) contract_violation(); const auto access=file->access(); if (read ? !(access==files::FileAccess::read||access==files::FileAccess::read_write) : !(access==files::FileAccess::write||access==files::FileAccess::read_write)) contract_violation(); return files::detail_duplicate_native_handle(*file); };
    int in_file=-1, out_file=-1, err_file=-1, null_read=-1, null_write=-1;
    if (state.input.kind_ == StandardInput::Kind::file) { in_file=validate_file(state.input.file_,true); if(in_file<0){auto e=native_error(errors::invalid_stdio,"duplicate stdin file");cleanup();return e;} }
    if (state.output.kind_ == StandardOutput::Kind::file) { out_file=validate_file(state.output.file_,false); if(out_file<0){auto e=native_error(errors::invalid_stdio,"duplicate stdout file");close_noexcept(in_file);cleanup();return e;} }
    if (state.error.kind_ == StandardError::Kind::file) { err_file=validate_file(state.error.file_,false); if(err_file<0){auto e=native_error(errors::invalid_stdio,"duplicate stderr file");close_noexcept(in_file);close_noexcept(out_file);cleanup();return e;} }
    if (state.input.kind_ == StandardInput::Kind::null) { null_read=::open("/dev/null",O_RDONLY|O_CLOEXEC); if(null_read<0){auto e=native_error(errors::spawn_failed,"open null");close_noexcept(in_file);close_noexcept(out_file);close_noexcept(err_file);cleanup();return e;} }
    if (state.output.kind_ == StandardOutput::Kind::null || state.error.kind_ == StandardError::Kind::null) { null_write=::open("/dev/null",O_WRONLY|O_CLOEXEC); if(null_write<0){auto e=native_error(errors::spawn_failed,"open null");close_noexcept(in_file);close_noexcept(out_file);close_noexcept(err_file);close_noexcept(null_read);cleanup();return e;} }

    // Environment is fully materialized before fork; native inherited bytes survive untouched.
    std::vector<std::string> environment;
    vix::detail::allocation_or_terminate([&]
    {
      if (state.environment.state_->inherited) for (char **p=environ; p && *p; ++p) environment.emplace_back(*p);
      for (const auto &change : state.environment.state_->changes)
      {
        const auto prefix=change.name+"=";
        environment.erase(std::remove_if(environment.begin(),environment.end(),[&](const std::string &entry){return entry.rfind(prefix,0)==0;}),environment.end());
        if(change.value) environment.push_back(prefix+*change.value);
      }
    });
    std::vector<char *> environment_ptrs; vix::detail::allocation_or_terminate([&] { for(auto &entry:environment) environment_ptrs.push_back(entry.data()); environment_ptrs.push_back(nullptr); });

    std::string executable;
    if (state.executable.state_->search)
    {
      std::string path_value;
      for (const auto &entry : environment) if (entry.rfind("PATH=",0)==0) { path_value=entry.substr(5); break; }
      if (path_value.empty()) { close_noexcept(in_file);close_noexcept(out_file);close_noexcept(err_file);close_noexcept(null_read);close_noexcept(null_write);cleanup();return error(errors::executable_not_found,"PATH is empty"); }
      std::string base; if(state.working_directory) base=files::detail_native_path(*state.working_directory); else { char cwd[4096]; if(!::getcwd(cwd,sizeof(cwd))){close_noexcept(in_file);close_noexcept(out_file);close_noexcept(err_file);close_noexcept(null_read);close_noexcept(null_write);cleanup();return native_error(errors::spawn_failed,"getcwd");} base=cwd; }
      bool found=false; for(std::size_t offset=0; offset<=path_value.size();) { const auto next=path_value.find(':',offset); auto part=path_value.substr(offset,next==std::string::npos?std::string::npos:next-offset); std::string candidate=(part.empty()||part[0]!='/')?base+"/"+part+"/"+state.executable.state_->name:part+"/"+state.executable.state_->name; if(::access(candidate.c_str(),X_OK)==0){executable=std::move(candidate);found=true;break;} if(next==std::string::npos)break; offset=next+1; }
      if(!found){close_noexcept(in_file);close_noexcept(out_file);close_noexcept(err_file);close_noexcept(null_read);close_noexcept(null_write);cleanup();return error(errors::executable_not_found,"PATH executable not found");}
    }
    else executable=files::detail_native_path(state.executable.state_->path);
    std::vector<std::string> argv_storage; vix::detail::allocation_or_terminate([&] { argv_storage.push_back(executable); for(const auto &argument:state.arguments) argv_storage.push_back(argument); });
    std::vector<char *> argv; vix::detail::allocation_or_terminate([&] { for(auto &argument:argv_storage) argv.push_back(argument.data()); argv.push_back(nullptr); });

    pid_t child = -1;
#if defined(__APPLE__) && defined(POSIX_SPAWN_CLOEXEC_DEFAULT) && \
    defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101500
    // macOS does not provide pipe2 on all supported SDKs. The fcntl fallback
    // above is paired with CLOEXEC_DEFAULT here so only explicitly mapped
    // standard streams enter the child process.
    if (__builtin_available(macOS 10.15, *))
    {
      posix_spawn_file_actions_t actions;
      posix_spawnattr_t attributes;
      int spawn_error = ::posix_spawn_file_actions_init(&actions);
      bool actions_initialized = spawn_error == 0;
      bool attributes_initialized = false;
      if (spawn_error == 0)
      {
        spawn_error = ::posix_spawnattr_init(&attributes);
        attributes_initialized = spawn_error == 0;
      }
      if (spawn_error == 0 && state.working_directory)
        spawn_error = ::posix_spawn_file_actions_addchdir_np(
          &actions, files::detail_native_path(*state.working_directory).c_str());
      auto add_stdio = [&](int source, int destination)
      {
        if (spawn_error == 0)
          spawn_error = ::posix_spawn_file_actions_adddup2(&actions, source, destination);
      };
      const int child_stdin = state.input.kind_ == StandardInput::Kind::pipe ? in_pipe[0]
        : state.input.kind_ == StandardInput::Kind::null ? null_read
        : state.input.kind_ == StandardInput::Kind::file ? in_file : STDIN_FILENO;
      const int child_stdout = state.output.kind_ == StandardOutput::Kind::pipe ? out_pipe[1]
        : state.output.kind_ == StandardOutput::Kind::null ? null_write
        : state.output.kind_ == StandardOutput::Kind::file ? out_file : STDOUT_FILENO;
      const int child_stderr = state.error.kind_ == StandardError::Kind::pipe ? err_pipe[1]
        : state.error.kind_ == StandardError::Kind::null ? null_write
        : state.error.kind_ == StandardError::Kind::file ? err_file
        : state.error.kind_ == StandardError::Kind::merge_stdout ? child_stdout : STDERR_FILENO;
      if (spawn_error == 0) add_stdio(child_stdin, STDIN_FILENO);
      if (spawn_error == 0) add_stdio(child_stdout, STDOUT_FILENO);
      if (spawn_error == 0) add_stdio(child_stderr, STDERR_FILENO);
      if (spawn_error == 0)
      {
        short flags = POSIX_SPAWN_CLOEXEC_DEFAULT;
        spawn_error = ::posix_spawnattr_setflags(&attributes, flags);
      }
      if (spawn_error == 0)
        spawn_error = ::posix_spawn(&child, executable.c_str(), &actions, &attributes,
                                    argv.data(), environment_ptrs.data());
      if (attributes_initialized) (void)::posix_spawnattr_destroy(&attributes);
      if (actions_initialized) (void)::posix_spawn_file_actions_destroy(&actions);

      if (spawn_error != 0)
      {
        close_noexcept(in_file); close_noexcept(out_file); close_noexcept(err_file);
        close_noexcept(null_read); close_noexcept(null_write);
        cleanup();
        if (state.working_directory)
        {
          struct stat working_directory_info{};
          const auto &working_directory = files::detail_native_path(*state.working_directory);
          if (::stat(working_directory.c_str(), &working_directory_info) != 0)
            return native_error(errors::invalid_working_directory, "posix_spawn working directory", errno);
          if (!S_ISDIR(working_directory_info.st_mode))
            return native_error(errors::invalid_working_directory, "posix_spawn working directory", ENOTDIR);
        }
        return native_error(errors::spawn_failed, "posix_spawn", spawn_error);
      }

      close_noexcept(report_pipe[0]); close_noexcept(report_pipe[1]);
      close_noexcept(in_pipe[0]); close_noexcept(out_pipe[1]); close_noexcept(err_pipe[1]);
      close_noexcept(in_file); close_noexcept(out_file); close_noexcept(err_file);
      close_noexcept(null_read); close_noexcept(null_write);
      {
        std::lock_guard lock(record->mutex);
        record->pid = child;
        detail_process_record_access::set_native(record->id, static_cast<std::uint64_t>(child));
      }
      return Process{std::move(record), in_pipe[1], out_pipe[0], err_pipe[0]};
    }
#endif
    child=::fork();
    if(child<0){auto e=native_error(errors::spawn_failed,"fork");close_noexcept(in_file);close_noexcept(out_file);close_noexcept(err_file);close_noexcept(null_read);close_noexcept(null_write);cleanup();return e;}
    if(child==0)
    {
      close_noexcept(report_pipe[0]);
      auto fail=[&](int stage) noexcept
      {
        ExecFailure failure{stage,errno};
        const auto *bytes=reinterpret_cast<const char *>(&failure);
        std::size_t written=0;
        while(written<sizeof(failure))
        {
          const auto count=::write(report_pipe[1],bytes+written,sizeof(failure)-written);
          if(count>0){written+=static_cast<std::size_t>(count);continue;}
          if(count<0&&errno==EINTR)continue;
          break;
        }
        _exit(127);
      };
      if(state.working_directory && ::chdir(files::detail_native_path(*state.working_directory).c_str())!=0) fail(cwd_stage);
      auto attach=[&](int source,int target) noexcept { if(source>=0 && source!=target && ::dup2(source,target)<0) fail(stdio_stage); };
      if(state.input.kind_==StandardInput::Kind::pipe) attach(in_pipe[0],STDIN_FILENO); else if(state.input.kind_==StandardInput::Kind::null) attach(null_read,STDIN_FILENO); else if(state.input.kind_==StandardInput::Kind::file) attach(in_file,STDIN_FILENO);
      if(state.output.kind_==StandardOutput::Kind::pipe) attach(out_pipe[1],STDOUT_FILENO); else if(state.output.kind_==StandardOutput::Kind::null) attach(null_write,STDOUT_FILENO); else if(state.output.kind_==StandardOutput::Kind::file) attach(out_file,STDOUT_FILENO);
      if(state.error.kind_==StandardError::Kind::merge_stdout) attach(STDOUT_FILENO,STDERR_FILENO); else if(state.error.kind_==StandardError::Kind::pipe) attach(err_pipe[1],STDERR_FILENO); else if(state.error.kind_==StandardError::Kind::null) attach(null_write,STDERR_FILENO); else if(state.error.kind_==StandardError::Kind::file) attach(err_file,STDERR_FILENO);
      close_noexcept(in_pipe[0]);close_noexcept(in_pipe[1]);close_noexcept(out_pipe[0]);close_noexcept(out_pipe[1]);close_noexcept(err_pipe[0]);close_noexcept(err_pipe[1]);close_noexcept(in_file);close_noexcept(out_file);close_noexcept(err_file);close_noexcept(null_read);close_noexcept(null_write);
      ::execve(executable.c_str(),argv.data(),environment_ptrs.data()); fail(exec_stage);
    }
    close_noexcept(report_pipe[1]); close_noexcept(in_pipe[0]);close_noexcept(out_pipe[1]);close_noexcept(err_pipe[1]);close_noexcept(in_file);close_noexcept(out_file);close_noexcept(err_file);close_noexcept(null_read);close_noexcept(null_write);
    ExecFailure failure{};
    std::size_t report_bytes=0;
    int report_error=0;
    while(report_bytes<sizeof(failure))
    {
      const auto count=::read(report_pipe[0],reinterpret_cast<char *>(&failure)+report_bytes,sizeof(failure)-report_bytes);
      if(count>0){report_bytes+=static_cast<std::size_t>(count);continue;}
      if(count==0)break;
      if(errno==EINTR)continue;
      report_error=errno;break;
    }
    close_noexcept(report_pipe[0]);
    if(report_error!=0 || (report_bytes!=0 && report_bytes!=sizeof(failure)))
    {
      int ignored=0;
      while(::waitpid(child,&ignored,0)<0&&errno==EINTR){}
      cleanup();
      return report_error!=0 ? native_error(errors::spawn_failed,"read child spawn status",report_error)
                             : error(errors::spawn_failed,"child spawn status was incomplete");
    }
    if(report_bytes==sizeof(failure))
    {
      int ignored=0;
      while(::waitpid(child,&ignored,0)<0&&errno==EINTR){}
      cleanup();
      return failure.stage==cwd_stage ? native_error(errors::invalid_working_directory,"chdir",failure.error_number)
                                      : native_error(errors::spawn_failed,"exec",failure.error_number);
    }
    { std::lock_guard lock(record->mutex); record->pid=child; detail_process_record_access::set_native(record->id, static_cast<std::uint64_t>(child)); }
    return Process{std::move(record),in_pipe[1],out_pipe[0],err_pipe[0]};
  }
}
