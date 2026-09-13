/**
 *
 *  @file rate_limit_pipeline_demo.cpp — Rate limit pipeline demo (Vix.cpp)
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
//   vix run rate_limit_pipeline_demo.cpp
// ============================================================================
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <vix/http/Request.hpp>
#include <vix/http/Response.hpp>
#include <vix/http/ResponseWrapper.hpp>
#include <vix/middleware/pipeline.hpp>
#include <vix/middleware/security/rate_limit.hpp>

using namespace vix::middleware;

static vix::http::Request make_req()
{
  vix::http::Request::HeaderMap headers;
  headers.emplace("Host", "localhost");
  headers.emplace("x-forwarded-for", "1.2.3.4");

  return vix::http::Request("GET", "/api/x", std::move(headers), "");
}

int main()
{
  vix::middleware::security::RateLimitOptions opt{};
  opt.capacity = 2.0;
  opt.refill_per_sec = 0.0;
  opt.add_headers = true;

  HttpPipeline p;

  auto shared = std::make_shared<vix::middleware::security::RateLimiterState>();
  p.services().provide<vix::middleware::security::RateLimiterState>(shared);

  p.use(vix::middleware::security::rate_limit(opt));

  auto run_once = [&](vix::http::Response &res)
  {
    auto req = make_req();
    vix::http::ResponseWrapper w(res);

    p.run(req, w, [&](Request &, Response &resp)
          { resp.ok().text("OK"); });
  };

  // 1) OK
  {
    vix::http::Response res;
    run_once(res);
    assert(res.status() == 200);
    assert(res.body() == "OK");
    assert(!res.header("X-RateLimit-Limit").empty());
    assert(!res.header("X-RateLimit-Remaining").empty());
  }

  // 2) OK
  {
    vix::http::Response res;
    run_once(res);
    assert(res.status() == 200);
    assert(res.body() == "OK");
  }

  // 3) BLOCKED
  {
    vix::http::Response res;
    run_once(res);
    assert(res.status() == 429);
    assert(res.body().find("rate_limited") != std::string::npos);
    assert(!res.header("Retry-After").empty());
    assert(res.header("X-RateLimit-Remaining") == "0");
  }

  std::cout << "[OK] rate_limit pipeline demo\n";
  return 0;
}
