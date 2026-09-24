#include <vix/requests/requests.hpp>
#include "example_env.hpp"

#include <iostream>
#include <string>

int main()
{
  const std::string url = vix_examples::requests::env_or("VIX_REQUESTS_ERROR_URL", "https://httpbin.org/status/404");

  try
  {
    const auto response = vix::requests::get(url);
    response.raise_for_status();
    std::cout << response.text() << '\n';
  }
  catch (const vix::requests::HttpException &error)
  {
    std::cerr << "HTTP error " << error.status_code() << ": " << error.what() << '\n';
  }
  catch (const vix::requests::TimeoutException &error)
  {
    std::cerr << "timeout: " << error.what() << '\n';
  }
  catch (const vix::requests::ConnectionException &error)
  {
    std::cerr << "connection failed: " << error.what() << '\n';
  }
  catch (const vix::requests::RequestException &error)
  {
    std::cerr << "request failed: " << error.what() << '\n';
  }
}
