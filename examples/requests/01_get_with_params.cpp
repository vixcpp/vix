#include <vix/requests/requests.hpp>
#include "example_env.hpp"

#include <iostream>
#include <string>

int main()
{
  const std::string baseUrl = vix_examples::requests::env_or("VIX_REQUESTS_BASE_URL", "https://httpbin.org");

  vix::requests::RequestOptions options;
  options.params.set("q", "vix requests");
  options.params.set("page", "1");
  options.headers.set("Accept", "application/json");
  options.set_user_agent("vix-requests-examples/1.0");

  const auto response = vix::requests::get(baseUrl + "/get", options);
  response.raise_for_status();

  std::cout << "status: " << response.status_code() << '\n';
  std::cout << "url: " << response.url() << '\n';
  std::cout << response.text() << '\n';
}
