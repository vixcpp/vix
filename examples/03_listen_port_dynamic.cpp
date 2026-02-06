#include <vix.hpp>
#include <iostream>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res)
          { res.text("Dynamic port example"); });

  // Port 0 → choisi par l’OS
  app.listen_port(0, [](int port)
                  { std::cout << "Listening on http://localhost:" << port << "\n"; });

  app.wait();
}
