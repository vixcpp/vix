#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/", [](Request &, Response &res)
          { res.text("Hello Vix"); });

  app.run(8080);
}
