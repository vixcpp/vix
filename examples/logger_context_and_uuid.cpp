#include <vix/core.h>
#include <vix/utils/Logger.hpp>
#include <vix/utils/UUID.hpp>
#include <nlohmann/json.hpp>
#include <string>

using Vix::Logger;

int main()
{
    auto &log = Logger::getInstance();
    log.setPattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    log.setLevel(Logger::Level::INFO);
    log.setAsync(true);

    Vix::App app;

    app.get("/trace", [](auto &req, auto &res)
            {
        Logger::Context cx;
        cx.request_id = Vix::utils::uuid4();
        cx.module = "trace_handler";
        Logger::getInstance().setContext(cx);

        std::string path(req.target().data(), req.target().size());
        std::string method(req.method_string().data(), req.method_string().size());

        Logger::getInstance().logf(
            Logger::Level::INFO,
            "Incoming request",
            "path", path.c_str(),
            "method", method.c_str());

        res.json(nlohmann::json{{"rid", cx.request_id}, {"ok", true}}); });

    app.run(8080);
    return 0;
}