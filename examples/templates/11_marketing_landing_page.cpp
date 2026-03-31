/**
 *
 *  @file 11_marketing_landing_page.cpp
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
//   vix run examples/templates/11_marketing_landing_page.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template files:
//   examples/templates/views/11_marketing_landing_page/base.html
//   examples/templates/views/11_marketing_landing_page/header.html
//   examples/templates/views/11_marketing_landing_page/index.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/11_marketing_landing_page");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;

            ctx.set("page_title", "Vix Deploy");
            ctx.set("brand_name", "Vix Deploy");
            ctx.set("nav_docs", "Docs");
            ctx.set("nav_pricing", "Pricing");
            ctx.set("nav_download", "Download");

            ctx.set("hero_title", "Deploy real-world apps with confidence");
            ctx.set("hero_subtitle",
                    "Vix Deploy is built for modern applications that need reliability, predictable performance, and infrastructure designed for imperfect conditions.");
            ctx.set("primary_cta", "Get started");
            ctx.set("secondary_cta", "Read the docs");

            ctx.set("stat_1_value", "99.95%");
            ctx.set("stat_1_label", "deployment reliability");
            ctx.set("stat_2_value", "< 5 min");
            ctx.set("stat_2_label", "to first deploy");
            ctx.set("stat_3_value", "1 runtime");
            ctx.set("stat_3_label", "simple workflow");

            vix::template_::Array features;

            {
              vix::template_::Object feature;
              feature["title"] = "Predictable deployments";
              feature["description"] =
                  "Keep release behavior consistent with a workflow designed for stability and repeatability.";
              features.emplace_back(feature);
            }

            {
              vix::template_::Object feature;
              feature["title"] = "Offline-first mindset";
              feature["description"] =
                  "Built for environments where networks are not always fast, clean, or available.";
              features.emplace_back(feature);
            }

            {
              vix::template_::Object feature;
              feature["title"] = "Simple developer flow";
              feature["description"] =
                  "Reduce friction from local development to deployment with one clear product path.";
              features.emplace_back(feature);
            }

            ctx.set("features", features);

            vix::template_::Array plans;

            {
              vix::template_::Object plan;
              plan["name"] = "Starter";
              plan["price"] = "$0";
              plan["description"] = "For personal projects and experimentation.";
              plan["cta"] = "Start free";
              plans.emplace_back(plan);
            }

            {
              vix::template_::Object plan;
              plan["name"] = "Pro";
              plan["price"] = "$29";
              plan["description"] = "For production apps and growing teams.";
              plan["cta"] = "Choose Pro";
              plans.emplace_back(plan);
            }

            {
              vix::template_::Object plan;
              plan["name"] = "Enterprise";
              plan["price"] = "Custom";
              plan["description"] = "For organizations with advanced deployment needs.";
              plan["cta"] = "Contact sales";
              plans.emplace_back(plan);
            }

            ctx.set("plans", plans);

            res.render("index.html", ctx); });

  app.run(8080);
}
