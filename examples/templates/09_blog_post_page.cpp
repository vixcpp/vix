/**
 *
 *  @file 09_blog_post_page.cpp
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
//   vix run examples/templates/09_blog_post_page.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template files:
//   examples/templates/views/09_blog_post_page/base.html
//   examples/templates/views/09_blog_post_page/header.html
//   examples/templates/views/09_blog_post_page/post.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/09_blog_post_page");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;

            ctx.set("page_title", "Building reliable systems for unreliable environments");
            ctx.set("blog_name", "Vix Engineering Blog");
            ctx.set("tagline", "Notes on C++, performance, and real-world systems.");

            vix::template_::Object post;
            post["title"] = "Building reliable systems for unreliable environments";
            post["excerpt"] =
                "Why local-first execution, deterministic behavior, and offline-first design matter in the real world.";
            post["author"] = "Gaspard Kirira";
            post["date"] = "March 31, 2026";
            post["category"] = "Architecture";
            post["read_time"] = "6 min read";
            post["content_1"] =
                "Most systems are designed as if the network is always available, latency is always stable, and failures are rare. Reality is different.";
            post["content_2"] =
                "In real environments, requests time out, machines restart, and connectivity disappears. Systems must continue to behave correctly when conditions degrade.";
            post["content_3"] =
                "This is why Vix focuses on deterministic execution, predictable behavior under load, and an architecture that stays useful even when the environment is unreliable.";
            ctx.set("post", post);

            vix::template_::Array tags;
            tags.emplace_back("C++");
            tags.emplace_back("Architecture");
            tags.emplace_back("Offline-first");
            tags.emplace_back("Reliability");
            ctx.set("tags", tags);

            vix::template_::Object author;
            author["name"] = "Gaspard Kirira";
            author["role"] = "Creator of Vix.cpp";
            author["bio"] =
                "Gaspard builds systems focused on performance, resilience, and offline-first execution for real-world environments.";
            ctx.set("author", author);

            vix::template_::Array related_posts;

            {
              vix::template_::Object related;
              related["title"] = "Stable performance under load";
              related["date"] = "March 24, 2026";
              related["category"] = "Performance";
              related_posts.emplace_back(related);
            }

            {
              vix::template_::Object related;
              related["title"] = "Designing for offline-first systems";
              related["date"] = "March 20, 2026";
              related["category"] = "Offline-first";
              related_posts.emplace_back(related);
            }

            {
              vix::template_::Object related;
              related["title"] = "Introducing the Vix template engine";
              related["date"] = "March 28, 2026";
              related["category"] = "Templates";
              related_posts.emplace_back(related);
            }

            ctx.set("related_posts", related_posts);

            res.render("post.html", ctx); });

  app.run(8080);
}
