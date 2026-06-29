#include <vix/requests/requests.hpp>
#include "example_env.hpp"

#include <iostream>
#include <string>

int main()
{
  const std::string url = vix_examples::requests::env_or("VIX_REQUESTS_AUTH_URL", "https://httpbin.org/bearer");
  const std::string token = vix_examples::requests::env_or("VIX_REQUESTS_TOKEN", "demo-token");

  vix::requests::RequestOptions options;
  options.headers.set("Accept", "application/json");
  options.headers.set("Authorization", "Bearer " + token);
  options.timeout = std::chrono::seconds(10);

  const auto response = vix::requests::get(url, options);

  std::cout << "status: " << response.status_code() << '\n';
  if (!response.ok())
  {
    std::cout << "auth endpoint rejected the token or requires a real one\n";
  }
  std::cout << response.text() << '\n';
}
