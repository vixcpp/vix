// ============================================================================
// delete_user.cpp — DELETE example (new Vix.cpp API)
// DELETE /users/{id} -> {"action":"delete","status":"deleted","user_id":"<id>"}
// ============================================================================

#include <vix.hpp>
#include <vix/json/Simple.hpp>
#include <string>

using namespace Vix;
namespace J = Vix::json;

int main()
{
    App app;

    // DELETE /users/{id}
    app.del("/users/{id}", [](auto &, auto &res, auto &params)
            {
        const std::string id = params["id"];

        // In a real app you'd remove the resource from DB or memory here
        res.json({
            "action",  "delete",
            "status",  "deleted",
            "user_id", id
        }); });

    app.run(8080);
    return 0;
}
