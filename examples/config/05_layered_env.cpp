#include <iostream>
#include <vix.hpp>

using namespace vix;

int main()
{
  config::Config cfg{".env.production"};

  std::cout << "SERVER_PORT=" << cfg.getServerPort() << '\n';
  std::cout << "WAF_MODE=" << cfg.getWafMode() << '\n';
  std::cout << "DATABASE_DEFAULT_HOST=" << cfg.getDbHost() << '\n';

  return 0;
}
