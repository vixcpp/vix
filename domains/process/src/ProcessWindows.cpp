#include <vix/process/ProcessDomain.hpp>

#include <vix/detail/AllocationFailure.hpp>
#include <vix/files/detail/NativePath.hpp>
#include <vix/files/detail/ProcessBridge.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <fcntl.h>
#include <io.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <climits>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vix::process
{
  struct detail_process_record_access
  {
    static ProcessId id(std::uint64_t generation, std::uint64_t native) noexcept { return ProcessId{generation, native}; }
    static ExitStatus exited(std::uint32_t code) noexcept { return ExitStatus{ExitKind::exited, code}; }
    static ExitStatus abnormal() noexcept { return ExitStatus{ExitKind::abnormal, std::nullopt}; }
    static void set_native(ProcessId &value, std::uint64_t native) noexcept { value.native_ = native; }
  };

  namespace
  {
    [[noreturn]] void contract_violation() noexcept { std::terminate(); }
    [[nodiscard]] Error error(ErrorIdentity identity, std::string_view message)
    { return vix::detail::allocation_or_terminate([&] { return Error{identity, std::string(message)}; }); }

    [[nodiscard]] std::optional<std::wstring> wide(std::string_view value)
    {
      if (value.size() > static_cast<std::size_t>(INT_MAX)) return std::nullopt;
      if (value.empty()) return std::wstring{};
      const auto length = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                                 static_cast<int>(value.size()), nullptr, 0);
      if (length <= 0) return std::nullopt;
      auto result = vix::detail::allocation_or_terminate([&] { return std::wstring(static_cast<std::size_t>(length), L'\0'); });
      if (::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                                result.data(), length) != length) return std::nullopt;
      return result;
    }

    [[nodiscard]] bool valid_utf8(std::string_view value) noexcept
    {
      for (std::size_t i = 0; i < value.size();)
      {
        const auto lead = static_cast<unsigned char>(value[i]);
        std::size_t count = 0;
        std::uint32_t code = 0;
        if (lead <= 0x7f) { ++i; continue; }
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

    [[nodiscard]] Error win_error(ErrorIdentity fallback, const char *operation, DWORD code = ::GetLastError())
    {
      ErrorIdentity identity = fallback;
      switch (code)
      {
      case ERROR_FILE_NOT_FOUND: case ERROR_PATH_NOT_FOUND: identity = fallback == errors::spawn_failed ? errors::executable_not_found : fallback; break;
      case ERROR_ACCESS_DENIED: case ERROR_PRIVILEGE_NOT_HELD: identity = errors::access_denied; break;
      case ERROR_TOO_MANY_OPEN_FILES: identity = errors::resource_exhausted; break;
      case ERROR_BAD_EXE_FORMAT: identity = errors::not_executable; break;
      case ERROR_BROKEN_PIPE: case ERROR_NO_DATA: identity = errors::broken_pipe; break;
      default: break;
      }
      return vix::detail::allocation_or_terminate([&]
      { return Error{identity, std::string(operation) + ": Windows error " + std::to_string(code)}; });
    }

    [[nodiscard]] Error crt_error(ErrorIdentity fallback, const char *operation, int code = errno)
    {
      ErrorIdentity identity = fallback;
      if (code == EACCES || code == EPERM) identity = errors::access_denied;
      else if (code == EMFILE || code == ENFILE) identity = errors::resource_exhausted;
      else if (code == EPIPE) identity = errors::broken_pipe;
      return vix::detail::allocation_or_terminate([&]
      { return Error{identity, std::string(operation) + ": " + std::strerror(code)}; });
    }

    void close_fd(int &fd) noexcept { if (fd >= 0) { const int old = std::exchange(fd, -1); (void)::_close(old); } }
    void close_handle(HANDLE &handle) noexcept
    { if (handle && handle != INVALID_HANDLE_VALUE) { HANDLE old = std::exchange(handle, nullptr); (void)::CloseHandle(old); } }

    [[nodiscard]] bool same_name(std::wstring_view left, std::wstring_view right) noexcept
    {
      return left.size() <= static_cast<std::size_t>(INT_MAX) && right.size() <= static_cast<std::size_t>(INT_MAX) &&
        ::CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
    }

    struct EnvLess
    {
      bool operator()(const std::wstring &left, const std::wstring &right) const noexcept
      {
        const int result = ::CompareStringOrdinal(left.data(), static_cast<int>(left.size()),
                                                   right.data(), static_cast<int>(right.size()), TRUE);
        return result == CSTR_LESS_THAN || (result == CSTR_EQUAL && left < right);
      }
    };

    using EnvMap = std::map<std::wstring, std::wstring, EnvLess>;

    [[nodiscard]] std::optional<EnvMap> current_environment()
    {
      EnvMap values;
      LPWCH block = ::GetEnvironmentStringsW();
      if (!block) return std::nullopt;
      for (const wchar_t *entry = block; *entry; entry += std::wcslen(entry) + 1)
      {
        const wchar_t *separator = std::wcschr(entry + (entry[0] == L'=' ? 1 : 0), L'=');
        if (!separator) continue;
        values[std::wstring(entry, separator)] = separator + 1;
      }
      ::FreeEnvironmentStringsW(block);
      return values;
    }

    [[nodiscard]] std::wstring quote_argument(std::wstring_view arg)
    {
      // Encode one argv item using the Microsoft CRT parsing convention.
      // CreateProcessW receives a command line, not an argv array; child
      // programs with a custom parser may intentionally interpret it differently.
      if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring_view::npos) return std::wstring(arg);
      std::wstring out(1, L'"');
      std::size_t slashes = 0;
      for (wchar_t ch : arg)
      {
        if (ch == L'\\') { ++slashes; continue; }
        if (ch == L'"')
        {
          out.append(slashes * 2 + 1, L'\\');
          out.push_back(L'"');
        }
        else
        {
          out.append(slashes, L'\\');
          out.push_back(ch);
        }
        slashes = 0;
      }
      out.append(slashes * 2, L'\\');
      out.push_back(L'"');
      return out;
    }

    [[nodiscard]] std::wstring full_path(std::wstring_view path)
    {
      const std::wstring input(path);
      const DWORD needed = ::GetFullPathNameW(input.c_str(), 0, nullptr, nullptr);
      if (needed == 0) return {};
      auto result = vix::detail::allocation_or_terminate([&] { return std::wstring(static_cast<std::size_t>(needed), L'\0'); });
      const DWORD written = ::GetFullPathNameW(input.c_str(), needed, result.data(), nullptr);
      if (written == 0 || written >= needed) return {};
      result.resize(written);
      return result;
    }

    [[nodiscard]] bool regular_executable_candidate(const std::wstring &candidate) noexcept
    {
      const DWORD attributes = ::GetFileAttributesW(candidate.c_str());
      return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    [[nodiscard]] std::optional<std::wstring> resolve_search_name(std::wstring name, const EnvMap &environment,
                                                                   const std::optional<std::wstring> &cwd)
    {
      // Search uses the effective child environment (including PATH/PATHEXT
      // overlays), with Windows' conventional COM/EXE defaults when PATHEXT
      // is absent. Batch files are not shell-launched by this provider.
      std::wstring path_value;
      std::wstring extensions = L".COM;.EXE";
      for (const auto &[key, value] : environment)
      {
        if (same_name(key, L"PATH")) path_value = value;
        else if (same_name(key, L"PATHEXT") && !value.empty()) extensions = value;
      }
      if (path_value.empty()) return std::nullopt;
      std::vector<std::wstring> suffixes;
      if (std::filesystem::path(name).has_extension()) suffixes.emplace_back();
      else
      {
        std::size_t start = 0;
        while (start <= extensions.size())
        {
          const auto end = extensions.find(L';', start);
          auto suffix = extensions.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
          if (!suffix.empty()) suffixes.push_back(std::move(suffix));
          if (end == std::wstring::npos) break;
          start = end + 1;
        }
      }
      std::size_t start = 0;
      while (start <= path_value.size())
      {
        const auto end = path_value.find(L';', start);
        auto directory = path_value.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
        if (directory.empty()) directory = cwd.value_or(L".");
        else if (std::filesystem::path(directory).is_relative())
        {
          const std::wstring base=cwd ? *cwd : std::wstring(L".");
          directory = full_path(base + L"\\" + directory);
        }
        for (const auto &suffix : suffixes)
        {
          auto candidate = directory;
          if (!candidate.empty() && candidate.back() != L'\\' && candidate.back() != L'/') candidate.push_back(L'\\');
          candidate += name;
          candidate += suffix;
          if (regular_executable_candidate(candidate)) return full_path(candidate);
        }
        if (end == std::wstring::npos) break;
        start = end + 1;
      }
      return std::nullopt;
    }

    [[nodiscard]] Result<int> pipe_fd(HANDLE handle, int flags)
    {
      const int fd = ::_open_osfhandle(reinterpret_cast<std::intptr_t>(handle), flags | _O_BINARY | _O_NOINHERIT);
      if (fd < 0)
      {
        const auto failure = crt_error(errors::spawn_failed, "_open_osfhandle");
        (void)::CloseHandle(handle);
        return failure;
      }
      return fd;
    }
  }

  struct Executable::State
  {
    bool search{};
    files::Path path;
    std::string name;
    State(bool find, files::Path value, std::string executable_name)
      : search(find), path(std::move(value)), name(std::move(executable_name)) {}
  };
  struct Environment::State
  {
    struct Change { std::string name; std::optional<std::string> value; };
    bool inherited{};
    std::vector<Change> changes;
    explicit State(bool inherit) : inherited(inherit) {}
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
    explicit State(Executable value) : executable(std::move(value)), environment(Environment::inherit()) {}
  };

  namespace detail
  {
    struct ProcessRecord
    {
      explicit ProcessRecord(ProcessId process_id) noexcept : id(std::move(process_id)) {}
      ~ProcessRecord() { if (handle) (void)::CloseHandle(handle); }
      mutable std::mutex mutex;
      HANDLE handle{};
      ProcessId id;
      std::optional<ExitStatus> status;
      std::optional<Error> lost;
    };
  }

  std::string ProcessId::display() const
  { return vix::detail::allocation_or_terminate([&] { return std::to_string(generation_) + ":" + std::to_string(native_); }); }
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
      auto requested_name=wide(name);
      if(!requested_name)return error(errors::invalid_environment,"environment name cannot be converted to UTF-16");
      auto state = std::make_shared<State>(*state_);
      for(auto &change:state->changes)
      {
        auto old_name=wide(change.name);
        if(old_name && same_name(*old_name,*requested_name)){change.value=std::string(value);return Environment{std::move(state)};}
      }
      state->changes.push_back({std::string(name), std::string(value)});
      return Environment{std::move(state)};
    });
  }
  Result<Environment> Environment::without(std::string_view name) const
  {
    if (name.empty() || name.find('=') != std::string_view::npos || name.find('\0') != std::string_view::npos || !valid_utf8(name))
      return error(errors::invalid_environment, "invalid environment variable name");
    return vix::detail::allocation_or_terminate([&]() -> Result<Environment>
    {
      auto requested_name=wide(name);
      if(!requested_name)return error(errors::invalid_environment,"environment name cannot be converted to UTF-16");
      auto state = std::make_shared<State>(*state_);
      for(auto &change:state->changes)
      {
        auto old_name=wide(change.name);
        if(old_name && same_name(*old_name,*requested_name)){change.value.reset();return Environment{std::move(state)};}
      }
      state->changes.push_back({std::string(name), std::nullopt});
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
  { if (this != &other) state_ = vix::detail::allocation_or_terminate([&] { return std::make_unique<State>(*other.state_); }); return *this; }
  ProcessSpec &ProcessSpec::operator=(ProcessSpec &&) noexcept = default;
  ProcessSpec::~ProcessSpec() = default;
  Result<void> ProcessSpec::add_argument(std::string_view value)
  {
    if (value.find('\0') != std::string_view::npos || !valid_utf8(value)) return error(errors::invalid_argument, "invalid process argument");
    return vix::detail::allocation_or_terminate([&]() -> Result<void> { state_->arguments.emplace_back(value); return Result<void>::success(); });
  }
  void ProcessSpec::set_environment(Environment value) { state_->environment = std::move(value); }
  Result<void> ProcessSpec::set_working_directory(files::Path value)
  { if (value.empty()) return error(errors::invalid_working_directory, "working directory is empty"); state_->working_directory = std::move(value); return Result<void>::success(); }
  void ProcessSpec::set_stdin(StandardInput value) { state_->input = value; }
  void ProcessSpec::set_stdout(StandardOutput value) { state_->output = value; }
  void ProcessSpec::set_stderr(StandardError value) { state_->error = value; }

  ChildInput::ChildInput(int descriptor) noexcept : descriptor_(descriptor) {}
  ChildInput::ChildInput(ChildInput &&other) noexcept : descriptor_(std::exchange(other.descriptor_, -1)), close_error_(std::move(other.close_error_)) {}
  ChildInput::~ChildInput() noexcept { close_fd(descriptor_); }
  bool ChildInput::is_open() const noexcept { return descriptor_ >= 0; }
  Result<void> ChildInput::close()
  { if (descriptor_ < 0) { if (close_error_) return *close_error_; contract_violation(); } const int fd=std::exchange(descriptor_,-1); if(::_close(fd)==0)return Result<void>::success(); close_error_.emplace(crt_error(errors::close_failed,"close pipe"));return *close_error_; }
  Result<std::size_t> ChildInput::write_some(std::span<const std::byte> buffer)
  { if(!is_open())contract_violation(); if(buffer.empty())return std::size_t{0}; const int count=::_write(descriptor_,buffer.data(),static_cast<unsigned>(std::min<std::size_t>(buffer.size(),UINT_MAX))); if(count<0)return crt_error(errno==EPIPE?errors::broken_pipe:errors::spawn_failed,"pipe write"); if(count==0)return error(errors::spawn_failed,"pipe made no progress"); return static_cast<std::size_t>(count); }
  ChildOutput::ChildOutput(int descriptor) noexcept : descriptor_(descriptor) {}
  ChildOutput::ChildOutput(ChildOutput &&other) noexcept : descriptor_(std::exchange(other.descriptor_, -1)), close_error_(std::move(other.close_error_)) {}
  ChildOutput::~ChildOutput() noexcept { close_fd(descriptor_); }
  bool ChildOutput::is_open() const noexcept { return descriptor_ >= 0; }
  Result<void> ChildOutput::close()
  { if (descriptor_ < 0) { if (close_error_) return *close_error_; contract_violation(); } const int fd=std::exchange(descriptor_,-1); if(::_close(fd)==0)return Result<void>::success(); close_error_.emplace(crt_error(errors::close_failed,"close pipe"));return *close_error_; }
  Result<std::size_t> ChildOutput::read_some(std::span<std::byte> buffer)
  { if(!is_open())contract_violation(); if(buffer.empty())return std::size_t{0}; const int count=::_read(descriptor_,buffer.data(),static_cast<unsigned>(std::min<std::size_t>(buffer.size(),UINT_MAX))); if(count<0)return crt_error(errors::pipe_read_failed,"pipe read"); return static_cast<std::size_t>(count); }

  WaitOptions::WaitOptions(CancellationToken cancellation, std::optional<Deadline> deadline) noexcept
    : cancellation_(std::move(cancellation)), deadline_(deadline) {}

  Process::Process(std::shared_ptr<detail::ProcessRecord> record, int input, int output, int error_fd) noexcept
    : record_(std::move(record)), stdin_descriptor_(input), stdout_descriptor_(output), stderr_descriptor_(error_fd) {}
  Process::Process(Process &&other) noexcept
    : record_(std::move(other.record_)), stdin_descriptor_(std::exchange(other.stdin_descriptor_,-1)),
      stdout_descriptor_(std::exchange(other.stdout_descriptor_,-1)), stderr_descriptor_(std::exchange(other.stderr_descriptor_,-1)) {}
  Process::~Process() noexcept { close_fd(stdin_descriptor_); close_fd(stdout_descriptor_); close_fd(stderr_descriptor_); }
  ProcessId Process::id() const noexcept { if(!record_)contract_violation(); return record_->id; }
  namespace
  {
    [[nodiscard]] Result<std::optional<ExitStatus>> observe(const std::shared_ptr<detail::ProcessRecord> &record)
    {
      std::lock_guard lock(record->mutex);
      if(record->status)return record->status;
      if(record->lost)return *record->lost;
      const DWORD state=::WaitForSingleObject(record->handle,0);
      if(state==WAIT_TIMEOUT)return std::optional<ExitStatus>{};
      if(state!=WAIT_OBJECT_0)return win_error(errors::wait_failed,"WaitForSingleObject");
      DWORD code=0;
      if(!::GetExitCodeProcess(record->handle,&code))return win_error(errors::wait_failed,"GetExitCodeProcess");
      record->status.emplace(detail_process_record_access::exited(code));
      return record->status;
    }
    [[nodiscard]] DWORD wait_slice(const std::optional<Deadline> &deadline)
    {
      if(!deadline)return 20;
      const auto remaining=deadline->remaining();
      const auto nanos=remaining.nanosecond_count();
      if(nanos<=0)return 0;
      const auto millis=static_cast<std::uint64_t>(nanos/1000000 + (nanos%1000000 != 0 ? 1 : 0));
      return static_cast<DWORD>(std::min<std::uint64_t>(millis,20));
    }
  }
  Result<ExitStatus> Process::wait()
  {
    if(!record_)contract_violation();
    for(;;){auto value=observe(record_);if(!value)return value.error();if(value.value())return *value.value();(void)::WaitForSingleObject(record_->handle,20);}
  }
  Completion<ExitStatus> Process::wait(WaitOptions options)
  {
    if(!record_)contract_violation();
    for(;;)
    {
      auto value=observe(record_);if(!value)return value.error();if(value.value())return *value.value();
      if(options.cancellation_.is_cancel_requested())return Completion<ExitStatus>::cancelled();
      if(options.deadline_ && options.deadline_->reached())return error(errors::wait_timed_out,"wait deadline reached");
      (void)::WaitForSingleObject(record_->handle,wait_slice(options.deadline_));
    }
  }
  Result<void> Process::terminate()
  {
    if(!record_)contract_violation();
    { auto value=observe(record_);if(!value)return value.error();if(value.value())return error(errors::already_exited,"process already exited"); }
    if(::TerminateProcess(record_->handle,1))return Result<void>::success();
    const DWORD code=::GetLastError();
    auto after=observe(record_);
    if(after && after.value())return error(errors::already_exited,"process already exited");
    return win_error(errors::termination_failed,"TerminateProcess",code);
  }
  Result<ChildInput> Process::take_stdin()
  { if(!record_)contract_violation();if(stdin_descriptor_==-2)return error(errors::pipe_already_transferred,"stdin pipe already transferred");if(stdin_descriptor_<0)return error(errors::stdio_not_piped,"stdin is not piped");return ChildInput{std::exchange(stdin_descriptor_,-2)}; }
  Result<ChildOutput> Process::take_stdout()
  { if(!record_)contract_violation();if(stdout_descriptor_==-2)return error(errors::pipe_already_transferred,"stdout pipe already transferred");if(stdout_descriptor_<0)return error(errors::stdio_not_piped,"stdout is not piped");return ChildOutput{std::exchange(stdout_descriptor_,-2)}; }
  Result<ChildOutput> Process::take_stderr()
  { if(!record_)contract_violation();if(stderr_descriptor_==-2)return error(errors::pipe_already_transferred,"stderr pipe already transferred");if(stderr_descriptor_<0)return error(errors::stdio_not_piped,"stderr is not piped");return ChildOutput{std::exchange(stderr_descriptor_,-2)}; }

  Result<Process> spawn(ProcessSpec spec)
  {
    auto &state=*spec.state_;
    return vix::detail::allocation_or_terminate([&]() -> Result<Process>
    {
      static std::atomic<std::uint64_t> generation{1};
      auto record=vix::detail::allocation_or_terminate([&]
      { return std::make_shared<detail::ProcessRecord>(detail_process_record_access::id(generation.fetch_add(1,std::memory_order_relaxed),0)); });
      EnvMap environment;
      if(state.environment.state_->inherited)
      {
        auto inherited=current_environment();
        if(!inherited)return win_error(errors::spawn_failed,"GetEnvironmentStringsW");
        environment=std::move(*inherited);
      }
      for(const auto &change:state.environment.state_->changes)
      {
        auto key=wide(change.name);if(!key)return error(errors::invalid_environment,"environment name is not valid UTF-8");
        for(auto it=environment.begin();it!=environment.end();)if(same_name(it->first,*key))it=environment.erase(it);else ++it;
        if(change.value){auto value=wide(*change.value);if(!value)return error(errors::invalid_environment,"environment value is not valid UTF-8");environment.emplace(std::move(*key),std::move(*value));}
      }
      std::optional<std::wstring> cwd;
      if(state.working_directory)
      {
        auto converted=wide(files::detail_native_path(*state.working_directory));
        if(!converted)return error(errors::invalid_working_directory,"working directory is not valid UTF-8");
        cwd=full_path(*converted);
        if(cwd->empty())return win_error(errors::invalid_working_directory,"resolve working directory");
        const DWORD attributes=::GetFileAttributesW(cwd->c_str());
        if(attributes==INVALID_FILE_ATTRIBUTES || !(attributes&FILE_ATTRIBUTE_DIRECTORY))
          return win_error(errors::invalid_working_directory,"validate working directory",attributes==INVALID_FILE_ATTRIBUTES?::GetLastError():ERROR_DIRECTORY);
      }
      std::optional<std::wstring> executable;
      if(state.executable.state_->search)
      {
        auto name=wide(state.executable.state_->name);
        if(!name)return error(errors::invalid_argument,"executable name is not valid UTF-8");
        executable=resolve_search_name(std::move(*name),environment,cwd);
        if(!executable)return error(errors::executable_not_found,"PATH executable not found");
      }
      else
      {
        auto path=wide(files::detail_native_path(state.executable.state_->path));
        if(!path)return error(errors::invalid_argument,"executable path is not valid UTF-8");
        if(std::filesystem::path(*path).is_absolute())executable=*path;
        else
        {
          const std::wstring base=cwd ? *cwd : std::wstring(L".");
          executable=full_path(base+L"\\"+*path);
        }
        if(executable->empty())return win_error(errors::spawn_failed,"resolve executable path");
      }
      std::wstring command=quote_argument(*executable);
      for(const auto &argument:state.arguments)
      {
        auto value=wide(argument);if(!value)return error(errors::invalid_argument,"argument is not valid UTF-8");
        command.push_back(L' ');command+=quote_argument(*value);
      }
      if(command.size()>=32767)return error(errors::invalid_argument,"Windows process command line exceeds the platform limit");
      command.push_back(L'\0');

      std::vector<wchar_t> env_block;
      for(const auto &[name,value]:environment){env_block.insert(env_block.end(),name.begin(),name.end());env_block.push_back(L'=');env_block.insert(env_block.end(),value.begin(),value.end());env_block.push_back(L'\0');}
      env_block.push_back(L'\0');if(env_block.size()==1)env_block.push_back(L'\0');

      SECURITY_ATTRIBUTES inheritable{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
      HANDLE in_child=nullptr,in_parent=nullptr,out_parent=nullptr,out_child=nullptr,err_parent=nullptr,err_child=nullptr;
      std::vector<HANDLE> owned_child_handles;
      auto close_child_handles=[&]() noexcept
      {
        const HANDLE direct[]{in_child,out_child,err_child};
        for(std::size_t i=0;i<3;++i)
        {
          const HANDLE handle=direct[i];
          if(!handle||handle==INVALID_HANDLE_VALUE||std::find(owned_child_handles.begin(),owned_child_handles.end(),handle)!=owned_child_handles.end())continue;
          if(std::find(direct,direct+i,handle)==direct+i)(void)::CloseHandle(handle);
        }
        for(std::size_t i=0;i<owned_child_handles.size();++i)
          if(std::find(owned_child_handles.begin(),owned_child_handles.begin()+static_cast<std::ptrdiff_t>(i),owned_child_handles[i])==owned_child_handles.begin()+static_cast<std::ptrdiff_t>(i))
            (void)::CloseHandle(owned_child_handles[i]);
        in_child=nullptr;out_child=nullptr;err_child=nullptr;owned_child_handles.clear();
      };
      auto cleanup_handles=[&]() noexcept
      {
        close_child_handles();close_handle(in_parent);close_handle(out_parent);close_handle(err_parent);
      };
      auto make_pipe=[&](bool input)->bool
      {
        HANDLE child=nullptr,parent=nullptr;
        if(!::CreatePipe(&child,&parent,&inheritable,0))return false;
        if(!::SetHandleInformation(parent,HANDLE_FLAG_INHERIT,0)){const auto e=::GetLastError();::CloseHandle(child);::CloseHandle(parent);::SetLastError(e);return false;}
        if(input){in_child=child;in_parent=parent;}else{out_parent=parent;out_child=child;}
        return true;
      };
      if(state.input.kind_==StandardInput::Kind::pipe && !make_pipe(true)){auto e=win_error(errors::spawn_failed,"CreatePipe stdin");cleanup_handles();return e;}
      if(state.output.kind_==StandardOutput::Kind::pipe && !make_pipe(false)){auto e=win_error(errors::spawn_failed,"CreatePipe stdout");cleanup_handles();return e;}
      if(state.error.kind_==StandardError::Kind::pipe)
      {
        HANDLE child=nullptr,parent=nullptr;
        if(!::CreatePipe(&child,&parent,&inheritable,0)){auto e=win_error(errors::spawn_failed,"CreatePipe stderr");cleanup_handles();return e;}
        if(!::SetHandleInformation(parent,HANDLE_FLAG_INHERIT,0)){auto e=win_error(errors::spawn_failed,"SetHandleInformation stderr");::CloseHandle(child);::CloseHandle(parent);cleanup_handles();return e;}
        err_child=child;err_parent=parent;
      }
      auto null_handle=[&](DWORD access)->HANDLE
      { HANDLE h=::CreateFileW(L"NUL",access,FILE_SHARE_READ|FILE_SHARE_WRITE,&inheritable,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);return h==INVALID_HANDLE_VALUE?nullptr:h; };
      if(state.input.kind_==StandardInput::Kind::null){in_child=null_handle(GENERIC_READ);if(!in_child){auto e=win_error(errors::spawn_failed,"open NUL stdin");cleanup_handles();return e;}}
      if(state.output.kind_==StandardOutput::Kind::null){out_child=null_handle(GENERIC_WRITE);if(!out_child){auto e=win_error(errors::spawn_failed,"open NUL stdout");cleanup_handles();return e;}}
      if(state.error.kind_==StandardError::Kind::null){err_child=null_handle(GENERIC_WRITE);if(!err_child){auto e=win_error(errors::spawn_failed,"open NUL stderr");cleanup_handles();return e;}}
      auto file_handle=[&](files::File *file,bool read)->HANDLE
      {
        if(!file||!file->is_open())contract_violation();const auto access=file->access();
        if(read ? !(access==files::FileAccess::read||access==files::FileAccess::read_write) : !(access==files::FileAccess::write||access==files::FileAccess::read_write))contract_violation();
        const auto duplicate=files::detail_duplicate_process_handle(*file);if(duplicate==-1)return nullptr;
        if(!files::detail_set_process_handle_inheritable(duplicate,true)){(void)files::detail_close_process_handle(duplicate);return nullptr;}
        HANDLE handle=reinterpret_cast<HANDLE>(duplicate);owned_child_handles.push_back(handle);return handle;
      };
      if(state.input.kind_==StandardInput::Kind::file){in_child=file_handle(state.input.file_,true);if(!in_child){auto e=error(errors::invalid_stdio,"could not duplicate stdin handle");cleanup_handles();return e;}}
      if(state.output.kind_==StandardOutput::Kind::file){out_child=file_handle(state.output.file_,false);if(!out_child){auto e=error(errors::invalid_stdio,"could not duplicate stdout handle");cleanup_handles();return e;}}
      if(state.error.kind_==StandardError::Kind::file){err_child=file_handle(state.error.file_,false);if(!err_child){auto e=error(errors::invalid_stdio,"could not duplicate stderr handle");cleanup_handles();return e;}}
      auto inherited_std=[&](DWORD which)->HANDLE
      {
        HANDLE source=::GetStdHandle(which);if(!source||source==INVALID_HANDLE_VALUE)return nullptr;
        HANDLE copy=nullptr;if(!::DuplicateHandle(::GetCurrentProcess(),source,::GetCurrentProcess(),&copy,0,TRUE,DUPLICATE_SAME_ACCESS))return INVALID_HANDLE_VALUE;
        owned_child_handles.push_back(copy);return copy;
      };
      if(state.input.kind_==StandardInput::Kind::inherit)
      {in_child=inherited_std(STD_INPUT_HANDLE);if(in_child==INVALID_HANDLE_VALUE){auto e=win_error(errors::invalid_stdio,"duplicate inherited stdin");cleanup_handles();return e;}if(!in_child){in_child=null_handle(GENERIC_READ);if(!in_child){auto e=win_error(errors::invalid_stdio,"open NUL stdin");cleanup_handles();return e;}owned_child_handles.push_back(in_child);}}
      if(state.output.kind_==StandardOutput::Kind::inherit)
      {out_child=inherited_std(STD_OUTPUT_HANDLE);if(out_child==INVALID_HANDLE_VALUE){auto e=win_error(errors::invalid_stdio,"duplicate inherited stdout");cleanup_handles();return e;}if(!out_child){out_child=null_handle(GENERIC_WRITE);if(!out_child){auto e=win_error(errors::invalid_stdio,"open NUL stdout");cleanup_handles();return e;}owned_child_handles.push_back(out_child);}}
      if(state.error.kind_==StandardError::Kind::inherit)
      {err_child=inherited_std(STD_ERROR_HANDLE);if(err_child==INVALID_HANDLE_VALUE){auto e=win_error(errors::invalid_stdio,"duplicate inherited stderr");cleanup_handles();return e;}if(!err_child){err_child=null_handle(GENERIC_WRITE);if(!err_child){auto e=win_error(errors::invalid_stdio,"open NUL stderr");cleanup_handles();return e;}owned_child_handles.push_back(err_child);}}
      if(state.error.kind_==StandardError::Kind::merge_stdout)err_child=out_child;

      STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;
      startup.StartupInfo.hStdInput=in_child;startup.StartupInfo.hStdOutput=out_child;startup.StartupInfo.hStdError=err_child;
      std::vector<HANDLE> handle_list;
      for(HANDLE handle:{in_child,out_child,err_child})if(handle&&std::find(handle_list.begin(),handle_list.end(),handle)==handle_list.end())handle_list.push_back(handle);
      SIZE_T attribute_size=0;
      (void)::InitializeProcThreadAttributeList(nullptr,1,0,&attribute_size);
      auto attribute_storage=vix::detail::allocation_or_terminate([&]{return std::vector<std::byte>(attribute_size);});
      startup.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attribute_storage.data());
      if(!::InitializeProcThreadAttributeList(startup.lpAttributeList,1,0,&attribute_size)){auto e=win_error(errors::spawn_failed,"InitializeProcThreadAttributeList");cleanup_handles();return e;}
      const BOOL updated=::UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        handle_list.data(),handle_list.size()*sizeof(HANDLE),nullptr,nullptr);
      if(!updated){const auto code=::GetLastError();::DeleteProcThreadAttributeList(startup.lpAttributeList);auto e=win_error(errors::spawn_failed,"UpdateProcThreadAttribute",code);cleanup_handles();return e;}
      // CreateProcessW may modify this buffer. Copy the already-terminated
      // UTF-16 command line into mutable storage without losing its terminator.
      std::vector<wchar_t> mutable_command(command.begin(), command.end());
      PROCESS_INFORMATION information{};
      const BOOL created=::CreateProcessW(executable->c_str(),mutable_command.data(),nullptr,nullptr,TRUE,
        EXTENDED_STARTUPINFO_PRESENT|CREATE_UNICODE_ENVIRONMENT,env_block.data(),cwd?cwd->c_str():nullptr,
        &startup.StartupInfo,&information);
      const DWORD create_error=created ? ERROR_SUCCESS : ::GetLastError();
      ::DeleteProcThreadAttributeList(startup.lpAttributeList);
      if(!created){auto e=win_error(errors::spawn_failed,"CreateProcessW",create_error);cleanup_handles();return e;}
      (void)::CloseHandle(information.hThread);
      close_child_handles();
      auto terminate_failed_setup=[&]() noexcept
      {
        close_handle(in_parent);close_handle(out_parent);close_handle(err_parent);
        if (::TerminateProcess(information.hProcess, 1))
          (void)::WaitForSingleObject(information.hProcess, INFINITE);
        (void)::CloseHandle(information.hProcess);
      };
      auto input=state.input.kind_==StandardInput::Kind::pipe?pipe_fd(in_parent,_O_WRONLY):Result<int>{-1};if(state.input.kind_==StandardInput::Kind::pipe)in_parent=nullptr;
      if(!input){terminate_failed_setup();return input.error();}
      auto output=state.output.kind_==StandardOutput::Kind::pipe?pipe_fd(out_parent,_O_RDONLY):Result<int>{-1};if(state.output.kind_==StandardOutput::Kind::pipe)out_parent=nullptr;
      if(!output){close_fd(input.value());terminate_failed_setup();return output.error();}
      auto errout=state.error.kind_==StandardError::Kind::pipe?pipe_fd(err_parent,_O_RDONLY):Result<int>{-1};if(state.error.kind_==StandardError::Kind::pipe)err_parent=nullptr;
      if(!errout){close_fd(input.value());close_fd(output.value());terminate_failed_setup();return errout.error();}
      record->handle=information.hProcess;
      detail_process_record_access::set_native(record->id,information.dwProcessId);
      return Process{std::move(record),input.take(),output.take(),errout.take()};
    });
  }
}
