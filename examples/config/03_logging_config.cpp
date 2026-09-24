#include <iostream>
#include <vix.hpp>

using namespace vix;

int main()
{
  config::Config cfg{".env"};

  std::cout << "LOGGING_ASYNC=" << (cfg.getLogAsync() ? "true" : "false") << '\n';
  std::cout << "LOGGING_QUEUE_MAX=" << cfg.getLogQueueMax() << '\n';
  std::cout << "LOGGING_DROP_ON_OVERFLOW="
            << (cfg.getLogDropOnOverflow() ? "true" : "false") << '\n';

  return 0;
}
