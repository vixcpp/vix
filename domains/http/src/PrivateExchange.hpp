#pragma once

#include "Http1.hpp"

#include <vix/network/Operation.hpp>
#include <vix/network/TcpConnection.hpp>
#include <vix/ErrorIdentity.hpp>
#include <vix/Result.hpp>
#include <vix/Duration.hpp>

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace vix::http::detail
{
  inline constexpr auto malformed_response =
      vix::ErrorIdentity::define<"vix.http">(1);
  inline constexpr auto malformed_request =
      vix::ErrorIdentity::define<"vix.http">(4);
  inline constexpr auto informational_response_limit_exceeded =
      vix::ErrorIdentity::define<"vix.http">(5);

  // Private exchange policy: keep the exchange and its provider resources
  // finite even when a peer never supplies a final response. The fourth
  // parameter is an internal test seam for exercising expiry without waiting
  // for the production budget.
  inline constexpr vix::Duration default_exchange_lifetime =
      vix::Duration::seconds(30);
  inline constexpr std::size_t max_informational_responses = 128;
  inline constexpr std::size_t default_exchange_receive_chunk = 11;
  inline constexpr std::size_t max_exchange_receive_chunk = 4096;

  struct InformationalResponseCounter
  {
    std::size_t count{};

    [[nodiscard]] bool consume() noexcept
    {
      if (count >= max_informational_responses)
        return false;
      ++count;
      return true;
    }
  };
  inline constexpr auto response_too_large =
      vix::ErrorIdentity::define<"vix.http">(2);
  inline constexpr auto unsupported_response_framing =
      vix::ErrorIdentity::define<"vix.http">(3);

  struct ExchangeResponse
  {
    std::string version;
    unsigned status_code{};
    std::string reason;
    std::vector<HeaderField> headers;
    std::vector<std::byte> body;
    std::vector<HeaderField> trailers;
  };

  // Private feasibility seam only. The returned parent operation owns the
  // HTTP lease and its request storage until every Network child is terminal.
  [[nodiscard]] vix::Result<vix::network::Operation<ExchangeResponse>>
  start_private_exchange(vix::network::TcpConnection &&,
                         std::span<const std::byte> serialized_request,
                         vix::network::OperationOptions = {},
                         vix::Duration exchange_lifetime =
                             default_exchange_lifetime,
                         std::size_t receive_chunk_for_test =
                             default_exchange_receive_chunk,
                         bool establish_default_deadline = true,
                         std::size_t response_body_limit_for_exchange = 65536);
} // namespace vix::http::detail
