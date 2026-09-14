#include <cpp_request/client.hpp>

#include "http/request_serializer.hpp"
#include "http/response_parser.hpp"
#include "net/resolver.hpp"
#include "net/tcp_connection.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <string_view>
#include <utility>

namespace cpp_request {
namespace {

[[nodiscard]] std::chrono::milliseconds remaining_timeout(
    std::chrono::steady_clock::time_point deadline) noexcept {
    const auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
        return std::chrono::milliseconds{0};
    }

    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - now);
    if (remaining.count() <= 0) {
        remaining = std::chrono::milliseconds{1};
    }
    return remaining;
}

[[nodiscard]] Request make_request(
    Method method,
    std::string_view url,
    std::string_view body = {}) noexcept {
    Request request{method, url};
    request.set_body(body);
    return request;
}

} // namespace

Result<Response> Client::request(const Request& request_value) {
    auto serialized_result = detail::http::serialize_request(request_value);
    if (!serialized_result) {
        return serialized_result.error();
    }
    auto serialized = std::move(serialized_result).value();

    auto endpoints_result = detail::net::Resolver::resolve(
        serialized.url.host(),
        serialized.url.port());
    if (!endpoints_result) {
        return endpoints_result.error();
    }

    auto connection_result = detail::net::TcpConnection::connect(
        endpoints_result.value(),
        connect_timeout_);
    if (!connection_result) {
        return connection_result.error();
    }
    auto connection = std::move(connection_result).value();

    const auto write_deadline = std::chrono::steady_clock::now() + write_timeout_;

    auto head_write = connection.write_all(
        serialized.head,
        remaining_timeout(write_deadline));
    if (!head_write) {
        return head_write.error();
    }

    if (!serialized.body.empty()) {
        auto body_write = connection.write_all(
            serialized.body,
            remaining_timeout(write_deadline));
        if (!body_write) {
            return body_write.error();
        }
    }

    detail::http::ResponseParser parser{request_value.method()};
    std::array<char, 16 * 1024> read_buffer{};

    while (!parser.complete()) {
        auto read_result = connection.read_some(
            read_buffer.data(),
            read_buffer.size(),
            read_timeout_);

        if (!read_result) {
            if (read_result.error().code == ErrorCode::ConnectionClosed) {
                auto eof_result = parser.finish_eof();
                if (!eof_result) {
                    return eof_result.error();
                }
                if (parser.complete()) {
                    break;
                }
            }
            return read_result.error();
        }

        auto parse_result = parser.feed(std::string_view{
            read_buffer.data(),
            read_result.value()});
        if (!parse_result) {
            return parse_result.error();
        }
    }

    return parser.take_response();
}

Result<Response> Client::get(std::string_view url) {
    return request(make_request(Method::Get, url));
}

Result<Response> Client::head(std::string_view url) {
    return request(make_request(Method::Head, url));
}

Result<Response> Client::post(std::string_view url, std::string_view body) {
    return request(make_request(Method::Post, url, body));
}

Result<Response> Client::put(std::string_view url, std::string_view body) {
    return request(make_request(Method::Put, url, body));
}

Result<Response> Client::patch(std::string_view url, std::string_view body) {
    return request(make_request(Method::Patch, url, body));
}

Result<Response> Client::del(std::string_view url) {
    return request(make_request(Method::Delete, url));
}

Result<Response> get(std::string_view url) {
    Client client;
    return client.get(url);
}

Result<Response> head(std::string_view url) {
    Client client;
    return client.head(url);
}

Result<Response> post(std::string_view url, std::string_view body) {
    Client client;
    return client.post(url, body);
}

Result<Response> put(std::string_view url, std::string_view body) {
    Client client;
    return client.put(url, body);
}

Result<Response> patch(std::string_view url, std::string_view body) {
    Client client;
    return client.patch(url, body);
}

Result<Response> del(std::string_view url) {
    Client client;
    return client.del(url);
}

} // namespace cpp_request
