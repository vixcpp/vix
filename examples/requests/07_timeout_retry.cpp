#include <vix/requests/requests.hpp>
#include "example_env.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main()
{
  const std::string url = vix_examples::requests::env_or("VIX_REQUESTS_RETRY_URL", "https://httpbin.org/delay/1");

  vix::requests::RequestOptions options;
  options.timeout.set_connect(std::chrono::seconds(2));
  options.timeout.set_read(std::chrono::milliseconds(500));
  options.timeout.set_total(std::chrono::seconds(3));

  for (int attempt = 1; attempt <= 3; ++attempt)
  {
    try
    {
      const auto response = vix::requests::get(url, options);
      response.raise_for_status();
      std::cout << "attempt " << attempt << " succeeded with status "
                << response.status_code() << '\n';
      return 0;
    }
    catch (const vix::requests::TimeoutException &error)
    {
      std::cerr << "attempt " << attempt << " timed out: " << error.what() << '\n';
    }
    catch (const vix::requests::ConnectionException &error)
    {
      std::cerr << "attempt " << attempt << " connection error: " << error.what() << '\n';
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100 * attempt));
  }

  std::cerr << "all retry attempts failed\n";
  return 1;
}
