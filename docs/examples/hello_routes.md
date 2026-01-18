# Example — hello_routes

Minimal GET routes and path params.

```cpp
#include <vix.hpp>
using namespace vix;

int main()
{
    App app;

    app.get("/hello", [](Request &, Response &res)
           { res.json({"message", "Hello, Vix!"}); });

    app.run(8080);
}
```
