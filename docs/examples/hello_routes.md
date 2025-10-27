# Example — hello_routes

Minimal GET routes and path params.

```cpp
#include <vix.hpp>
using namespace Vix;

int main()
{
    App app;

    app.get("/hello", [](auto &, auto &res)
           { res.json({"message", "Hello, Vix!"}); });

    app.run(8080);
}
```
