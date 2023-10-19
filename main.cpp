
#include "crow/app.h"
#include "crow/json.h"
#include "crow/query_string.h"
#include <exception>
#include <fmt/core.h>
#include <pqxx/pqxx>
constexpr int PORT = 4000;

using namespace crow;
using namespace fmt;

constexpr auto GET_PROPERTIES = "get_properties";
constexpr auto CREATE_PROPERTY = "create_property";
constexpr auto DELETE_PROPERTY = "delete_property";

void prepare_statements(pqxx::connection& conn);

json::wvalue::list get_properties(pqxx::connection& conn);

int main()
{

    pqxx::connection conn("host=/run/postgresql dbname=webby");

    prepare_statements(conn);

    SimpleApp app;

    const auto page = mustache::load("index.html");
    CROW_ROUTE(app, "/").methods("GET"_method)([&]() {
        try {

            mustache::context ctx{ { "properties", get_properties(conn) } };

            return page.render(ctx);

        } catch (const std::exception& e) {
            return page.render({ { "error", e.what() } });
        }
    });

    CROW_ROUTE(app, "/").methods("POST"_method)([&](const request& req) {
        const auto params = req.get_body_params();
        const std::string name = params.get("name");

        if (!name.empty()) {
            pqxx::work txn(conn);
            txn.exec_prepared(CREATE_PROPERTY, name);
            txn.commit();
        }

        mustache::context ctx({ { "properties", get_properties(conn) } });
        return page.render(ctx);
    });

    CROW_ROUTE(app, "/property/<int>")
      .methods("DELETE"_method)([&](int property_id) {
          pqxx::work txn(conn);
          txn.exec_prepared(DELETE_PROPERTY, property_id);
          txn.commit();

          mustache::context ctx({ { "properties", get_properties(conn) } });
          return page.render(ctx);
      });

    try {
        app.port(PORT).multithreaded().run();
    } catch (std::exception& e) {
        print(stderr, "Error: {}\n", e.what());
        return 1;
    }
}

void prepare_statements(pqxx::connection& conn)
{
    conn.prepare(GET_PROPERTIES, "SELECT * FROM properties order by name");
    conn.prepare(CREATE_PROPERTY, "INSERT INTO properties (name) VALUES ($1)");
    conn.prepare(DELETE_PROPERTY, "DELETE from properties where id = $1");
}

auto get_properties(pqxx::connection& conn) -> json::wvalue::list
{

    pqxx::nontransaction non(conn);
    pqxx::result properties = non.exec_prepared(GET_PROPERTIES);

    json::wvalue::list properties_list;

    for (const auto& row : properties) {
        auto [id, name] = row.as<int, const char*>();

        json::wvalue::object property{
            { "id", id },
            { "name", name },
        };
        properties_list.emplace_back(std::move(property));
    }
    return properties_list;
}