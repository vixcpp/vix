#include <vix/process/ProcessDomain.hpp>
#include <vix/files/Files.hpp>

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <cstdint>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace
{
  std::filesystem::path helper_path;
  std::filesystem::path path_from_utf8(std::string_view value)
  {
    std::u8string encoded;
    encoded.reserve(value.size());
    for (const char byte : value) encoded.push_back(static_cast<char8_t>(static_cast<unsigned char>(byte)));
    return std::filesystem::path(encoded);
  }
  vix::files::Path path(std::string_view value)
  { auto result=vix::files::Path::from_utf8(value);assert(result);return result.take(); }
  std::string utf8(const std::filesystem::path &value)
  {const auto bytes=value.u8string();return {reinterpret_cast<const char *>(bytes.data()),bytes.size()};}
  vix::process::Executable helper()
  { auto value=vix::process::Executable::path(path(utf8(helper_path)));assert(value);return value.take(); }
  std::string collect(vix::process::ChildOutput &output)
  {
    std::string result;
    std::array<std::byte,128> bytes{};
    for(;;){auto count=output.read_some(bytes);assert(count);if(count.value()==0)break;result.append(reinterpret_cast<const char*>(bytes.data()),count.value());}
    return result;
  }
}

int main(int argc,char **argv)
{
  using namespace vix;
  using namespace vix::process;

#if defined(_WIN32)
  std::wstring module(32768,L'\0');
  const DWORD module_size=::GetModuleFileNameW(nullptr,module.data(),static_cast<DWORD>(module.size()));
  assert(module_size>0&&module_size<module.size());
  module.resize(module_size);
  helper_path=std::filesystem::path(module).parent_path()/L"vix_process_test_child.exe";
#else
  assert(argc>0);
  helper_path=std::filesystem::absolute(path_from_utf8(argv[0])).parent_path()/"vix_process_test_child";
#endif

  ProcessSpec arguments(helper());
  assert(arguments.add_argument("args"));
  assert(arguments.add_argument("two words"));
  assert(arguments.add_argument("quote\"and\\slash"));
  assert(arguments.add_argument(""));
  assert(arguments.add_argument("trailing\\\\"));
  assert(arguments.add_argument("\xf0\x9f\x8c\x8d"));
  arguments.set_stdout(StandardOutput::pipe());
  auto argument_process=spawn(std::move(arguments));assert(argument_process);
  auto argument_child=argument_process.take();
  auto argument_output=argument_child.take_stdout();assert(argument_output);
  const auto argument_text=collect(argument_output.value());
  assert(argument_text=="arg:two words\narg:quote\"and\\slash\narg:\narg:trailing\\\\\narg:\xf0\x9f\x8c\x8d\n");
  assert(argument_child.wait().value().succeeded());

  auto search=Executable::search_path(utf8(
#if defined(_WIN32)
    helper_path.stem()
#else
    helper_path.filename()
#endif
  ));assert(search);
  ProcessSpec search_spec(search.take());assert(search_spec.add_argument("exit"));assert(search_spec.add_argument("0"));
  auto search_environment=Environment::inherit().with("PATH",utf8(helper_path.parent_path()));assert(search_environment);
  search_spec.set_environment(search_environment.take());
  auto searched=spawn(std::move(search_spec));assert(searched&&searched.value().wait().value().succeeded());

  const auto missing_path=std::filesystem::temp_directory_path()/"vix-process-no-such-executable";
  auto missing_executable=Executable::path(path(utf8(missing_path)));assert(missing_executable);
  ProcessSpec missing_spec(missing_executable.take());
  missing_spec.set_stdin(StandardInput::pipe());
  missing_spec.set_stdout(StandardOutput::pipe());
  missing_spec.set_stderr(StandardError::pipe());
  auto missing=spawn(std::move(missing_spec));
  assert(!missing&&missing.error().identity()==errors::executable_not_found);
  assert(argument_child.wait().value().succeeded());

  ProcessSpec environment_spec(helper());
  assert(environment_spec.add_argument("env"));
#if defined(_WIN32)
  auto environment=Environment::empty().with("vix_process_test_value","portable-\xf0\x9f\x8c\x8d");
#else
  auto environment=Environment::empty().with("VIX_PROCESS_TEST_VALUE","portable-\xf0\x9f\x8c\x8d");
#endif
  assert(environment);
  environment_spec.set_environment(environment.take());
  environment_spec.set_stdout(StandardOutput::pipe());
  auto environment_process=spawn(std::move(environment_spec));assert(environment_process);
  auto environment_child=environment_process.take();
  auto environment_output=environment_child.take_stdout();assert(environment_output);
  assert(collect(environment_output.value())=="portable-\xf0\x9f\x8c\x8d");
  assert(environment_child.wait().value().succeeded());

  auto working_directory=std::filesystem::temp_directory_path()/std::filesystem::path(u8"vix-process-working-directory-\U0001F30D");
  std::filesystem::create_directories(working_directory);
  ProcessSpec cwd_spec(helper());assert(cwd_spec.add_argument("cwd"));
  auto cwd_path=path(utf8(working_directory));assert(cwd_spec.set_working_directory(cwd_path));
  cwd_spec.set_stdout(StandardOutput::pipe());
  auto cwd_process=spawn(std::move(cwd_spec));assert(cwd_process);
  auto cwd_child=cwd_process.take();auto cwd_output=cwd_child.take_stdout();assert(cwd_output);
  assert(collect(cwd_output.value()).find("vix-process-working-directory-\xf0\x9f\x8c\x8d")!=std::string::npos);
  assert(cwd_child.wait().value().succeeded());
  std::filesystem::remove_all(working_directory);
#if defined(__APPLE__)
  const auto missing_working_directory=std::filesystem::temp_directory_path()/"vix-process-missing-working-directory";
  std::filesystem::remove_all(missing_working_directory);
  ProcessSpec invalid_cwd_spec(helper());
  assert(invalid_cwd_spec.set_working_directory(path(utf8(missing_working_directory))));
  invalid_cwd_spec.set_stdin(StandardInput::pipe());
  invalid_cwd_spec.set_stdout(StandardOutput::pipe());
  invalid_cwd_spec.set_stderr(StandardError::pipe());
  auto invalid_cwd=spawn(std::move(invalid_cwd_spec));
  assert(!invalid_cwd&&invalid_cwd.error().identity()==errors::invalid_working_directory);
#endif

  ProcessSpec stderr_spec(helper());assert(stderr_spec.add_argument("stderr"));stderr_spec.set_stderr(StandardError::pipe());
  auto stderr_process=spawn(std::move(stderr_spec));assert(stderr_process);
  auto stderr_child=stderr_process.take();auto stderr_output=stderr_child.take_stderr();assert(stderr_output);
  assert(collect(stderr_output.value())=="stderr-marker");assert(stderr_child.wait().value().succeeded());

#if !defined(_WIN32)
  // A broken child pipe must be reported as a Process error without
  // delivering SIGPIPE to, and terminating, the calling process.
  ProcessSpec broken_pipe_spec(helper());
  assert(broken_pipe_spec.add_argument("exit"));
  assert(broken_pipe_spec.add_argument("0"));
  broken_pipe_spec.set_stdin(StandardInput::pipe());
  auto broken_pipe_process_result=spawn(std::move(broken_pipe_spec));assert(broken_pipe_process_result);
  auto broken_pipe_process=broken_pipe_process_result.take();
  auto broken_pipe_input=broken_pipe_process.take_stdin();assert(broken_pipe_input);
  assert(broken_pipe_process.wait().value().succeeded());
  const std::array<std::byte,1> broken_pipe_payload{std::byte{'x'}};
  auto broken_pipe_write=broken_pipe_input.value().write_some(broken_pipe_payload);
  assert(!broken_pipe_write&&broken_pipe_write.error().identity()==errors::broken_pipe);
#endif

  ProcessSpec copy_spec(helper());
  assert(copy_spec.add_argument("copy"));
  copy_spec.set_stdin(StandardInput::pipe());
  copy_spec.set_stdout(StandardOutput::pipe());
  auto copy_process=spawn(std::move(copy_spec));assert(copy_process);
  auto copier=copy_process.take();
  auto input=copier.take_stdin();auto output=copier.take_stdout();assert(input&&output);
  const std::string payload(256*1024,'x');
  std::string received;
  std::thread reader([&]{received=collect(output.value());});
  std::size_t sent=0;
  while(sent<payload.size())
  {
    auto write=input.value().write_some(std::as_bytes(std::span{payload.data()+sent,payload.size()-sent}));
    assert(write&&write.value()>0);sent+=write.value();
  }
  assert(input.value().close());
  reader.join();
  assert(received==payload);
  assert(copier.wait().value().succeeded());

  ProcessSpec stderr_copy_spec(helper());
  assert(stderr_copy_spec.add_argument("copy-stderr"));
  stderr_copy_spec.set_stdin(StandardInput::pipe());
  stderr_copy_spec.set_stderr(StandardError::pipe());
  auto stderr_copy_result=spawn(std::move(stderr_copy_spec));assert(stderr_copy_result);
  auto stderr_copier=stderr_copy_result.take();
  auto stderr_input=stderr_copier.take_stdin();auto stderr_pipe=stderr_copier.take_stderr();assert(stderr_input&&stderr_pipe);
  std::string stderr_received;
  std::thread stderr_reader([&]{stderr_received=collect(stderr_pipe.value());});
  sent=0;
  while(sent<payload.size())
  {
    auto write=stderr_input.value().write_some(std::as_bytes(std::span{payload.data()+sent,payload.size()-sent}));
    assert(write&&write.value()>0);sent+=write.value();
  }
  assert(stderr_input.value().close());stderr_reader.join();
  assert(stderr_received==payload&&stderr_copier.wait().value().succeeded());

  const auto file_path=path(utf8(std::filesystem::temp_directory_path()/"vix-process-stdio-file"));
  assert(files::write_utf8(file_path,"file-stdio",files::WriteOptions::create_or_replace()));
  auto input_file_result=files::open(file_path,files::OpenOptions::existing(files::FileAccess::read));assert(input_file_result);
  auto input_file=input_file_result.take();
  ProcessSpec file_stdio(helper());assert(file_stdio.add_argument("copy"));
  file_stdio.set_stdin(StandardInput::from_file(input_file));file_stdio.set_stdout(StandardOutput::pipe());
  auto file_child_result=spawn(std::move(file_stdio));assert(file_child_result);
  auto file_child=file_child_result.take();auto file_output=file_child.take_stdout();assert(file_output);
  assert(collect(file_output.value())=="file-stdio");assert(file_child.wait().value().succeeded());
  assert(input_file.is_open());assert(input_file.close());assert(files::remove_file(file_path));

  ProcessSpec exit_spec(helper());
  assert(exit_spec.add_argument("exit"));assert(exit_spec.add_argument("23"));
  auto exit_process=spawn(std::move(exit_spec));assert(exit_process);
  auto exited=exit_process.value().wait();assert(exited&&exited.value().exit_code()==23);

  ProcessSpec delayed(helper());
  assert(delayed.add_argument("sleep-ms"));assert(delayed.add_argument("1000"));
  auto delayed_result=spawn(std::move(delayed));assert(delayed_result);
  auto running=delayed_result.take();
  CancellationSource cancellation;assert(cancellation.request_cancel());
  auto cancelled=running.wait(WaitOptions{cancellation.token()});
  assert(cancelled.status()==CompletionStatus::Cancelled);
  auto deadline_wait=running.wait(WaitOptions{{},Deadline::after(Duration::milliseconds(5))});
  assert(deadline_wait.status()==CompletionStatus::Failure&&deadline_wait.error().identity()==errors::wait_timed_out);
  assert(running.terminate());
  auto terminated=running.wait();
#if defined(_WIN32)
  assert(terminated&&terminated.value().kind()==ExitKind::exited&&terminated.value().exit_code()==1);
#else
  assert(terminated&&terminated.value().kind()==ExitKind::abnormal);
#endif

  for(int iteration=0;iteration<6;++iteration)
  {
    ProcessSpec race_spec(helper());assert(race_spec.add_argument("sleep-ms"));assert(race_spec.add_argument("8"));
    auto race_result=spawn(std::move(race_spec));assert(race_result);auto race_process=race_result.take();
    CancellationSource race_cancel;
    std::thread requester([&]{std::this_thread::sleep_for(std::chrono::milliseconds(5));(void)race_cancel.request_cancel();});
    auto completion=race_process.wait(WaitOptions{race_cancel.token()});requester.join();
    assert(completion.status()==CompletionStatus::Success||completion.status()==CompletionStatus::Cancelled);
    auto final_status=race_process.wait();assert(final_status&&final_status.value().succeeded());
  }
  for(int iteration=0;iteration<6;++iteration)
  {
    ProcessSpec race_spec(helper());assert(race_spec.add_argument("sleep-ms"));assert(race_spec.add_argument("8"));
    auto race_result=spawn(std::move(race_spec));assert(race_result);auto race_process=race_result.take();
    auto completion=race_process.wait(WaitOptions{{},Deadline::after(Duration::milliseconds(5))});
    assert(completion.status()==CompletionStatus::Success||
      (completion.status()==CompletionStatus::Failure&&completion.error().identity()==errors::wait_timed_out));
    auto final_status=race_process.wait();assert(final_status&&final_status.value().succeeded());
  }

  const auto dropped_marker=std::filesystem::temp_directory_path()/"vix-process-dropped-child-marker";
  std::filesystem::remove(dropped_marker);
  {
    ProcessSpec dropped_spec(helper());assert(dropped_spec.add_argument("delay-file"));
    assert(dropped_spec.add_argument("100"));assert(dropped_spec.add_argument(utf8(dropped_marker)));
    auto dropped=spawn(std::move(dropped_spec));assert(dropped);
  }
  bool marker_created=false;
  for(int attempt=0;attempt<200&&!marker_created;++attempt)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::ifstream marker(dropped_marker,std::ios::binary);
    std::string value;
    if(marker){marker>>value;marker_created=value=="child-survived";}
  }
  assert(marker_created);
  std::filesystem::remove(dropped_marker);

  std::atomic<bool> concurrent_spawns_ok{true};
  std::vector<std::thread> spawners;
  for(int index=0;index<4;++index)
    spawners.emplace_back([&]
    {
      ProcessSpec concurrent_spec(helper());
      if(!concurrent_spec.add_argument("exit")||!concurrent_spec.add_argument("0")){concurrent_spawns_ok=false;return;}
      auto child=spawn(std::move(concurrent_spec));
      if(!child||!child.value().wait()||!child.value().wait().value().succeeded())concurrent_spawns_ok=false;
    });
  for(auto &spawner:spawners)spawner.join();
  assert(concurrent_spawns_ok.load());

  std::array<std::filesystem::path,4> dropped_markers;
  std::atomic<bool> concurrent_drops_ok{true};
  std::vector<std::thread> droppers;
  for(std::size_t index=0;index<dropped_markers.size();++index)
  {
    dropped_markers[index]=std::filesystem::temp_directory_path()/
      ("vix-process-concurrent-drop-"+std::to_string(index));
    std::filesystem::remove(dropped_markers[index]);
    droppers.emplace_back([&,index]
    {
      ProcessSpec dropped_spec(helper());
      if(!dropped_spec.add_argument("delay-file")||!dropped_spec.add_argument("80")||
         !dropped_spec.add_argument(utf8(dropped_markers[index])))
      {concurrent_drops_ok=false;return;}
      auto dropped=spawn(std::move(dropped_spec));
      if(!dropped)concurrent_drops_ok=false;
    });
  }
  for(auto &dropper:droppers)dropper.join();
  assert(concurrent_drops_ok.load());
  for(const auto &marker_path:dropped_markers)
  {
    bool complete=false;
    for(int attempt=0;attempt<200&&!complete;++attempt)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      std::ifstream marker(marker_path,std::ios::binary);std::string value;
      if(marker){marker>>value;complete=value=="child-survived";}
    }
    assert(complete);std::filesystem::remove(marker_path);
  }

  ProcessSpec concurrent_wait_spec(helper());
  assert(concurrent_wait_spec.add_argument("sleep-ms"));assert(concurrent_wait_spec.add_argument("10"));
  auto concurrent_wait_result=spawn(std::move(concurrent_wait_spec));assert(concurrent_wait_result);
  auto concurrent_wait_process=concurrent_wait_result.take();
  std::atomic<bool> waiters_ok{true};
  auto wait_once=[&]
  {
    auto status=concurrent_wait_process.wait();
    if(!status||!status.value().succeeded())waiters_ok=false;
  };
  std::thread waiter_a(wait_once),waiter_b(wait_once);
  waiter_a.join();waiter_b.join();
  assert(waiters_ok.load());
  assert(concurrent_wait_process.wait().value().succeeded());

