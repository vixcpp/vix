#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/*", [](Request &req, Response &res)
          {
    std::string path = "public" + req.path();

    // Override cache policy
    res.header("Cache-Control", "public, max-age=86400");

    res.file(path); });

  app.run(8080);
}
