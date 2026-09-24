#include <vix.hpp>
#include <vix/middleware/http/cookies.hpp>
using namespace vix;

int main()
{
  App app;

  app.get("/cookie", [](Request &, Response &res){
    
    vix::middleware::cookies::Cookie c;
    c.name = "hello";
    c.value = "vix";
    c.max_age = 3600;
    vix::middleware::cookies::set(res, c);
  
    res.text("cookie set"); 
  });

  app.run(8080);
}
