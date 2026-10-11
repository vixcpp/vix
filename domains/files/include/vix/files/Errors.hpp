#ifndef VIX_FILES_ERRORS_HPP
#define VIX_FILES_ERRORS_HPP

#include <vix/ErrorIdentity.hpp>

namespace vix::files::errors
{
  inline constexpr auto not_found = ErrorIdentity::define<"vix.files">(1);
  inline constexpr auto already_exists = ErrorIdentity::define<"vix.files">(2);
  inline constexpr auto permission_denied = ErrorIdentity::define<"vix.files">(3);
  inline constexpr auto invalid_path = ErrorIdentity::define<"vix.files">(4);
  inline constexpr auto path_not_utf8 = ErrorIdentity::define<"vix.files">(5);
  inline constexpr auto symlink_not_allowed = ErrorIdentity::define<"vix.files">(6);
  inline constexpr auto not_a_file = ErrorIdentity::define<"vix.files">(7);
  inline constexpr auto not_a_directory = ErrorIdentity::define<"vix.files">(8);
  inline constexpr auto invalid_offset = ErrorIdentity::define<"vix.files">(9);
  inline constexpr auto size_limit_exceeded = ErrorIdentity::define<"vix.files">(10);
  inline constexpr auto invalid_utf8 = ErrorIdentity::define<"vix.files">(11);
  inline constexpr auto read_failed = ErrorIdentity::define<"vix.files">(12);
  inline constexpr auto write_failed = ErrorIdentity::define<"vix.files">(13);
  inline constexpr auto close_failed = ErrorIdentity::define<"vix.files">(14);
  inline constexpr auto flush_failed = ErrorIdentity::define<"vix.files">(15);
  inline constexpr auto create_failed = ErrorIdentity::define<"vix.files">(16);
  inline constexpr auto remove_failed = ErrorIdentity::define<"vix.files">(17);
  inline constexpr auto rename_failed = ErrorIdentity::define<"vix.files">(18);
  inline constexpr auto copy_failed = ErrorIdentity::define<"vix.files">(19);
  inline constexpr auto cleanup_failed = ErrorIdentity::define<"vix.files">(20);
  inline constexpr auto not_same_filesystem = ErrorIdentity::define<"vix.files">(21);
  inline constexpr auto resource_exhausted = ErrorIdentity::define<"vix.files">(22);
  inline constexpr auto no_space = ErrorIdentity::define<"vix.files">(23);
  inline constexpr auto read_only_filesystem = ErrorIdentity::define<"vix.files">(24);
  inline constexpr auto name_too_long = ErrorIdentity::define<"vix.files">(25);
  inline constexpr auto quota_exhausted = ErrorIdentity::define<"vix.files">(26);
  inline constexpr auto interrupted = ErrorIdentity::define<"vix.files">(27);
  inline constexpr auto device_failure = ErrorIdentity::define<"vix.files">(28);
  inline constexpr auto io_failure = ErrorIdentity::define<"vix.files">(29);
} // namespace vix::files::errors

#endif
