#include "platform/socket_io.hpp"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <cerrno>
#include <poll.h>
#include <sys/socket.h>
#endif

#include <algorithm>
#include <limits>

namespace cpp_request::detail::platform {
namespace {

SocketWaitResult wait_socket(
    NativeSocketHandle socket,
    std::chrono::milliseconds timeout,
    bool writable) noexcept {
    if (timeout.count() <= 0) {
        return {SocketWaitStatus::TimedOut, 0};
    }

#ifdef _WIN32
    fd_set ready_set;
    fd_set except_set;
    FD_ZERO(&ready_set);
    FD_ZERO(&except_set);
    FD_SET(socket, &ready_set);
    FD_SET(socket, &except_set);

    const long long total_ms = std::max<long long>(1, timeout.count());
    const long long seconds = std::min<long long>(
        total_ms / 1000,
        std::numeric_limits<long>::max());

    timeval tv{};
    tv.tv_sec = static_cast<long>(seconds);
    tv.tv_usec = seconds == std::numeric_limits<long>::max()
        ? 0L
        : static_cast<long>((total_ms % 1000) * 1000);

    const int wait_result = writable
        ? ::select(0, nullptr, &ready_set, &except_set, &tv)
        : ::select(0, &ready_set, nullptr, &except_set, &tv);

    if (wait_result == 0) {
        return {SocketWaitStatus::TimedOut, 0};
    }
    if (wait_result == SOCKET_ERROR) {
        return {SocketWaitStatus::Failed, ::WSAGetLastError()};
    }
    return {SocketWaitStatus::Ready, 0};
#else
    pollfd descriptor{};
    descriptor.fd = socket;
    descriptor.events = writable ? POLLOUT : POLLIN;

    const long long total_ms = std::max<long long>(1, timeout.count());
    const long long bounded = std::min<long long>(
        total_ms,
        std::numeric_limits<int>::max());

    const int wait_result = ::poll(
        &descriptor,
        1,
        static_cast<int>(bounded));

    if (wait_result == 0) {
        return {SocketWaitStatus::TimedOut, 0};
    }
    if (wait_result < 0) {
        return {SocketWaitStatus::Failed, errno};
    }
    return {SocketWaitStatus::Ready, 0};
#endif
}

std::size_t bounded_io_size(std::size_t size) noexcept {
    return std::min<std::size_t>(
        size,
        static_cast<std::size_t>(std::numeric_limits<int>::max()));
}

} // namespace

SocketWaitResult wait_socket_readable(
    NativeSocketHandle socket,
    std::chrono::milliseconds timeout) noexcept {
    return wait_socket(socket, timeout, false);
}

SocketWaitResult wait_socket_writable(
    NativeSocketHandle socket,
    std::chrono::milliseconds timeout) noexcept {
    return wait_socket(socket, timeout, true);
}

SocketIoAttempt send_some(
    NativeSocketHandle socket,
    const char* data,
    std::size_t size) noexcept {
    const std::size_t bounded = bounded_io_size(size);

#ifdef _WIN32
    const int result = ::send(
        socket,
        data,
        static_cast<int>(bounded),
        0);
    if (result == SOCKET_ERROR) {
        return {-1, ::WSAGetLastError()};
    }
    return {result, 0};
#else
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif
    const ssize_t result = ::send(socket, data, bounded, flags);
    if (result < 0) {
        return {-1, errno};
    }
    return {static_cast<std::ptrdiff_t>(result), 0};
#endif
}

SocketIoAttempt receive_some(
    NativeSocketHandle socket,
    char* data,
    std::size_t size) noexcept {
    const std::size_t bounded = bounded_io_size(size);

#ifdef _WIN32
    const int result = ::recv(
        socket,
        data,
        static_cast<int>(bounded),
        0);
    if (result == SOCKET_ERROR) {
        return {-1, ::WSAGetLastError()};
    }
    return {result, 0};
#else
    const ssize_t result = ::recv(socket, data, bounded, 0);
    if (result < 0) {
        return {-1, errno};
    }
    return {static_cast<std::ptrdiff_t>(result), 0};
#endif
}

bool socket_error_is_would_block(int code) noexcept {
#ifdef _WIN32
    return code == WSAEWOULDBLOCK;
#else
    return code == EAGAIN || code == EWOULDBLOCK;
#endif
}

bool socket_error_is_interrupted(int code) noexcept {
#ifdef _WIN32
    return code == WSAEINTR;
#else
    return code == EINTR;
#endif
}

bool socket_error_is_connection_closed(int code) noexcept {
#ifdef _WIN32
    return code == WSAECONNRESET
        || code == WSAECONNABORTED
        || code == WSAENOTCONN
        || code == WSAESHUTDOWN;
#else
    return code == EPIPE
        || code == ECONNRESET
        || code == ENOTCONN
        || code == ESHUTDOWN;
#endif
}

} // namespace cpp_request::detail::platform
