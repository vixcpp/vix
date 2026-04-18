/**
 *
 *  @file csrf_pipeline_demo.cpp - CSRF pipeline demo (Vix.cpp)
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
// Run:
//   vix run csrf_pipeline_demo.cpp
// ============================================================================

#include <cassert>
#include <iostream>
#include <string>

#include <vix/http/Request.hpp>
#include <vix/http/Response.hpp>
#include <vix/http/ResponseWrapper.hpp>
#include <vix/middleware/pipeline.hpp>
#include <vix/middleware/security/csrf.hpp>

using namespace vix::middleware;

static vix::http::Request make_post(bool ok)
{
  vix::http::Request::HeaderMap headers;
  headers["Host"] = "localhost";

  // Cookie + header must match
  headers["Cookie"] = "csrf_token=abc";
  headers["x-csrf-token"] = ok ? "abc" : "wrong";
  headers["Content-Type"] = "application/x-www-form-urlencoded";

  return vix::http::Request(
      "POST",
      "/api/update",
      std::move(headers),
      "x=1");
}

int main()
{
  // FAIL
  {
    auto req = make_post(false);
    vix::http::Response raw_res;
    vix::http::ResponseWrapper w(raw_res);

    HttpPipeline p;
    p.use(vix::middleware::security::csrf());

    int final_calls = 0;
    p.run(req, w, [&](Request &, Response &res)
          {
            final_calls++;
            res.ok().text("OK"); });

    assert(final_calls == 0);
    assert(raw_res.status() == 403);
  }

  // OK
  {
    auto req = make_post(true);
    vix::http::Response raw_res;
    vix::http::ResponseWrapper w(raw_res);

    HttpPipeline p;
    p.use(vix::middleware::security::csrf());

    int final_calls = 0;
    p.run(req, w, [&](Request &, Response &res)
          {
            final_calls++;
            res.ok().text("OK"); });

    assert(final_calls == 1);
    assert(raw_res.status() == 200);
  }

  std::cout << "[OK] csrf pipeline demo\n";
  return 0;
}
