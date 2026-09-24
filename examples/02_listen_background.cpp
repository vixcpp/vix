#include <vix.hpp>
#include <iostream>

using namespace vix;

int main()
{
  App app;

  app.get("/health", [](Request &, Response &res)
          { res.json({"ok", true}); });

  app.listen(8080, []()
             { console.log("Server is ready and accepting connections"); });

  app.wait();
}
