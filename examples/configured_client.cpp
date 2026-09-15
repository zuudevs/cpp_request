#include <cpp_request/client.hpp>
#include <cpp_request/error.hpp>
#include <cpp_request/response_limits.hpp>

#include <chrono>
#include <iostream>

int main() {
    using namespace std::chrono_literals;

    cpp_request::Client client;
    client.set_connect_timeout(2s);
    client.set_write_timeout(5s);
    client.set_read_timeout(5s);
    client.set_follow_redirects(true);
    client.set_max_redirects(5);

    cpp_request::ResponseLimits limits;
    limits.max_head_bytes = 32 * 1024;
    limits.max_body_bytes = 8 * 1024 * 1024;
    limits.max_chunk_line_bytes = 8 * 1024;
    limits.max_trailer_bytes = 32 * 1024;
    client.set_response_limits(limits);

    const char* urls[] = {
        "http://127.0.0.1:8080/one",
        "http://127.0.0.1:8080/two",
    };

    for (const char* url : urls) {
        auto result = client.get(url);
        if (!result) {
            std::cerr << "request failed: "
                      << cpp_request::error_message(result.error().code)
                      << '\n';
            return 1;
        }

        std::cout << result.value().status_code() << ' '
                  << result.value().body() << '\n';
    }

    return 0;
}
