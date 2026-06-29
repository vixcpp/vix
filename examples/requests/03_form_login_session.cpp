#include <vix/requests/requests.hpp>
#include "example_env.hpp"

#include <iostream>
#include <string>

int main()
{
  const std::string baseUrl = vix_examples::requests::env_or("VIX_REQUESTS_BASE_URL", "https://httpbin.org");

  vix::requests::Session session;
  session.headers().set("Accept", "application/json");
  session.headers().set("User-Agent", "vix-session-example/1.0");
  session.timeout() = std::chrono::seconds(10);

  const auto login = session.post(
      baseUrl + "/post",
      vix::requests::form_body({{"username", "demo"}, {"password", "secret"}}));
  login.raise_for_status();

  const auto setCookie = session.get(baseUrl + "/cookies/set/session/demo-token");
  setCookie.raise_for_status();

  const auto profile = session.get(baseUrl + "/cookies");
  profile.raise_for_status();

  std::cout << "login status: " << login.status_code() << '\n';
  std::cout << "session cookies response:\n" << profile.text() << '\n';
}
