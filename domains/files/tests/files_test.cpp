#include <vix/files/Files.hpp>
#include <vix/files/detail/ProcessBridge.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#endif
#endif

namespace
{
  vix::files::Path path(const std::string &value)
  {
    auto result = vix::files::Path::from_utf8(value);
    assert(result);
    return result.take();
  }
}

int main()
{
  namespace files = vix::files;
#if defined(__wasi__)
  const auto root = std::filesystem::path("/workspace/vix_files_baseline_test");
#else
  const auto root = std::filesystem::temp_directory_path() / "vix_files_baseline_test";
#endif
  std::filesystem::remove_all(root);
  const auto root_u8 = root.u8string();
  auto root_path = path(std::string(reinterpret_cast<const char *>(root_u8.data()), root_u8.size()));
  auto invalid_utf8_path = files::Path::from_utf8(std::string{"\xff", 1});
  assert(!invalid_utf8_path && invalid_utf8_path.error().identity() == files::errors::path_not_utf8);
  auto normalized = path("one/./two/../three").lexically_normal();
  assert(normalized.to_utf8().value() == "one/three");
#if defined(_WIN32)
  assert(path("C:/vix").is_absolute());
  assert(path("C:vix").is_relative());
  assert(path("//server/share/vix").is_absolute());
  assert(path("folder\\vix").is_relative());
#endif
  assert(path("one/../two") != path("two"));
  assert(files::create_directories(root_path));
  auto child = root_path.joined(path("payload.txt"));
  const std::string text = "hello";
  auto written = files::write_utf8(child, text, files::WriteOptions::create_new());
  assert(written);
  auto unicode_directory = root_path.joined(path("unicode-\xf0\x9f\x8c\x8d"));
  assert(files::create_directory(unicode_directory));
  auto unicode_child = unicode_directory.joined(path("caf\xc3\xa9-\xf0\x90\x90\xb7-\xe6\x96\x87\xe4\xbb\xb6.txt"));
  assert(files::write_utf8(unicode_child, "unicode", files::WriteOptions::create_new()));
  assert(files::read_utf8(unicode_child, files::ReadLimit{32}).value() == "unicode");
  auto second_new = files::write_utf8(child, text, files::WriteOptions::create_new());
  assert(!second_new && second_new.error().identity() == files::errors::already_exists);
  auto read = files::read_utf8(child, files::ReadLimit{100});
  assert(read && read.value() == text);
  auto too_small = files::read_all(child, files::ReadLimit{4});
  assert(!too_small && too_small.error().identity() == files::errors::size_limit_exceeded);
  auto metadata = files::info(child);
  assert(metadata && metadata.value().type() == files::FileType::regular);
  auto opened = files::open(child, files::OpenOptions::existing(files::FileAccess::read));
  assert(opened);
  auto file = opened.take();
  std::byte buffer[8]{};
  assert(file.read_some(std::span<std::byte>{buffer, 0}).value() == 0);
  assert(file.seek(2, files::SeekOrigin::begin).value() == 2);
  assert(file.tell().value() == 2);
  assert(file.read_some(std::span<std::byte>{buffer, 3}).value() == 3);
  assert(static_cast<char>(buffer[0]) == 'l' && static_cast<char>(buffer[2]) == 'o');
  assert(file.flush());
#if !defined(__wasi__)
  const auto process_handle = files::detail_duplicate_process_handle(file);
  assert(process_handle != -1);
  assert(files::detail_set_process_handle_inheritable(process_handle, true));
  assert(files::detail_set_process_handle_inheritable(process_handle, false));
  assert(files::detail_close_process_handle(process_handle));
#endif
  assert(file.close());
  assert(file.close());

  auto append = files::open(child, files::OpenOptions::append_existing());
  assert(append);
  auto appender = append.take();
  assert(appender.seek(0, files::SeekOrigin::begin));
  const std::byte exclamation{static_cast<unsigned char>('!')};
  assert(appender.write_some(std::span<const std::byte>{&exclamation, 1}).value() == 1);
  assert(appender.flush());
  assert(appender.close());
  assert(files::read_utf8(child, files::ReadLimit{100}).value() == "hello!");

  auto copied = root_path.joined(path("copied.txt"));
  assert(files::copy_file(child, copied, files::CopyOptions::create_new()));
  auto renamed = root_path.joined(path("renamed.txt"));
  assert(files::write_utf8(renamed, "replace me", files::WriteOptions::create_new()));
  assert(files::rename_replace(copied, renamed));
  assert(files::read_utf8(renamed, files::ReadLimit{100}).value() == "hello!");

#if defined(__unix__) || defined(__APPLE__)
  auto linked = root_path.joined(path("entry-link"));
  assert(::symlink("payload.txt", linked.to_utf8().value().c_str()) == 0);
  assert(files::exists(linked).value());
  auto write_link = files::open(linked, files::OpenOptions::existing(files::FileAccess::write));
  assert(!write_link && write_link.error().identity() == files::errors::symlink_not_allowed);
#elif defined(_WIN32)
  const auto link_native = root / L"entry-link";
  // A relative target path creates a relative symlink; Win32 has no separate
  // SYMBOLIC_LINK_FLAG_RELATIVE flag.
  const auto link_flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
  if (::CreateSymbolicLinkW(link_native.c_str(), L"payload.txt", link_flags))
  {
    const auto link_u8 = link_native.u8string();
    auto linked_path = path(std::string(reinterpret_cast<const char *>(link_u8.data()), link_u8.size()));
    auto write_link = files::open(linked_path, files::OpenOptions::existing(files::FileAccess::write));
    assert(!write_link && write_link.error().identity() == files::errors::symlink_not_allowed);
    assert(files::remove_file(linked_path));
  }
  else
  {
    const auto error = ::GetLastError();
    assert(error == ERROR_PRIVILEGE_NOT_HELD || error == ERROR_ACCESS_DENIED || error == ERROR_NOT_SUPPORTED);
    std::fprintf(stderr, "Windows symlink behavior not exercised: host denied symlink creation (error %lu).\n",
                 static_cast<unsigned long>(error));
  }
#endif

  auto iterator = files::iterate(root_path);
  assert(iterator);
  std::size_t count = 0;
  while (true)
  {
    auto entry = iterator.value().next();
    assert(entry);
    if (!entry.value()) break;
    ++count;
  }
  assert(count >= 2);
  assert(!iterator.value().next().value().has_value());
  assert(iterator.value().close());
  assert(files::remove_tree(root_path, files::RemoveTreeOptions::require_existing()));
}
