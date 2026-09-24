#include <iostream>
#include <vix.hpp>

using namespace vix;

int main()
{
  config::Config cfg{".env"};

  std::cout << "DATABASE_DEFAULT_HOST=" << cfg.getDbHost() << '\n';
  std::cout << "DATABASE_DEFAULT_USER=" << cfg.getDbUser() << '\n';
  std::cout << "DATABASE_DEFAULT_NAME=" << cfg.getDbName() << '\n';
  std::cout << "DATABASE_DEFAULT_PORT=" << cfg.getDbPort() << '\n';

  return 0;
}
