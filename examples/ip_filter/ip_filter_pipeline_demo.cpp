/**
 *
 *  @file examples/ip_filter/ip_filter_pipeline_demo.cpp
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

#include <cassert>
#include <iostream>
#include <string>

#include <vix/http/Request.hpp>
#include <vix/http/Response.hpp>
#include <vix/http/ResponseWrapper.hpp>
#include <vix/middleware/pipeline.hpp>
#include <vix/middleware/security/ip_filter.hpp>

using namespace vix::middleware;

static vix::http::Request make_req(std::string ip)
{
  vix::http::Request::HeaderMap headers;
  headers.emplace("Host", "localhost");
  headers.emplace("X-Forwarded-For", std::move(ip));

  return vix::http::Request("GET", "/x", std::move(headers), "");
}

int main()
{
  // Case 1: deny list blocks
  {
    auto req = make_req("1.2.3.4");
    vix::http::Response res;
    vix::http::ResponseWrapper w(res);

    vix::middleware::security::IpFilterOptions opt;
    opt.deny = {"1.2.3.4"};

    HttpPipeline p;
    p.use(vix::middleware::security::ip_filter(opt));

    int final_calls = 0;
    p.run(req, w, [&](Request &, Response &)
          {
                  final_calls++;
                  w.ok().text("OK"); });

    assert(final_calls == 0);
    assert(res.status() == 403);
  }

  // Case 2: allow list lets through
  {
    auto req = make_req("9.9.9.9");
    vix::http::Response res;
    vix::http::ResponseWrapper w(res);

    vix::middleware::security::IpFilterOptions opt;
    opt.allow = {"9.9.9.9"};

    HttpPipeline p;
    p.use(vix::middleware::security::ip_filter(opt));

    int final_calls = 0;
    p.run(req, w, [&](Request &, Response &)
          {
                  final_calls++;
                  w.ok().text("OK"); });

    assert(final_calls == 1);
    assert(res.status() == 200);
  }

  // Case 3: allow list blocks others
  {
    auto req = make_req("2.2.2.2");
    vix::http::Response res;
    vix::http::ResponseWrapper w(res);

    vix::middleware::security::IpFilterOptions opt;
    opt.allow = {"9.9.9.9"};

    HttpPipeline p;
    p.use(vix::middleware::security::ip_filter(opt));

    int final_calls = 0;
    p.run(req, w, [&](Request &, Response &)
          {
                  final_calls++;
                  w.ok().text("OK"); });

    assert(final_calls == 0);
    assert(res.status() == 403);
  }

  std::cout << "[OK] ip_filter pipeline demo\n";
  return 0;
}
