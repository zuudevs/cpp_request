#include <cpp_request/client.hpp>
#include <cpp_request/error.hpp>

#include <iostream>

int main() {
    auto result = cpp_request::get("http://127.0.0.1:8080/");
    if (!result) {
        const auto error = result.error();
        std::cerr << "request failed: "
                  << cpp_request::error_message(error.code);
        if (error.native_code != 0) {
            std::cerr << " (native=" << error.native_code << ')';
        }
        std::cerr << '\n';
        return 1;
    }

    const auto& response = result.value();
    std::cout << "HTTP status: " << response.status_code() << '\n';
    std::cout << response.body() << '\n';
    return 0;
}
