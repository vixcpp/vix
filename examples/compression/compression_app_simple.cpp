/**
 *
 *  @file  compression_app_simple.cpp — Compression middleware (App) example (Vix.cpp)
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
//   vix run compression_app_simple.cpp
//
// Test:
//   # No Accept-Encoding => Vary may still appear (add_vary=true), but no planned header
//   curl -i http://localhost:8080/x
//
//   # With Accept-Encoding and big enough body => Vary + (debug) X-Vix-Compression: planned
//   curl -i -H "Accept-Encoding: gzip, br" http://localhost:8080/x
//
//   # Small body (< min_size) => no planned header
//   curl -i -H "Accept-Encoding: gzip" http://localhost:8080/small
//
//   # Inspect only headers
//   curl -s -D - -o /dev/null -H "Accept-Encoding: gzip, br" http://localhost:8080/x
// ============================================================================

#include <iostream>
#include <string>
#include <vix.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/performance/compression.hpp>

using namespace vix;

static void print_help()
{
  std::cout
      << "Vix Compression example running:\n"
      << " http://localhost:8080/\n"
      << " http://localhost:8080/x\n"
      << " http://localhost:8080/small\n\n"
      << "Try:\n"
      << " curl -i http://localhost:8080/x\n"
      << " curl -i -H \"Accept-Encoding: gzip, br\" http://localhost:8080/x\n"
      << " curl -i -H \"Accept-Encoding: gzip\" http://localhost:8080/small\n";
}

int main()
{
  App app;

  // Install compression middleware globally
  auto mw = vix::middleware::app::adapt_ctx(
      vix::middleware::performance::compression({
          .min_size = 8, // same as smoke test
          .add_vary = true,
          .enabled = true,
      }));

  app.use(std::move(mw));

  app.get("/", [](Request &, Response &res)
          { res.send("Compression middleware installed. Try /x with Accept-Encoding."); });

  // Big body => should trigger "planned" (debug) if Accept-Encoding asks gzip/br
  app.get("/x", [](Request &, Response &res)
          { res.status(200).send(std::string(20, 'a')); });

  // Small body => should NOT trigger "planned"
  app.get("/small", [](Request &, Response &res)
          {
            res.status(200).send("aaaa"); // 4 bytes
          });

  print_help();
  app.run(8080);
  return 0;
}
