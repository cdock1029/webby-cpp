
#include <crow.h>
#include <exception>
#include <fmt/core.h>
#include <pqxx/pqxx>

using namespace crow;
using namespace fmt;

constexpr uint16_t PORT = 4000;
constexpr auto GET_PROPERTIES = "get_properties";
constexpr auto GET_PROPERTY = "get_property";
constexpr auto CREATE_PROPERTY = "create_property";
constexpr auto DELETE_PROPERTY = "delete_property";
constexpr auto UPDATE_PROPERTY = "update_property";

void prepare_statements(pqxx::connection& conn);

json::wvalue::list get_properties(pqxx::connection& conn);
json::wvalue get_property(pqxx::connection& conn, int property_id);

int main()
{
    pqxx::connection conn("host=/run/postgresql dbname=webby");

    prepare_statements(conn);
    SimpleApp app;

    const auto index = mustache::load("index.html");
    const auto properties = mustache::load("fragments/properties-list.html");
    CROW_ROUTE(app, "/").methods("GET"_method)([&]() {
        const mustache::context ctx{ { "properties", get_properties(conn) } };
        return index.render(ctx);
    });
    CROW_ROUTE(app, "/properties").methods("GET"_method)([&]() {
        const mustache::context ctx{ { "properties", get_properties(conn) } };
        return properties.render(ctx);
    });

    CROW_ROUTE(app, "/").methods("POST"_method)([&](const request& req) {
        const auto params = req.get_body_params();
        const std::string name = params.get("name");

        response res{ 204 };
        if (!name.empty()) {
            pqxx::work txn(conn);
            txn.exec_prepared(CREATE_PROPERTY, name);
            txn.commit();
            res.add_header("HX-Trigger", "newProperty");
        }
        return res;
    });

    CROW_ROUTE(app, "/property/<int>")
      .methods("DELETE"_method)([&](int property_id) {
          pqxx::work txn(conn);
          txn.exec_prepared(DELETE_PROPERTY, property_id);
          txn.commit();

          const mustache::context ctx(
            { { "properties", get_properties(conn) } });
          return index.render(ctx);
      });

    CROW_ROUTE(app, "/property/<int>")
      .methods("PUT"_method)([&](const request& req, int property_id) {
          const auto params = req.get_body_params();

          pqxx::work txn(conn);
          txn.exec_prepared(UPDATE_PROPERTY, property_id, params.get("name"));
          txn.commit();

          const mustache::context ctx(
            { { "properties", get_properties(conn) } });
          return index.render(ctx);
      });

    CROW_ROUTE(app, "/property/<int>/edit")
      .methods("GET"_method)([&](const request& req, int property_id) {
          const auto params = req.get_body_params();

          static auto edit_fragment = mustache::load("property-edit.html");

          const mustache::context ctx(
            { "property", get_property(conn, property_id) });
          return edit_fragment.render(ctx);
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
    conn.prepare(UPDATE_PROPERTY,
                 "UPDATE properties set name = $2 where id = $1");

    conn.prepare(GET_PROPERTY,
                 "SELECT * FROM properties where id = $1 limit 1");
}

auto get_properties(pqxx::connection& conn) -> json::wvalue::list
{

    pqxx::nontransaction non(conn);
    auto properties = non.exec_prepared(GET_PROPERTIES);

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

auto get_property(pqxx::connection& conn, int property_id) -> json::wvalue
{
    pqxx::nontransaction non(conn);
    const pqxx::result property = non.exec_params(GET_PROPERTY, property_id);

    if (property.empty()) {
        throw std::runtime_error("Property not found: " +
                                 std::to_string(property_id));
    }

    auto row = property.front();
    auto [id, name] = row.as<int, const char*>();

    return json::wvalue::object{
        { "id", id },
        { "name", name },
    };
}
