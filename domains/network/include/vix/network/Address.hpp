#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vix/Result.hpp>

namespace vix::network
{
  inline constexpr auto invalid_address = vix::ErrorIdentity::define<"vix.network">(1);
  inline constexpr auto invalid_port = vix::ErrorIdentity::define<"vix.network">(2);

  enum class AddressFamily : std::uint8_t { ipv4, ipv6 };

  class Address
  {
  public:
    static vix::Result<Address> parse(std::string_view);
    [[nodiscard]] AddressFamily family() const noexcept { return family_; }
    [[nodiscard]] std::uint32_t scope_id() const noexcept { return scope_id_; }
    [[nodiscard]] std::string display() const;
    friend bool operator==(const Address &, const Address &) noexcept = default;

  private:
    Address(AddressFamily family, std::array<std::uint8_t, 16> bytes, std::uint32_t scope) noexcept
      : family_(family), bytes_(bytes), scope_id_(scope) {}
    AddressFamily family_{};
    std::array<std::uint8_t, 16> bytes_{};
    std::uint32_t scope_id_{};
    friend struct detail_native_endpoint;
  };

  class Endpoint
  {
  public:
    static vix::Result<Endpoint> create(Address, std::uint32_t);
    [[nodiscard]] const Address &address() const noexcept { return address_; }
    [[nodiscard]] std::uint16_t port() const noexcept { return port_; }
    friend bool operator==(const Endpoint &, const Endpoint &) noexcept = default;

  private:
    Endpoint(Address address, std::uint16_t port) noexcept : address_(address), port_(port) {}
    Address address_;
    std::uint16_t port_{};
    friend struct detail_native_endpoint;
  };
}
