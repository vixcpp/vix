#ifndef VIX_FILES_FILES_HPP
#define VIX_FILES_FILES_HPP

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <vix/Result.hpp>

#include <vix/files/DirectoryIterator.hpp>
#include <vix/files/Errors.hpp>
#include <vix/files/File.hpp>
#include <vix/files/FileInfo.hpp>
#include <vix/files/Options.hpp>
#include <vix/files/Path.hpp>

namespace vix::files
{
  [[nodiscard]] vix::Result<File> open(const Path &path, OpenOptions options);
  [[nodiscard]] vix::Result<std::vector<std::byte>> read_all(const Path &path, ReadLimit limit);
  [[nodiscard]] vix::Result<std::string> read_utf8(const Path &path, ReadLimit limit);
  [[nodiscard]] vix::Result<void> write_all(const Path &path, std::span<const std::byte> data, WriteOptions options);
  [[nodiscard]] vix::Result<void> write_utf8(const Path &path, std::string_view text, WriteOptions options);
  [[nodiscard]] vix::Result<FileInfo> info(const Path &path);
  [[nodiscard]] vix::Result<bool> exists(const Path &path);
  [[nodiscard]] vix::Result<void> create_directory(const Path &path);
  [[nodiscard]] vix::Result<void> create_directories(const Path &path);
  [[nodiscard]] vix::Result<DirectoryIterator> iterate(const Path &path);
  [[nodiscard]] vix::Result<void> remove_file(const Path &path);
  [[nodiscard]] vix::Result<void> remove_directory(const Path &path);
  [[nodiscard]] vix::Result<void> remove_tree(const Path &path, RemoveTreeOptions options);
  [[nodiscard]] vix::Result<void> copy_file(const Path &source, const Path &destination, CopyOptions options);
  [[nodiscard]] vix::Result<void> rename_replace(const Path &source, const Path &destination, RenameOptions options = RenameOptions::replace());
} // namespace vix::files

#endif
