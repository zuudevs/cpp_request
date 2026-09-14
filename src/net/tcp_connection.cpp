#include "net/tcp_connection.hpp"

#include "platform/network_runtime.hpp"
#include "platform/socket_connect.hpp"

#include <chrono>

namespace cpp_request::detail::net {

Result<TcpConnection> TcpConnection::connect(
    const std::vector<Endpoint>& endpoints,
    std::chrono::milliseconds timeout) {
    if (timeout.count() <= 0) {
        return Error{ErrorCode::ConnectTimeout};
    }

    if (endpoints.empty()) {
        return Error{ErrorCode::ConnectFailed};
    }

    const auto runtime = platform::ensure_network_runtime();
    if (!runtime.ok) {
        return Error{ErrorCode::SocketCreateFailed, runtime.native_code};
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    bool created_socket = false;
    bool saw_valid_endpoint = false;
    int last_native_error = 0;

    for (const Endpoint& endpoint : endpoints) {
        if (!endpoint.valid()) {
            continue;
        }

        saw_valid_endpoint = true;
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return Error{ErrorCode::ConnectTimeout, last_native_error};
        }

        int create_error = 0;
        auto socket = platform::create_tcp_socket(
            endpoint.family(),
            endpoint.socket_type(),
            endpoint.protocol(),
            create_error);

        if (!socket) {
            last_native_error = create_error;
            continue;
        }

        created_socket = true;
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());

        const auto attempt = platform::connect_with_timeout(
            socket.get(),
            endpoint.native_address(),
            endpoint.native_address_size(),
            remaining);

        if (attempt.status == platform::ConnectStatus::Connected) {
            return TcpConnection{std::move(socket)};
        }

        last_native_error = attempt.native_code;
        if (attempt.status == platform::ConnectStatus::TimedOut) {
            return Error{ErrorCode::ConnectTimeout, last_native_error};
        }
    }

    if (!saw_valid_endpoint) {
        return Error{ErrorCode::ConnectFailed};
    }

    if (!created_socket) {
        return Error{ErrorCode::SocketCreateFailed, last_native_error};
    }

    return Error{ErrorCode::ConnectFailed, last_native_error};
}

} // namespace cpp_request::detail::net
