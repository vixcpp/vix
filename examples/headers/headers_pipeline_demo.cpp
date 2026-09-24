/**
 *
 *  @file headers_pipeline_demo.cpp - Security headers pipeline demo (Vix.cpp)
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
// ----------------------------------------------------------------------------
// Run:
//   vix run headers_pipeline_demo.cpp
// ============================================================================

#include <cassert>
#include <iostream>

#include <vix/http/Request.hpp>
#include <vix/http/Response.hpp>
#include <vix/http/ResponseWrapper.hpp>
#include <vix/middleware/pipeline.hpp>
#include <vix/middleware/security/headers.hpp>

using namespace vix::middleware;

static vix::http::Request make_req()
{
  vix::http::Request::HeaderMap headers;
  headers["Host"] = "localhost";

  return vix::http::Request(
      "GET",
      "/x",
      std::move(headers),
      "");
}

int main()
{
  auto req = make_req();
  vix::http::Response raw_res;
  vix::http::ResponseWrapper w(raw_res);

  HttpPipeline p;

  // Default headers() adds:
  // - X-Content-Type-Options: nosniff
  // - X-Frame-Options: DENY
  // - Referrer-Policy: no-referrer
  // - Permissions-Policy: ...
  p.use(vix::middleware::security::headers());

  int final_calls = 0;
  p.run(req, w, [&](Request &, Response &res)
        {
          final_calls++;
          res.ok().text("OK"); });

  assert(final_calls == 1);
  assert(raw_res.status() == 200);

  // Must exist
  assert(raw_res.has_header("X-Content-Type-Options"));
  assert(raw_res.has_header("X-Frame-Options"));
  assert(raw_res.has_header("Referrer-Policy"));
  assert(raw_res.has_header("Permissions-Policy"));

  std::cout << "[OK] security headers pipeline demo\n";
  return 0;
}
