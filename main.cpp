
#include "crow/app.h"
constexpr int PORT = 4000;

using namespace crow;

int main()
{
    SimpleApp app;

    CROW_ROUTE(app, "/").methods("GET"_method)([]() {
        const auto page = mustache::load("index.html");
        mustache::context ctx({ { "name", "Freya" } });
        return page.render(ctx);
    });

    app.port(PORT).multithreaded().run();
}
