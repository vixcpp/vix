#include <vix.hpp>

#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/performance/static_files.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace vix;

static std::filesystem::path source_dir()
{
    // __FILE__ = /home/softadastra/dev/tmp/static_files_app_simple.cpp
    return std::filesystem::path(__FILE__).parent_path();
}

int main()
{
    // Dossier public à côté du .cpp (stable même si vix run change le cwd)
    std::filesystem::path root = source_dir() / "public";
    std::filesystem::create_directories(root);

    // Fichiers de test
    {
        std::ofstream(root / "index.html") << "<h1>OK</h1>";
    }
    {
        std::ofstream(root / "hello.txt") << "hello";
    }

    App app;

    // Adapter Pipeline middleware -> App middleware
    app.use(vix::middleware::app::adapt_ctx(
        vix::middleware::performance::static_files(
            root,
            {
                .mount = "/",
                .index_file = "index.html",
                .add_cache_control = true,
                .cache_control = "public, max-age=3600",
                .fallthrough = true,
            })));

    app.get("/api/ping", [](Request &, Response &res)
            { res.json({"ok", true}); });

    std::cout << "Static root: " << root << "\n";
    app.run(8080);
}
