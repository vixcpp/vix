#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  // Serve a single file
  app.get("/", [](Request &, Response &res)
          { res.file("public/index.html"); });

  app.run(8080);
}
