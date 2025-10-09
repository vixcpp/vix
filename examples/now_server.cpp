// ============================================================================
// now_server.cpp — Demo route: current time in ISO 8601 and milliseconds
// GET /now -> {"iso8601":"2025-10-09T10:34:12.123Z","ms":1696848852123}
// ============================================================================

#include <vix.hpp>
#include <vix/utils/Env.hpp>
#include <vix/utils/Time.hpp>
#include <vix/utils/Logger.hpp>

using namespace Vix;

int main()
{
    auto &log = Logger::getInstance();
    log.setPattern("[%H:%M:%S.%e] [%^%l%$] %v");
    log.setLevel(Logger::Level::INFO);

    const int port = utils::env_int("PORT", 8080);

    App app;

    // GET /now → returns current ISO 8601 timestamp and epoch ms
    app.get("/now", [](auto &, auto &res)
            { res.json({"iso8601", utils::iso8601_now(),
                        "ms", static_cast<long long>(utils::now_ms())}); });

    log.log(Logger::Level::INFO, "Starting server on port {}", port);

    app.run(port);
    return 0;
}
