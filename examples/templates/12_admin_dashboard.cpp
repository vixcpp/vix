/**
 *
 *  @file 12_admin_dashboard.cpp
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
//   vix run examples/templates/12_admin_dashboard.cpp
//
// Then open:
//   http://localhost:8080
//
// Expected template files:
//   examples/templates/views/12_admin_dashboard/base.html
//   examples/templates/views/12_admin_dashboard/header.html
//   examples/templates/views/12_admin_dashboard/dashboard.html

#include <vix.hpp>

using namespace vix;

int main()
{
  App app;
  app.templates("./examples/templates/views/12_admin_dashboard");

  app.get("/", [](Request &, Response &res)
          {
            vix::template_::Context ctx;

            ctx.set("page_title", "Admin Dashboard");
            ctx.set("brand_name", "Vix Admin");
            ctx.set("admin_name", "Gaspard");
            ctx.set("admin_role", "System Administrator");

            vix::template_::Array sidebar_items;

            {
              vix::template_::Object item;
              item["label"] = "Overview";
              item["href"] = "#overview";
              item["active"] = true;
              sidebar_items.emplace_back(item);
            }

            {
              vix::template_::Object item;
              item["label"] = "Users";
              item["href"] = "#users";
              item["active"] = false;
              sidebar_items.emplace_back(item);
            }

            {
              vix::template_::Object item;
              item["label"] = "Activity";
              item["href"] = "#activity";
              item["active"] = false;
              sidebar_items.emplace_back(item);
            }

            {
              vix::template_::Object item;
              item["label"] = "Tasks";
              item["href"] = "#tasks";
              item["active"] = false;
              sidebar_items.emplace_back(item);
            }

            ctx.set("sidebar_items", sidebar_items);

            ctx.set("total_users", 1248);
            ctx.set("active_sessions", 96);
            ctx.set("monthly_requests", 68000);
            ctx.set("system_health", "Healthy");

            vix::template_::Array activities;

            {
              vix::template_::Object activity;
              activity["title"] = "New deployment completed";
              activity["time"] = "5 minutes ago";
              activity["type"] = "Deployment";
              activities.emplace_back(activity);
            }

            {
              vix::template_::Object activity;
              activity["title"] = "User account upgraded";
              activity["time"] = "18 minutes ago";
              activity["type"] = "Billing";
              activities.emplace_back(activity);
            }

            {
              vix::template_::Object activity;
              activity["title"] = "Template cache warmed";
              activity["time"] = "42 minutes ago";
              activity["type"] = "System";
              activities.emplace_back(activity);
            }

            ctx.set("activities", activities);

            vix::template_::Array tasks;

            {
              vix::template_::Object task;
              task["title"] = "Review failed payment alerts";
              task["priority"] = "High";
              task["done"] = false;
              tasks.emplace_back(task);
            }

            {
              vix::template_::Object task;
              task["title"] = "Approve new support agent";
              task["priority"] = "Medium";
              task["done"] = false;
              tasks.emplace_back(task);
            }

            {
              vix::template_::Object task;
              task["title"] = "Rotate deployment logs";
              task["priority"] = "Low";
              task["done"] = true;
              tasks.emplace_back(task);
            }

            ctx.set("tasks", tasks);

            vix::template_::Array users;

            {
              vix::template_::Object user;
              user["name"] = "Alice";
              user["email"] = "alice@example.com";
              user["role"] = "Editor";
              user["status"] = "Active";
              users.emplace_back(user);
            }

            {
              vix::template_::Object user;
              user["name"] = "Bob";
              user["email"] = "bob@example.com";
              user["role"] = "Manager";
              user["status"] = "Pending";
              users.emplace_back(user);
            }

            {
              vix::template_::Object user;
              user["name"] = "Charlie";
              user["email"] = "charlie@example.com";
              user["role"] = "Admin";
              user["status"] = "Active";
              users.emplace_back(user);
            }

            {
              vix::template_::Object user;
              user["name"] = "Diane";
              user["email"] = "diane@example.com";
              user["role"] = "Support";
              user["status"] = "Suspended";
              users.emplace_back(user);
            }

            ctx.set("users", users);

            res.render("dashboard.html", ctx); });

  app.run(8080);
}
