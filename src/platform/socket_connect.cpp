#include "platform/socket_connect.hpp"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#endif

#include <algorithm>
#include <limits>

namespace cpp_request::detail::platform {
namespace {

#ifdef _WIN32
bool set_nonblocking(NativeSocketHandle socket, bool enabled, int& native_code) noexcept {
    u_long mode = enabled ? 1UL : 0UL;
    if (::ioctlsocket(socket, FIONBIO, &mode) == 0) {
        return true;
    }
    native_code = ::WSAGetLastError();
    return false;
}

bool connect_in_progress(int code) noexcept {
    return code == WSAEWOULDBLOCK || code == WSAEINPROGRESS || code == WSAEALREADY;
}
#else
bool set_nonblocking(NativeSocketHandle socket, bool enabled, int& native_code) noexcept {
    const int flags = ::fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        native_code = errno;
        return false;
    }

    const int next = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (::fcntl(socket, F_SETFL, next) == 0) {
        return true;
    }
    native_code = errno;
    return false;
}

bool connect_in_progress(int code) noexcept {
    return code == EINPROGRESS || code == EWOULDBLOCK || code == EALREADY;
}
#endif

ConnectAttemptResult socket_error_result(NativeSocketHandle socket) noexcept {
    int error = 0;
#ifdef _WIN32
    int length = sizeof(error);
    if (::getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &length) != 0) {
        return {ConnectStatus::Failed, ::WSAGetLastError()};
    }
#else
    socklen_t length = sizeof(error);
    if (::getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &length) != 0) {
        return {ConnectStatus::Failed, errno};
    }
#endif
    return error == 0
        ? ConnectAttemptResult{ConnectStatus::Connected, 0}
        : ConnectAttemptResult{ConnectStatus::Failed, error};
}

} // namespace

NativeSocket create_tcp_socket(
    int family,
    int socket_type,
    int protocol,
    int& native_code) noexcept {
    native_code = 0;
    const NativeSocketHandle handle = ::socket(family, socket_type, protocol);
    if (handle == kInvalidSocket) {
        native_code = last_socket_error();
        return {};
    }
    return NativeSocket{handle};
}

ConnectAttemptResult connect_with_timeout(
    NativeSocketHandle socket,
    const sockaddr* address,
    std::size_t address_size,
    std::chrono::milliseconds timeout) noexcept {
    if (timeout.count() <= 0) {
        return {ConnectStatus::TimedOut, 0};
    }

    int native_code = 0;
    if (!set_nonblocking(socket, true, native_code)) {
        return {ConnectStatus::Failed, native_code};
    }

#ifdef _WIN32
    const int connect_result = ::connect(
        socket,
        address,
        static_cast<int>(address_size));
#else
    const int connect_result = ::connect(
        socket,
        address,
        static_cast<socklen_t>(address_size));
#endif

    if (connect_result == 0) {
        if (!set_nonblocking(socket, false, native_code)) {
            return {ConnectStatus::Failed, native_code};
        }
        return {ConnectStatus::Connected, 0};
    }

    native_code = last_socket_error();
    if (!connect_in_progress(native_code)) {
        return {ConnectStatus::Failed, native_code};
    }

#ifdef _WIN32
    fd_set write_set;
    fd_set except_set;
    FD_ZERO(&write_set);
    FD_ZERO(&except_set);
    FD_SET(socket, &write_set);
    FD_SET(socket, &except_set);

    const auto total_ms = timeout.count();
    timeval tv{};
    tv.tv_sec = static_cast<long>(total_ms / 1000);
    tv.tv_usec = static_cast<long>((total_ms % 1000) * 1000);

    const int wait_result = ::select(0, nullptr, &write_set, &except_set, &tv);
    if (wait_result == 0) {
        return {ConnectStatus::TimedOut, 0};
    }
    if (wait_result == SOCKET_ERROR) {
        return {ConnectStatus::Failed, ::WSAGetLastError()};
    }
#else
    pollfd descriptor{};
    descriptor.fd = socket;
    descriptor.events = POLLOUT;

    const auto bounded = std::min<long long>(
        timeout.count(),
        std::numeric_limits<int>::max());
    const int wait_result = ::poll(&descriptor, 1, static_cast<int>(bounded));
    if (wait_result == 0) {
        return {ConnectStatus::TimedOut, 0};
    }
    if (wait_result < 0) {
        return {ConnectStatus::Failed, errno};
    }
#endif

    auto result = socket_error_result(socket);
    if (result.status == ConnectStatus::Connected) {
        if (!set_nonblocking(socket, false, native_code)) {
            return {ConnectStatus::Failed, native_code};
        }
    }
    return result;
}

} // namespace cpp_request::detail::platform
