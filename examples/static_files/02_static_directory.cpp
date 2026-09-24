#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.static_dir("public");

  app.run(8080);
}

// curl /
// curl /index.html
// curl /style.css
