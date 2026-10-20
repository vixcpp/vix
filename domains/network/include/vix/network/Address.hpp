/**
 * @file Address.hpp
 * @author Gaspard Kirira
 *
 * Copyright 2026, Gaspard Kirira.
 * All rights reserved.
 * https://github.com/vixcpp/vix
 *
 * Use of this source code is governed by an MIT license
 * that can be found in the LICENSE file.
 *
 * Vix.cpp
 *
 * @brief IP addresses and TCP endpoint representations.
 *
 * @details
 * Provides value types for representing numeric IPv4
 * and IPv6 addresses and combining them with TCP ports.
 *
 * Address parsing is numeric only. It does not perform
 * DNS resolution or hostname lookup.
 *
 * @see vix::network::Address
 * @see vix::network::Endpoint
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vix/Result.hpp>

namespace vix::network
{
  /**
   * @brief Identifies an invalid numeric IP address.
   *
   * @details
   * Returned by Address::parse() when the supplied
   * text is not a valid numeric IPv4 or IPv6 address,
   * or contains an invalid IPv6 scope identifier.
   *
   * Error domain: "vix.network".
   * Error code: 1.
   *
   * @see Address::parse()
   */
  inline constexpr auto invalid_address = vix::ErrorIdentity::define<"vix.network">(1);

  /**
   * @brief Identifies an invalid network port.
   *
   * @details
   * Returned by Endpoint::create() when the supplied
   * port exceeds the maximum 16-bit TCP port value.
   *
   * Valid ports range from 0 to 65535.
   *
   * Error domain: "vix.network".
   * Error code: 2.
   *
   * @see Endpoint::create()
   */
  inline constexpr auto invalid_port = vix::ErrorIdentity::define<"vix.network">(2);

  /**
   * @brief Identifies the Internet Protocol address family.
   *
   * @details
   * AddressFamily distinguishes IPv4 and IPv6 addresses.
   *
   * @see Address::family()
   */
  enum class AddressFamily : std::uint8_t
  {
    ipv4, ///< Internet Protocol version 4.
    ipv6  ///< Internet Protocol version 6.
  };

  /**
   * @brief Represents a numeric IPv4 or IPv6 address.
   *
   * @details
   * Address is a value type containing:
   * - The IP address family.
   * - The binary address representation.
   * - An optional numeric IPv6 scope identifier.
   *
   * Use parse() to create an Address from a textual
   * numeric IP address.
   *
   * @par Example: IPv4
   * @code{.cpp}
   * #include <vix/network/Address.hpp>
   *
   * auto result = vix::network::Address::parse(
   *     "127.0.0.1");
   *
   * if (result)
   * {
   *     auto address = result.take();
   *
   *     auto family = address.family();
   *     auto text = address.display();
   * }
   * @endcode
   *
   * @par Example: IPv6
   * @code{.cpp}
   * auto result = vix::network::Address::parse("::1");
   *
   * if (result)
   * {
   *     auto address = result.take();
   * }
   * @endcode
   *
   * @par IPv6 scope identifiers
   * IPv6 addresses may contain a numeric scope
   * identifier following the '%' character.
   *
   * For example:
   * @code{.cpp}
   * auto result = vix::network::Address::parse(
   *     "fe80::1%2");
   * @endcode
   *
   * Scope identifiers are represented by
   * unsigned 32-bit integers.
   *
   * Interface names are not resolved by parse().
   * Only numeric scope identifiers are accepted.
   *
   * @par Value semantics
   * Address stores its data by value.
   *
   * Copies do not share external resources,
   * and an Address does not own a socket.
   *
   * Addresses can be compared for equality.
   *
   * @note
   * Address::parse() does not resolve hostnames
   * such as "localhost" or "example.com".
   *
   * @note
   * An Address identifies an IP address only.
   * Use Endpoint to associate it with a port.
   *
   * @see AddressFamily
   * @see Endpoint
   * @see parse()
   */
  class Address
  {
  public:
    /**
     * @brief Parses a numeric IPv4 or IPv6 address.
     *
     * @param value Text containing the numeric address.
     *
     * @return An Address on success, or an Error
     *         with invalid_address if parsing fails.
     *
     * @details
     * Accepts numeric addresses such as:
     * - "127.0.0.1"
     * - "192.168.1.10"
     * - "::1"
     * - "2001:db8::1"
     * - "fe80::1%2"
     *
     * An IPv6 scope identifier is optional and must
     * contain decimal digits representing a value
     * within the uint32_t range.
     *
     * Parsing rejects:
     * - Empty addresses.
     * - Invalid numeric IP representations.
     * - Hostnames requiring DNS resolution.
     * - Invalid or nonnumeric scope identifiers.
     * - IPv4 addresses with a scope suffix.
     *
     * The input is not retained after parsing.
     *
     * @par Example
     * @code{.cpp}
     * auto result = Address::parse("2001:db8::1");
     *
     * if (!result)
     * {
     *     // Handle invalid_address.
     * }
     * else
     * {
     *     auto address = result.take();
     * }
     * @endcode
     *
     * @note
     * This operation performs address parsing only.
     * It does not open a socket or contact a network.
     *
     * @see invalid_address
     * @see family()
     * @see display()
     */
    static vix::Result<Address> parse(std::string_view);

    /**
     * @brief Returns the address family.
     *
     * @return AddressFamily::ipv4 or AddressFamily::ipv6.
     *
     * @par Example
     * @code{.cpp}
     * if (address.family() == AddressFamily::ipv6)
     * {
     *     // Handle an IPv6 address.
     * }
     * @endcode
     *
     * @see AddressFamily
     */
    [[nodiscard]] AddressFamily family() const noexcept { return family_; }

    /**
     * @brief Returns the numeric IPv6 scope identifier.
     *
     * @return The stored scope identifier.
     *
     * @details
     * A zero value indicates that no nonzero scope
     * identifier is associated with the address.
     *
     * IPv4 addresses always have a zero scope identifier.
     *
     * @par Example
     * @code{.cpp}
     * auto result = Address::parse("fe80::1%2");
     *
     * if (result)
     * {
     *     auto scope = result.value().scope_id();
     *     // scope == 2
     * }
     * @endcode
     *
     * @note
     * The identifier is numeric and is not converted
     * to or from an operating-system interface name.
     *
     * @see parse()
     * @see family()
     */
    [[nodiscard]] std::uint32_t scope_id() const noexcept { return scope_id_; }

    /**
     * @brief Formats the numeric IP address as text.
     *
     * @return A string representing the address.
     *
     * @details
     * Uses the platform's numeric IP address formatter.
     *
     * For IPv6 addresses with a nonzero scope identifier,
     * the returned string includes a numeric suffix
     * following the '%' character.
     *
     * The output contains an IP address only,
     * without a TCP port.
     *
     * @par Example
     * @code{.cpp}
     * auto result = Address::parse("127.0.0.1");
     *
     * if (result)
     * {
     *     std::string text = result.value().display();
     * }
     * @endcode
     *
     * @note
     * Formatting may normalize the textual representation.
     * It is not guaranteed to reproduce the original input.
     *
     * @see parse()
     * @see Endpoint
     */
    [[nodiscard]] std::string display() const;

    /**
     * @brief Compares two IP addresses for equality.
     *
     * @param left First address.
     * @param right Second address.
     *
     * @return true if both addresses have identical
     *         stored families, bytes, and scope identifiers.
     *
     * @details
     * Equality compares the address values rather
     * than their original textual representations.
     *
     * @par Example
     * @code{.cpp}
     * auto first = Address::parse("127.0.0.1");
     * auto second = Address::parse("127.0.0.1");
     *
     * if (first && second)
     * {
     *     bool equal = first.value() == second.value();
     * }
     * @endcode
     *
     * @note
     * IPv6 addresses with different scope identifiers
     * are not equal.
     */
    friend bool operator==(const Address &, const Address &) noexcept = default;

  private:
    Address(AddressFamily family, std::array<std::uint8_t, 16> bytes, std::uint32_t scope) noexcept
        : family_(family), bytes_(bytes), scope_id_(scope) {}
    AddressFamily family_{};
    std::array<std::uint8_t, 16> bytes_{};
    std::uint32_t scope_id_{};
    friend struct detail_native_endpoint;
  };

  /**
   * @brief Represents an IP address and a TCP port.
   *
   * @details
   * Endpoint combines an Address with a 16-bit port.
   *
   * It identifies a local or remote network endpoint
   * that can be supplied to TCP networking operations.
   *
   * @par Example
   * @code{.cpp}
   * #include <vix/network/Address.hpp>
   *
   * auto address = vix::network::Address::parse(
   *     "127.0.0.1");
   *
   * if (address)
   * {
   *     auto endpoint = vix::network::Endpoint::create(
   *         address.take(),
   *         8080);
   *
   *     if (endpoint)
   *     {
   *         auto port = endpoint.value().port();
   *         auto text = endpoint.value().address().display();
   *     }
   * }
   * @endcode
   *
   * @par Port range
   * Endpoint accepts ports from 0 to 65535.
   *
   * Port zero is useful when binding a listener
   * and requesting an operating-system-assigned port.
   *
   * A remote TCP connection requires a nonzero
   * destination port.
   *
   * @par Value semantics
   * Endpoint owns its Address value.
   *
   * It is copyable and movable, and does not
   * own or create a network socket.
   *
   * @note
   * Creating an Endpoint does not establish
   * a TCP connection or start a listener.
   *
   * @see Address
   * @see create()
   */
  class Endpoint
  {
  public:
    /**
     * @brief Creates an endpoint from an address and port.
     *
     * @param address IPv4 or IPv6 address.
     * @param port TCP port in the range 0 to 65535.
     *
     * @return An Endpoint on success, or an Error
     *         with invalid_port when the port is invalid.
     *
     * @details
     * Validates the supplied port and stores the
     * address and port as an Endpoint value.
     *
     * Port zero is accepted by this factory.
     *
     * Whether port zero is valid for a particular
     * network operation depends on that operation.
     *
     * @par Example
     * @code{.cpp}
     * auto address = Address::parse("::1");
     *
     * if (address)
     * {
     *     auto endpoint = Endpoint::create(
     *         address.take(),
     *         443);
     *
     *     if (!endpoint)
     *     {
     *         // Handle invalid_port.
     *     }
     * }
     * @endcode
     *
     * @note
     * This operation performs no network I/O.
     *
     * @see invalid_port
     * @see Address::parse()
     */
    static vix::Result<Endpoint> create(Address, std::uint32_t);

    /**
     * @brief Returns the endpoint's IP address.
     *
     * @return Read-only reference to the stored Address.
     *
     * @details
     * The returned reference is owned by this Endpoint.
     *
     * It must not be used after the Endpoint
     * has been destroyed.
     *
     * @par Example
     * @code{.cpp}
     * auto family = endpoint.address().family();
     * @endcode
     *
     * @see Address
     */
    [[nodiscard]] const Address &address() const noexcept { return address_; }

    /**
     * @brief Returns the endpoint's TCP port.
     *
     * @return The stored port number.
     *
     * @details
     * The value is an unsigned 16-bit integer
     * between 0 and 65535.
     *
     * @par Example
     * @code{.cpp}
     * auto port = endpoint.port();
     * @endcode
     *
     * @see create()
     */
    [[nodiscard]] std::uint16_t port() const noexcept { return port_; }

    /**
     * @brief Compares two network endpoints for equality.
     *
     * @param left First endpoint.
     * @param right Second endpoint.
     *
     * @return true when both the stored Address
     *         and port are equal, false otherwise.
     *
     * @details
     * Addresses are compared by value, including
     * their IP family, binary representation,
     * and IPv6 scope identifier.
     *
     * @note
     * Endpoint equality does not perform DNS resolution
     * or verify network connectivity.
     */
    friend bool operator==(const Endpoint &, const Endpoint &) noexcept = default;

  private:
    Endpoint(Address address, std::uint16_t port) noexcept : address_(address), port_(port) {}
    Address address_;
    std::uint16_t port_{};
    friend struct detail_native_endpoint;
  };
}
