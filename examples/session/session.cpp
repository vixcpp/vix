#include <vix.hpp>
#include <vix/middleware/app/presets.hpp>
using namespace vix;

int main()
{
  App app;

  app.use(middleware::app::adapt_ctx(
    middleware::auth::session({.secret = "dev"})
  ));

  app.get("/session", [](Request &req, Response &res){
    auto &s = req.state<middleware::auth::Session>();

    int n = s.get("n") ? std::stoi(*s.get("n")) : 0;
    s.set("n", std::to_string(++n));

    res.text("n=" + std::to_string(n)); 
  });

  app.run(8080);
}
