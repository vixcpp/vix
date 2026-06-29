#include <vix/requests/requests.hpp>
#include "example_env.hpp"

#include <iostream>
#include <string>

namespace
{

  std::string preview(const std::string &value)
  {
    constexpr std::size_t limit = 180;
    if (value.size() <= limit)
    {
      return value;
    }
    return value.substr(0, limit) + "...";
  }
}

int main()
{
  const std::string baseUrl = vix_examples::requests::env_or("VIX_REQUESTS_BASE_URL", "https://httpbin.org");

  vix::requests::Session session;
  session.headers().set("Accept", "application/json");
  session.headers().set("User-Agent", "vix-pagination-example/1.0");
  session.timeout() = std::chrono::seconds(10);

  for (int page = 1; page <= 3; ++page)
  {
    vix::requests::RequestOptions options;
    options.params.set("page", std::to_string(page));
    options.params.set("per_page", "25");

    const auto response = session.get(baseUrl + "/get", options);
    response.raise_for_status();

    std::cout << "page " << page << " status=" << response.status_code() << '\n';
    std::cout << preview(response.text()) << "\n\n";
  }
}
