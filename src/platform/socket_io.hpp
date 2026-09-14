#pragma once

#include "platform/native_socket.hpp"

#include <chrono>
#include <cstddef>

namespace cpp_request::detail::platform {

enum class SocketWaitStatus {
    Ready,
    TimedOut,
    Failed
};

struct SocketWaitResult {
    SocketWaitStatus status{SocketWaitStatus::Failed};
    int native_code{0};
};

struct SocketIoAttempt {
    std::ptrdiff_t transferred{-1};
    int native_code{0};
};

[[nodiscard]] SocketWaitResult wait_socket_readable(
    NativeSocketHandle socket,
    std::chrono::milliseconds timeout) noexcept;

[[nodiscard]] SocketWaitResult wait_socket_writable(
    NativeSocketHandle socket,
    std::chrono::milliseconds timeout) noexcept;

[[nodiscard]] SocketIoAttempt send_some(
    NativeSocketHandle socket,
    const char* data,
    std::size_t size) noexcept;

[[nodiscard]] SocketIoAttempt receive_some(
    NativeSocketHandle socket,
    char* data,
    std::size_t size) noexcept;

[[nodiscard]] bool socket_error_is_would_block(int code) noexcept;
[[nodiscard]] bool socket_error_is_interrupted(int code) noexcept;
[[nodiscard]] bool socket_error_is_connection_closed(int code) noexcept;

} // namespace cpp_request::detail::platform
