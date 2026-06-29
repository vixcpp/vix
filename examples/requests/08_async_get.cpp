#include <vix/requests/requests.hpp>
#include <vix/async/core/io_context.hpp>
#include <vix/async/core/task.hpp>
#include "example_env.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace
{

  vix::async::core::task<void> app(vix::async::core::io_context &ctx)
  {
    const std::string baseUrl = vix_examples::requests::env_or("VIX_REQUESTS_BASE_URL", "https://httpbin.org");

    vix::requests::RequestOptions options;
    options.params.set("mode", "async");
    options.headers.set("Accept", "application/json");

    auto requestTask = vix::requests::async_get(ctx, baseUrl + "/get", options);
    const auto response = co_await requestTask;

    std::cout << "async status: " << response.status_code() << '\n';
    std::cout << response.text() << '\n';

    ctx.stop();
    co_return;
  }
}

namespace
{
  vix::async::core::task<void> drive_app(
      vix::async::core::io_context &ctx,
      std::exception_ptr &error)
  {
    try
    {
      auto task = app(ctx);
      co_await task;
    }
    catch (...)
    {
      error = std::current_exception();
      ctx.stop();
    }

    co_return;
  }
}

int main()
{
  vix::async::core::io_context ctx;
  std::exception_ptr error;

  auto runner = drive_app(ctx, error);
  ctx.post(runner.handle());
  ctx.run();

  if (error)
  {
    std::rethrow_exception(error);
  }
}
