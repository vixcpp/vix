/**
 *
 *  @file 07_shop_dashboard.cpp
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
//   vix run examples/templates/07_shop_dashboard.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template files:
//   examples/templates/views/07_shop_dashboard/base.html
//   examples/templates/views/07_shop_dashboard/header.html
//   examples/templates/views/07_shop_dashboard/dashboard.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/07_shop_dashboard");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;

            ctx.set("page_title", "Softadastra Shop Dashboard");
            ctx.set("shop_name", "Softadastra Store");
            ctx.set("owner_name", "Gaspard");
            ctx.set("currency", "USD");
            ctx.set("is_shop_open", true);
            ctx.set("total_products", 128);
            ctx.set("total_orders", 42);
            ctx.set("monthly_revenue", 2450);

            vix::template_::Array orders;

            {
              vix::template_::Object order;
              order["id"] = "ORD-1001";
              order["customer"] = "Alice";
              order["status"] = "Paid";
              order["amount"] = 120;
              orders.emplace_back(order);
            }

            {
              vix::template_::Object order;
              order["id"] = "ORD-1002";
              order["customer"] = "Bob";
              order["status"] = "Pending";
              order["amount"] = 75;
              orders.emplace_back(order);
            }

            {
              vix::template_::Object order;
              order["id"] = "ORD-1003";
              order["customer"] = "Charlie";
              order["status"] = "Shipped";
              order["amount"] = 210;
              orders.emplace_back(order);
            }

            ctx.set("recent_orders", orders);

            vix::template_::Array alerts;
            alerts.emplace_back("2 products are low in stock");
            alerts.emplace_back("1 order requires manual review");
            ctx.set("alerts", alerts);

            res.render("dashboard.html", ctx); });

  app.run(8080);
}
