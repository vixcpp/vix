/**
 *
 *  @file headers_pipeline_demo.cpp — Security headers pipeline demo (Vix.cpp)
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

#include <boost/beast/http.hpp>
#include <vix/middleware/pipeline.hpp>
#include <vix/middleware/security/headers.hpp>

using namespace vix::middleware;

static vix::vhttp::RawRequest make_req()
{
  namespace http = boost::beast::http;
  vix::vhttp::RawRequest req{http::verb::get, "/x", 11};
  req.set(http::field::host, "localhost");
  req.prepare_payload();
  return req;
}

int main()
{
  namespace http = boost::beast::http;

  auto raw = make_req();
  http::response<http::string_body> res;

  vix::vhttp::Request req(raw, {});
  vix::vhttp::ResponseWrapper w(res);

  HttpPipeline p;

  // Default headers() adds:
  // - X-Content-Type-Options: nosniff
  // - X-Frame-Options: DENY
  // - Referrer-Policy: no-referrer
  // - Permissions-Policy: ...
  p.use(vix::middleware::security::headers());

  int final_calls = 0;
  p.run(req, w, [&](Request &, Response &)
        {
        final_calls++;
        w.ok().text("OK"); });

  assert(final_calls == 1);
  assert(res.result_int() == 200);

  // Must exist
  assert(res.find("X-Content-Type-Options") != res.end());
  assert(res.find("X-Frame-Options") != res.end());
  assert(res.find("Referrer-Policy") != res.end());
  assert(res.find("Permissions-Policy") != res.end());

  std::cout << "[OK] security headers pipeline demo\n";
  return 0;
}
