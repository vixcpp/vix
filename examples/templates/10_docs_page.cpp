/**
 *
 *  @file 10_docs_page.cpp
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
//   vix run examples/templates/10_docs_page.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template files:
//   examples/templates/views/10_docs_page/base.html
//   examples/templates/views/10_docs_page/header.html
//   examples/templates/views/10_docs_page/docs.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/10_docs_page");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;

            ctx.set("page_title", "Vix Docs - Templates");
            ctx.set("site_name", "Vix Documentation");
            ctx.set("site_tagline", "Guides, references, and examples for Vix.cpp.");

            ctx.set("doc_title", "Templates");
            ctx.set("doc_description",
                    "Learn how to configure template rendering, pass data from C++ to HTML, and build reusable server-rendered pages with Vix.");
            ctx.set("current_section", "Templates");

            vix::template_::Array sidebar;

            {
              vix::template_::Object item;
              item["title"] = "Getting Started";
              item["href"] = "#getting-started";
              item["active"] = false;
              sidebar.emplace_back(item);
            }

            {
              vix::template_::Object item;
              item["title"] = "Templates";
              item["href"] = "#templates";
              item["active"] = true;
              sidebar.emplace_back(item);
            }

            {
              vix::template_::Object item;
              item["title"] = "Routing";
              item["href"] = "#routing";
              item["active"] = false;
              sidebar.emplace_back(item);
            }

            {
              vix::template_::Object item;
              item["title"] = "Responses";
              item["href"] = "#responses";
              item["active"] = false;
              sidebar.emplace_back(item);
            }

            ctx.set("sidebar", sidebar);

            vix::template_::Array concepts;
            concepts.emplace_back("Configure a template directory with app.templates(\"./views\")");
            concepts.emplace_back("Render HTML with res.render(\"file.html\", ctx)");
            concepts.emplace_back("Pass strings, numbers, arrays, and objects from C++");
            concepts.emplace_back("Use layouts, includes, loops, and conditionals");
            ctx.set("concepts", concepts);

            ctx.set("code_example",
                    "#include <vix.hpp>\\n"
                    "using namespace vix;\\n\\n"
                    "int main()\\n"
                    "{\\n"
                    "  App app;\\n"
                    "  app.templates(\\\"./views\\\");\\n\\n"
                    "  app.get(\\\"/\\\", [](Request &, Response &res)\\n"
                    "  {\\n"
                    "    vix::template_::Context ctx;\\n"
                    "    ctx.set(\\\"title\\\", \\\"Hello Vix\\\");\\n"
                    "    res.render(\\\"index.html\\\", ctx);\\n"
                    "  });\\n\\n"
                    "  app.run(8080);\\n"
                    "}");

            vix::template_::Array quick_links;

            {
              vix::template_::Object link;
              link["label"] = "Template examples";
              link["href"] = "https://vixcpp.com/docs/examples";
              quick_links.emplace_back(link);
            }

            {
              vix::template_::Object link;
              link["label"] = "CLI docs";
              link["href"] = "https://vixcpp.com/docs/modules/cli";
              quick_links.emplace_back(link);
            }

            {
              vix::template_::Object link;
              link["label"] = "Registry";
              link["href"] = "https://vixcpp.com/registry";
              quick_links.emplace_back(link);
            }

            ctx.set("quick_links", quick_links);

            res.render("docs.html", ctx); });

  app.run(8080);
}
