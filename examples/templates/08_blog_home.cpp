/**
 *
 *  @file 08_blog_home.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
// Run:
//   vix run examples/templates/08_blog_home.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template files:
//   examples/templates/views/08_blog_home/base.html
//   examples/templates/views/08_blog_home/header.html
//   examples/templates/views/08_blog_home/index.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/08_blog_home");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;

            ctx.set("page_title", "Vix Blog");
            ctx.set("blog_name", "Vix Engineering Blog");
            ctx.set("tagline", "Notes on C++, performance, and real-world systems.");

            vix::template_::Object featured_post;
            featured_post["title"] = "Building reliable systems for unreliable environments";
            featured_post["excerpt"] =
                "Why local-first execution, deterministic behavior, and offline-first design matter in the real world.";
            featured_post["author"] = "Gaspard Kirira";
            featured_post["date"] = "March 31, 2026";
            featured_post["category"] = "Architecture";
            featured_post["slug"] = "reliable-systems";
            ctx.set("featured_post", featured_post);

            vix::template_::Array posts;

            {
              vix::template_::Object post;
              post["title"] = "Introducing the Vix template engine";
              post["excerpt"] =
                  "A built-in HTML template engine for Vix with layouts, includes, loops, and filters.";
              post["author"] = "Gaspard Kirira";
              post["date"] = "March 28, 2026";
              post["category"] = "Templates";
              post["slug"] = "introducing-vix-templates";
              posts.emplace_back(post);
            }

            {
              vix::template_::Object post;
              post["title"] = "Stable performance under load";
              post["excerpt"] =
                  "Why predictable behavior matters more than isolated peak benchmark numbers.";
              post["author"] = "Gaspard Kirira";
              post["date"] = "March 24, 2026";
              post["category"] = "Performance";
              post["slug"] = "stable-performance";
              posts.emplace_back(post);
            }

            {
              vix::template_::Object post;
              post["title"] = "Designing for offline-first systems";
              post["excerpt"] =
                  "How to think about local writes, recovery, sync, and resilience from the start.";
              post["author"] = "Gaspard Kirira";
              post["date"] = "March 20, 2026";
              post["category"] = "Offline-first";
              post["slug"] = "offline-first-design";
              posts.emplace_back(post);
            }

            ctx.set("posts", posts);

            res.render("index.html", ctx); });

  app.run(8080);
}
