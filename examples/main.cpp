#include <vix/core.h>

int main()
{
    try
    {
        Vix::Config &config = Vix::Config::getInstance();
        config.loadConfig();
        Vix::HTTPServer server(config);

        server.run();
    }
    catch (const std::exception &e)
    {
        spdlog::error("Critical error: {}", e.what());
    }

    return 0;
}
