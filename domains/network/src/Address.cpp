#include <vix/network/Address.hpp>
#include "SocketPlatform.hpp"
#include <cstring>
#include <limits>

namespace vix::network
{
  vix::Result<Address> Address::parse(std::string_view value)
  {
    if (value.empty() || value.size() >= detail::numeric_address_text_size + 12)
      return vix::Error{invalid_address, "invalid numeric address"};
    std::array<std::uint8_t, 16> bytes{};
    char text[detail::numeric_address_text_size]{};
    std::uint32_t scope{};
    const auto percent = value.find('%');
    const auto host = value.substr(0, percent);
    if (host.empty() || host.size() >= sizeof text)
      return vix::Error{invalid_address, "invalid numeric address"};
    std::memcpy(text, host.data(), host.size());
    if (percent != std::string_view::npos)
    {
      const auto suffix = value.substr(percent + 1);
      if (suffix.empty())
        return vix::Error{invalid_address, "invalid numeric scope"};
      std::uint64_t parsed{};
      for (char c : suffix)
      {
        if (c < '0' || c > '9')
          return vix::Error{invalid_address, "invalid numeric scope"};
        parsed = parsed * 10 + unsigned(c - '0');
        if (parsed > std::numeric_limits<std::uint32_t>::max())
          return vix::Error{invalid_address, "invalid numeric scope"};
      }
      scope = static_cast<std::uint32_t>(parsed);
    }
    if (percent == std::string_view::npos && detail::parse_numeric_address(AddressFamily::ipv4, text, bytes.data()))
      return Address{AddressFamily::ipv4, bytes, 0};
    if (detail::parse_numeric_address(AddressFamily::ipv6, text, bytes.data()))
      return Address{AddressFamily::ipv6, bytes, scope};
    return vix::Error{invalid_address, "invalid numeric address"};
  }
  std::string Address::display() const
  {
    char text[detail::numeric_address_text_size]{};
    if (!detail::format_numeric_address(family_, bytes_.data(), text, sizeof text))
      std::terminate();
    std::string out{text};
    if (family_ == AddressFamily::ipv6 && scope_id_)
      out += "%" + std::to_string(scope_id_);
    return out;
  }
  vix::Result<Endpoint> Endpoint::create(Address address, std::uint32_t port)
  {
    if (port > 65535)
      return vix::Error{invalid_port, "port exceeds 65535"};
    return Endpoint{address, static_cast<std::uint16_t>(port)};
  }
}
