#include <cpp_request/client.hpp>
#include <cpp_request/error.hpp>
#include <cpp_request/request.hpp>

#include <iostream>
#include <string>

int main() {
    cpp_request::Client client;

    std::string body = R"({"name":"cpp_request","kind":"example"})";
    cpp_request::Request request{
        cpp_request::Method::Post,
        "http://127.0.0.1:8080/items?existing=1"};

    request.headers().set("Content-Type", "application/json");
    request.headers().set("Accept", "application/json");
    request.headers().add("X-Example", "cpp_request");
    request.add_query_param("search", "low level C++");
    request.add_query_param("tag", "network/http");
    request.set_body(body);

    auto result = client.request(request);
    if (!result) {
        std::cerr << "request failed: "
                  << cpp_request::error_message(result.error().code)
                  << '\n';
        return 1;
    }

    const auto& response = result.value();
    std::cout << "HTTP status: " << response.status_code() << '\n';
    std::cout << response.body() << '\n';
    return 0;
}
