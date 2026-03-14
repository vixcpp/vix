#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res)
          { res.text("Dynamic port example"); });

  // Port 0 → choisi par l’OS
  app.listen_port(0, [](int port)
                  { console.log("Listening on http://localhost:", port); });

  app.wait();
}
