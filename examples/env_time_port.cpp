#include <vix/core.h>
#include <vix/utils/Env.hpp>
#include <vix/utils/Time.hpp>
#include <vix/utils/Logger.hpp>

int main()
{
    auto &log = Vix::Logger::getInstance();
    log.setPattern("[%H:%M:%S.%e] [%^%l%$] %v");
    log.setLevel(Vix::Logger::Level::INFO);

    int port = Vix::utils::env_int("PORT", 8080);

    Vix::App app;

    app.get("/now", [](auto &req, auto &res)
            { res.json(
                  {{"iso8601", Vix::utils::iso8601_now()},
                   {"ms", Vix::utils::now_ms()}}); });

    log.log(Vix::Logger::Level::INFO, "Starting on port {}", port);

    app.run(port);

    return 0;
}

// PORT=9090 ./env_time_port
// curl :9090/now
