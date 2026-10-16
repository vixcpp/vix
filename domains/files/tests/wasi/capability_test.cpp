#include <vix/files/Files.hpp>

#include <cassert>
#include <string>

namespace
{
  vix::files::Path path(std::string_view value)
  {
    auto result = vix::files::Path::from_utf8(value);
    assert(result);
    return result.take();
  }
}

int main()
{
  namespace files = vix::files;
  const auto root = path("/workspace/files-f2-case");
  (void)files::remove_tree(root, files::RemoveTreeOptions::ignore_missing());
  assert(files::create_directories(root));

  const auto source = root.joined(path("payload.txt"));
  auto opened = files::open(source, files::OpenOptions::create_new(files::FileAccess::write));
  assert(opened);
  auto file = opened.take();
  const std::string content = "preopened filesystem";
  assert(file.write_some(std::as_bytes(std::span{content.data(), content.size()})).value() == content.size());
  assert(file.flush());
  assert(file.close());
  assert(file.close());
  assert(files::read_utf8(source, files::ReadLimit{128}).value() == content);

  const auto metadata = files::info(source);
  assert(metadata && metadata.value().type() == files::FileType::regular);
  auto directory = files::iterate(root);
  assert(directory);
  const auto entry = directory.value().next();
  assert(entry && entry.value());
  assert(directory.value().close());

  auto denied = files::open(path("/outside-files-capability.txt"),
                            files::OpenOptions::existing(files::FileAccess::read));
  assert(!denied && denied.error().identity() == files::errors::permission_denied);

  const auto renamed = root.joined(path("renamed.txt"));
  assert(files::rename_replace(source, renamed));
  assert(files::remove_file(renamed));
  assert(files::remove_directory(root));
}
