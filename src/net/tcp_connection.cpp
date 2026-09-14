#include "net/tcp_connection.hpp"

#include "platform/network_runtime.hpp"
#include "platform/socket_connect.hpp"
#include "platform/socket_io.hpp"
#include "platform/socket_mode.hpp"

#include <chrono>

namespace cpp_request::detail::net {
namespace {

std::chrono::milliseconds remaining_timeout(
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

} // namespace

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
        const auto remaining = remaining_timeout(deadline);

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

Result<std::size_t> TcpConnection::write_all(
    std::string_view data,
    std::chrono::milliseconds timeout) {
    if (!connected()) {
        return Error{ErrorCode::ConnectionClosed};
    }

    if (data.empty()) {
        return std::size_t{0};
    }

    if (timeout.count() <= 0) {
        close();
        return Error{ErrorCode::WriteTimeout};
    }

    int native_code = 0;
    if (!platform::set_socket_nonblocking(socket_.get(), true, native_code)) {
        close();
        return Error{ErrorCode::WriteFailed, native_code};
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::size_t total_written = 0;

    while (total_written < data.size()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            close();
            return Error{ErrorCode::WriteTimeout};
        }

        const auto attempt = platform::send_some(
            socket_.get(),
            data.data() + total_written,
            data.size() - total_written);

        if (attempt.transferred > 0) {
            total_written += static_cast<std::size_t>(attempt.transferred);
            continue;
        }

        if (attempt.transferred == 0) {
            close();
            return Error{ErrorCode::ConnectionClosed};
        }

        native_code = attempt.native_code;
        if (platform::socket_error_is_interrupted(native_code)) {
            continue;
        }
        if (platform::socket_error_is_connection_closed(native_code)) {
            close();
            return Error{ErrorCode::ConnectionClosed, native_code};
        }
        if (!platform::socket_error_is_would_block(native_code)) {
            close();
            return Error{ErrorCode::WriteFailed, native_code};
        }

        const auto remaining = remaining_timeout(deadline);
        if (remaining.count() <= 0) {
            close();
            return Error{ErrorCode::WriteTimeout};
        }

        const auto wait = platform::wait_socket_writable(socket_.get(), remaining);
        if (wait.status == platform::SocketWaitStatus::Ready) {
            continue;
        }
        if (wait.status == platform::SocketWaitStatus::Failed
            && platform::socket_error_is_interrupted(wait.native_code)) {
            continue;
        }

        close();
        if (wait.status == platform::SocketWaitStatus::TimedOut) {
            return Error{ErrorCode::WriteTimeout, wait.native_code};
        }
        if (platform::socket_error_is_connection_closed(wait.native_code)) {
            return Error{ErrorCode::ConnectionClosed, wait.native_code};
        }
        return Error{ErrorCode::WriteFailed, wait.native_code};
    }

    if (!platform::set_socket_nonblocking(socket_.get(), false, native_code)) {
        close();
        return Error{ErrorCode::WriteFailed, native_code};
    }

    return total_written;
}

Result<std::size_t> TcpConnection::read_some(
    char* buffer,
    std::size_t capacity,
    std::chrono::milliseconds timeout) {
    if (!connected()) {
        return Error{ErrorCode::ConnectionClosed};
    }

    if (capacity == 0) {
        return std::size_t{0};
    }

    if (buffer == nullptr) {
        return Error{ErrorCode::ReadFailed};
    }

    if (timeout.count() <= 0) {
        close();
        return Error{ErrorCode::ReadTimeout};
    }

    int native_code = 0;
    if (!platform::set_socket_nonblocking(socket_.get(), true, native_code)) {
        close();
        return Error{ErrorCode::ReadFailed, native_code};
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;

    for (;;) {
        if (std::chrono::steady_clock::now() >= deadline) {
            close();
            return Error{ErrorCode::ReadTimeout};
        }

        const auto attempt = platform::receive_some(
            socket_.get(),
            buffer,
            capacity);

        if (attempt.transferred > 0) {
            if (!platform::set_socket_nonblocking(socket_.get(), false, native_code)) {
                close();
                return Error{ErrorCode::ReadFailed, native_code};
            }
            return static_cast<std::size_t>(attempt.transferred);
        }

        if (attempt.transferred == 0) {
            close();
            return Error{ErrorCode::ConnectionClosed};
        }

        native_code = attempt.native_code;
        if (platform::socket_error_is_interrupted(native_code)) {
            continue;
        }
        if (platform::socket_error_is_connection_closed(native_code)) {
            close();
            return Error{ErrorCode::ConnectionClosed, native_code};
        }
        if (!platform::socket_error_is_would_block(native_code)) {
            close();
            return Error{ErrorCode::ReadFailed, native_code};
        }

        const auto remaining = remaining_timeout(deadline);
        if (remaining.count() <= 0) {
            close();
            return Error{ErrorCode::ReadTimeout};
        }

        const auto wait = platform::wait_socket_readable(socket_.get(), remaining);
        if (wait.status == platform::SocketWaitStatus::Ready) {
            continue;
        }
        if (wait.status == platform::SocketWaitStatus::Failed
            && platform::socket_error_is_interrupted(wait.native_code)) {
            continue;
        }

        close();
        if (wait.status == platform::SocketWaitStatus::TimedOut) {
            return Error{ErrorCode::ReadTimeout, wait.native_code};
        }
        if (platform::socket_error_is_connection_closed(wait.native_code)) {
            return Error{ErrorCode::ConnectionClosed, wait.native_code};
        }
        return Error{ErrorCode::ReadFailed, wait.native_code};
    }
}

} // namespace cpp_request::detail::net
