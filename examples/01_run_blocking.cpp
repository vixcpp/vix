#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res)
          { res.text("Hello Vix"); });

  // Bloque le thread courant
  app.run(8080);
}
