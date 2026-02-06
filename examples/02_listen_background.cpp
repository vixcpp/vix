#include <vix.hpp>
#include <iostream>

using namespace vix;

int main()
{
  App app;

  app.get("/health", [](Request &, Response &res)
          { res.json({"ok", true}); });

  // Lance le serveur en background
  app.listen(8080, []()
             { std::cout << "Server is ready and accepting connections\n"; });

  // Attente propre
  app.wait();
}
