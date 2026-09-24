#include <vix/requests/requests.hpp>
#include "example_env.hpp"

#include <iostream>
#include <string>

int main()
{
  const std::string baseUrl = vix_examples::requests::env_or("VIX_REQUESTS_BASE_URL", "https://httpbin.org");

  vix::requests::RequestOptions options;
  options.headers.set("Accept", "application/json");
  options.timeout = std::chrono::seconds(10);

  const auto response = vix::requests::post(
      baseUrl + "/post",
      vix::requests::json_body(R"({"name":"Vix","kind":"example","active":true})"),
      options);

  response.raise_for_status();
  std::cout << "created/sent JSON, status: " << response.status_code() << '\n';
  std::cout << response.text() << '\n';
}