#if defined(_WIN32)
  SECURITY_ATTRIBUTES inheritable_attributes{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
  const auto sentinel_path=std::filesystem::temp_directory_path()/"vix-process-inheritance-sentinel";
  HANDLE sentinel=::CreateFileW(sentinel_path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
    &inheritable_attributes,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  assert(sentinel!=INVALID_HANDLE_VALUE);
  ProcessSpec isolation_spec(helper());assert(isolation_spec.add_argument("handle-check"));
  assert(isolation_spec.add_argument(std::to_string(reinterpret_cast<std::uintptr_t>(sentinel))));
  assert(isolation_spec.add_argument("vix-process-inheritance-sentinel"));
  isolation_spec.set_stdout(StandardOutput::pipe());
  auto isolation_result=spawn(std::move(isolation_spec));assert(isolation_result);
  auto isolation_child=isolation_result.take();auto isolation_output=isolation_child.take_stdout();assert(isolation_output);
  assert(collect(isolation_output.value())=="isolated");assert(isolation_child.wait().value().succeeded());
  assert(::CloseHandle(sentinel));std::filesystem::remove(sentinel_path);
#elif defined(__APPLE__)
  // macOS uses POSIX_SPAWN_CLOEXEC_DEFAULT: an unrelated descriptor that is
  // deliberately inheritable in the parent must still be absent in the child.
  const auto sentinel_path=std::filesystem::temp_directory_path()/"vix-process-inheritance-sentinel";
  const int sentinel=::open(sentinel_path.c_str(),O_CREAT|O_TRUNC|O_RDWR,0600);
  assert(sentinel>=3);
  const int descriptor_flags=::fcntl(sentinel,F_GETFD);
  assert(descriptor_flags>=0&&::fcntl(sentinel,F_SETFD,descriptor_flags&~FD_CLOEXEC)==0);
  ProcessSpec isolation_spec(helper());
  assert(isolation_spec.add_argument("fd-check"));
  assert(isolation_spec.add_argument(std::to_string(sentinel)));
  assert(isolation_spec.add_argument(utf8(sentinel_path)));
  isolation_spec.set_stdout(StandardOutput::pipe());
  auto isolation_result=spawn(std::move(isolation_spec));assert(isolation_result);
  auto isolation_child=isolation_result.take();auto isolation_output=isolation_child.take_stdout();assert(isolation_output);
  assert(collect(isolation_output.value())=="isolated");
  assert(isolation_child.wait().value().succeeded());
  assert(::close(sentinel)==0);
  std::filesystem::remove(sentinel_path);
#endif

  for(int i=0;i<12;++i)
  {
    ProcessSpec short_lived(helper());assert(short_lived.add_argument("exit"));assert(short_lived.add_argument("0"));
    auto child=spawn(std::move(short_lived));assert(child);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  return 0;
}
