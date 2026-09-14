#pragma once

#include "platform/native_socket.hpp"

#include <chrono>
#include <cstddef>

struct sockaddr;

namespace cpp_request::detail::platform {

enum class ConnectStatus {
    Connected,
    Failed,
    TimedOut
};

struct ConnectAttemptResult {
    ConnectStatus status{ConnectStatus::Failed};
    int native_code{0};
};

[[nodiscard]] NativeSocket create_tcp_socket(
    int family,
    int socket_type,
    int protocol,
    int& native_code) noexcept;

[[nodiscard]] ConnectAttemptResult connect_with_timeout(
    NativeSocketHandle socket,
    const sockaddr* address,
    std::size_t address_size,
    std::chrono::milliseconds timeout) noexcept;

} // namespace cpp_request::detail::platform
