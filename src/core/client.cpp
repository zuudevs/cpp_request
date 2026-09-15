#include <cpp_request/client.hpp>

#include "http/request_serializer.hpp"
#include "http/response_parser.hpp"
#include "net/resolver.hpp"
#include "net/tcp_connection.hpp"
#include "platform/native_socket.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace cpp_request {
namespace {

[[nodiscard]] constexpr char ascii_lower(char ch) noexcept {
    return ch >= 'A' && ch <= 'Z'
        ? static_cast<char>(ch + ('a' - 'A'))
        : ch;
}

[[nodiscard]] bool ascii_iequals(
    std::string_view lhs,
    std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (ascii_lower(lhs[index]) != ascii_lower(rhs[index])) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string_view trim_ows(std::string_view value) noexcept {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
        value.remove_prefix(1);
    }
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
        value.remove_suffix(1);
    }
    return value;
}

[[nodiscard]] bool contains_token(
    std::string_view value,
    std::string_view expected) noexcept {
    std::size_t begin = 0;
    while (begin <= value.size()) {
        const std::size_t comma = value.find(',', begin);
        const std::size_t end = comma == std::string_view::npos
            ? value.size()
            : comma;
        if (ascii_iequals(trim_ows(value.substr(begin, end - begin)), expected)) {
            return true;
        }
        if (comma == std::string_view::npos) {
            break;
        }
        begin = comma + 1;
    }
    return false;
}

[[nodiscard]] bool request_wants_connection_close(const Request& request) noexcept {
    for (const auto& field : request.headers()) {
        if (ascii_iequals(field.name, "Connection")
            && contains_token(field.value, "close")) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool same_origin(
    std::string_view stored_host,
    std::uint16_t stored_port,
    std::string_view host,
    std::uint16_t port) noexcept {
    return stored_port == port && ascii_iequals(stored_host, host);
}

[[nodiscard]] std::uintptr_t encode_socket_handle(
    detail::platform::NativeSocketHandle handle) noexcept {
    return static_cast<std::uintptr_t>(handle);
}

[[nodiscard]] detail::platform::NativeSocketHandle decode_socket_handle(
    std::uintptr_t token) noexcept {
    return static_cast<detail::platform::NativeSocketHandle>(token);
}

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

Client::~Client() noexcept {
    close_reusable_connection();
}

Client::Client(Client&& other) noexcept
    : reusable_socket_token_(std::exchange(other.reusable_socket_token_, 0)),
      has_reusable_connection_(std::exchange(other.has_reusable_connection_, false)),
      reusable_host_(std::move(other.reusable_host_)),
      reusable_port_(std::exchange(other.reusable_port_, 0)),
      connect_timeout_(other.connect_timeout_),
      read_timeout_(other.read_timeout_),
      write_timeout_(other.write_timeout_) {}

Client& Client::operator=(Client&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    close_reusable_connection();

    reusable_socket_token_ = std::exchange(other.reusable_socket_token_, 0);
    has_reusable_connection_ = std::exchange(other.has_reusable_connection_, false);
    reusable_host_ = std::move(other.reusable_host_);
    reusable_port_ = std::exchange(other.reusable_port_, 0);
    connect_timeout_ = other.connect_timeout_;
    read_timeout_ = other.read_timeout_;
    write_timeout_ = other.write_timeout_;
    return *this;
}

void Client::close_reusable_connection() noexcept {
    if (has_reusable_connection_) {
        detail::platform::NativeSocket socket{
            decode_socket_handle(reusable_socket_token_)};
        socket.close();
    }

    reusable_socket_token_ = 0;
    has_reusable_connection_ = false;
    reusable_host_.clear();
    reusable_port_ = 0;
}

Result<Response> Client::request(const Request& request_value) {
    auto serialized_result = detail::http::serialize_request(request_value);
    if (!serialized_result) {
        return serialized_result.error();
    }
    auto serialized = std::move(serialized_result).value();

    detail::net::TcpConnection connection;
    const bool reuse_existing = has_reusable_connection_
        && same_origin(
            reusable_host_,
            reusable_port_,
            serialized.url.host(),
            serialized.url.port());

    if (reuse_existing) {
        connection = detail::net::TcpConnection::adopt_native_handle(
            decode_socket_handle(reusable_socket_token_));
        reusable_socket_token_ = 0;
        has_reusable_connection_ = false;
        reusable_host_.clear();
        reusable_port_ = 0;
    } else {
        close_reusable_connection();

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
        connection = std::move(connection_result).value();
    }

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

    const bool reusable = connection.connected()
        && parser.connection_reusable()
        && parser.pending_bytes().empty()
        && !request_wants_connection_close(request_value);

    Response response = parser.take_response();

    if (reusable) {
        reusable_socket_token_ = encode_socket_handle(connection.release_native_handle());
        has_reusable_connection_ = true;
        reusable_host_.assign(serialized.url.host());
        reusable_port_ = serialized.url.port();
    }

    return response;
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
