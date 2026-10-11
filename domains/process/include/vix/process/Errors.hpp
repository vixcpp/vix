#ifndef VIX_PROCESS_ERRORS_HPP
#define VIX_PROCESS_ERRORS_HPP

#include <vix/ErrorIdentity.hpp>

namespace vix::process::errors
{
  inline constexpr auto executable_not_found = ErrorIdentity::define<"vix.process">(1);
  inline constexpr auto not_executable = ErrorIdentity::define<"vix.process">(2);
  inline constexpr auto access_denied = ErrorIdentity::define<"vix.process">(3);
  inline constexpr auto invalid_argument = ErrorIdentity::define<"vix.process">(4);
  inline constexpr auto invalid_environment = ErrorIdentity::define<"vix.process">(5);
  inline constexpr auto invalid_working_directory = ErrorIdentity::define<"vix.process">(6);
  inline constexpr auto invalid_stdio = ErrorIdentity::define<"vix.process">(7);
  inline constexpr auto stdio_not_piped = ErrorIdentity::define<"vix.process">(8);
  inline constexpr auto pipe_already_transferred = ErrorIdentity::define<"vix.process">(9);
  inline constexpr auto broken_pipe = ErrorIdentity::define<"vix.process">(10);
  inline constexpr auto resource_exhausted = ErrorIdentity::define<"vix.process">(11);
  inline constexpr auto spawn_failed = ErrorIdentity::define<"vix.process">(12);
  inline constexpr auto wait_failed = ErrorIdentity::define<"vix.process">(13);
  inline constexpr auto observation_lost = ErrorIdentity::define<"vix.process">(14);
  inline constexpr auto wait_timed_out = ErrorIdentity::define<"vix.process">(15);
  inline constexpr auto termination_failed = ErrorIdentity::define<"vix.process">(16);
  inline constexpr auto already_exited = ErrorIdentity::define<"vix.process">(17);
  inline constexpr auto pipe_read_failed = ErrorIdentity::define<"vix.process">(18);
  inline constexpr auto close_failed = ErrorIdentity::define<"vix.process">(19);
}

#endif
