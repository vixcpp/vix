# Example — delete_user.cpp

```cpp
#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <string>

using namespace vix;
namespace J = vix::json;

int main()
{
    App app;

    // DELETE /users/{id}
    app.del("/users/{id}", [](Request &req, Response &res)
            {
        const std::string id = req.param("id");

        // In a real app you'd remove the resource from DB or memory here
        res.json({
            "action",  "delete",
            "status",  "deleted",
            "user_id", id
        }); });

    app.run(8080);
    return 0;
}
```
