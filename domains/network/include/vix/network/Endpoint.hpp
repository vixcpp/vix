/**
 * @file Endpoint.hpp
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
 * @brief Convenience header for network endpoints.
 *
 * @details
 * Provides access to vix::network::Endpoint through
 * the public Network domain headers.
 *
 * Endpoint represents a numeric IPv4 or IPv6 address
 * combined with a TCP port.
 *
 * The Endpoint type is declared in Address.hpp.
 * This header includes that declaration without
 * introducing additional types or functionality.
 *
 * @par Example
 * @code{.cpp}
 * #include <vix/network/Endpoint.hpp>
 *
 * int main()
 * {
 *     using namespace vix::network;
 *
 *     auto address = Address::parse("127.0.0.1");
 *
 *     if (!address)
 *         return 1;
 *
 *     auto endpoint = Endpoint::create(
 *         address.take(),
 *         8080);
 *
 *     if (!endpoint)
 *         return 1;
 *
 *     auto port = endpoint.value().port();
 *     auto ip = endpoint.value().address().display();
 *
 *     return 0;
 * }
 * @endcode
 *
 * @par Endpoint behavior
 * Endpoint::create() accepts ports from 0 to 65535.
 *
 * Port zero can be used when binding a listener
 * to request an operating-system-assigned port.
 *
 * Creating an Endpoint does not create a socket,
 * start a listener, or establish a connection.
 *
 * @par Header organization
 * Both of the following includes provide
 * access to Endpoint:
 *
 * @code{.cpp}
 * #include <vix/network/Address.hpp>
 * @endcode
 *
 * @code{.cpp}
 * #include <vix/network/Endpoint.hpp>
 * @endcode
 *
 * @note
 * This header exists for API organization and
 * convenient inclusion.
 *
 * @see vix::network::Endpoint
 * @see vix::network::Address
 * @see vix::network::AddressFamily
 * @see vix::network::Endpoint::create()
 */

#pragma once

#include <vix/network/Address.hpp>
