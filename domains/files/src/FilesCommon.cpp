#include <vix/files/Files.hpp>
#include "FilesInternal.hpp"

#include <vix/detail/AllocationFailure.hpp>

#include <cstdint>
#include <string_view>
#include <utility>

namespace vix::files
{
  namespace
  {
    [[nodiscard]] Error file_error(ErrorIdentity identity, std::string_view message)
    {
      return detail::allocation_or_terminate([&] { return Error{identity, std::string(message)}; });
    }

  } // namespace

  bool detail_valid_utf8(std::string_view value) noexcept
  {
    for (std::size_t i = 0; i < value.size();)
    {
      const auto lead = static_cast<unsigned char>(value[i]);
      if (lead <= 0x7f) { ++i; continue; }
      std::size_t count = 0;
      std::uint32_t code = 0;
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

  Result<std::vector<std::byte>> read_all(const Path &path, ReadLimit limit)
  {
    auto opened = open(path, OpenOptions::existing(FileAccess::read));
    if (!opened) return std::move(opened).error();
    File file = opened.take();
    return detail::allocation_or_terminate([&]() -> Result<std::vector<std::byte>>
    {
      std::vector<std::byte> content;
      std::vector<std::byte> chunk(64 * 1024);
      while (true)
      {
        auto read = file.read_some(chunk);
        if (!read) return std::move(read).error();
        const auto count = read.value();
        if (count == 0) break;
        if (count > limit.bytes() || content.size() > limit.bytes() - count)
          return file_error(errors::size_limit_exceeded, "read limit exceeded");
        content.insert(content.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(count));
      }
      auto closed = file.close();
      if (!closed) return std::move(closed).error();
      return content;
    });
  }

  Result<std::string> read_utf8(const Path &path, ReadLimit limit)
  {
    auto bytes = read_all(path, limit);
    if (!bytes) return std::move(bytes).error();
    return detail::allocation_or_terminate([&]() -> Result<std::string>
    {
      const auto &data = bytes.value();
      std::string result(reinterpret_cast<const char *>(data.data()), data.size());
      if (!detail_valid_utf8(result)) return file_error(errors::invalid_utf8, "file content is not valid UTF-8");
      return result;
    });
  }

  Result<void> write_all(const Path &path, std::span<const std::byte> data, WriteOptions options)
  {
    const auto open_options = options.destination_policy() == DestinationPolicy::create_new
        ? OpenOptions::create_new(FileAccess::write)
        : options.destination_policy() == DestinationPolicy::replace_existing
            ? OpenOptions::truncate_existing() : OpenOptions::create_or_truncate();
    auto opened = open(path, open_options);
    if (!opened) return std::move(opened).error();
    File file = opened.take();
    std::size_t position = 0;
    while (position < data.size())
    {
      auto wrote = file.write_some(data.subspan(position));
      if (!wrote) return std::move(wrote).error();
      position += wrote.value();
    }
    return file.close();
  }

  Result<void> write_utf8(const Path &path, std::string_view text, WriteOptions options)
  {
    if (!detail_valid_utf8(text)) return file_error(errors::invalid_utf8, "text is not valid UTF-8");
    return write_all(path, std::as_bytes(std::span{text.data(), text.size()}), options);
  }
} // namespace vix::files
