#pragma once

#include "Http1.hpp"

#include <vix/http/Http.hpp>

namespace vix::http::detail
{
  [[nodiscard]] vix::Result<std::vector<std::byte>> snapshot_request(
      const vix::http::Request &, const vix::http::Limits &);
  [[nodiscard]] vix::Result<std::vector<std::byte>> snapshot_response(
      const vix::http::Response &, const vix::http::Limits &, bool response_to_head);
  [[nodiscard]] std::vector<vix::http::HeaderField> public_headers(
      const std::vector<vix::http::detail::HeaderField> &);
}
