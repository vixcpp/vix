#include <iostream>
#include <vix.hpp>

using namespace vix;

int main()
{
  config::Config cfg{".env"};

  std::cout << "SERVER_PORT=" << cfg.getServerPort() << '\n';

  App app;

  app.get("/", [](Request &, Response &res)
          { res.send("Hello from config example"); });

  app.run(cfg.getServerPort());
}
