#include <vix/requests/requests.hpp>
#include "example_env.hpp"

#include <fstream>
#include <iostream>
#include <string>

int main()
{
  const std::string url = vix_examples::requests::env_or("VIX_REQUESTS_DOWNLOAD_URL", "https://httpbin.org/bytes/1024");
  const std::string outputPath = vix_examples::requests::env_or("VIX_REQUESTS_OUTPUT", "requests_download.bin");

  vix::requests::RequestOptions options;
  options.timeout = std::chrono::seconds(15);

  const auto response = vix::requests::get(url, options);
  response.raise_for_status();

  std::ofstream out(outputPath, std::ios::binary);
  out.write(response.body().data(), static_cast<std::streamsize>(response.body().size()));

  std::cout << "downloaded " << response.size() << " bytes to " << outputPath << '\n';
}
