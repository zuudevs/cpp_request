#include "platform/socket_connect.hpp"

#include "platform/socket_io.hpp"
#include "platform/socket_mode.hpp"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <cerrno>
#include <sys/socket.h>
#endif

namespace cpp_request::detail::platform {
namespace {

#ifdef _WIN32
bool connect_in_progress(int code) noexcept {
    return code == WSAEWOULDBLOCK || code == WSAEINPROGRESS || code == WSAEALREADY;
}
#else
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

    NativeSocket socket{handle};

#if !defined(_WIN32) && defined(SO_NOSIGPIPE) && !defined(MSG_NOSIGNAL)
    const int enabled = 1;
    if (::setsockopt(
            handle,
            SOL_SOCKET,
            SO_NOSIGPIPE,
            &enabled,
            static_cast<socklen_t>(sizeof(enabled))) != 0) {
        native_code = last_socket_error();
        return {};
    }
#endif

    return socket;
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
    if (!set_socket_nonblocking(socket, true, native_code)) {
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
        if (!set_socket_nonblocking(socket, false, native_code)) {
            return {ConnectStatus::Failed, native_code};
        }
        return {ConnectStatus::Connected, 0};
    }

    native_code = last_socket_error();
    if (!connect_in_progress(native_code)) {
        return {ConnectStatus::Failed, native_code};
    }

    const auto wait = wait_socket_writable(socket, timeout);
    if (wait.status == SocketWaitStatus::TimedOut) {
        return {ConnectStatus::TimedOut, wait.native_code};
    }
    if (wait.status == SocketWaitStatus::Failed) {
        return {ConnectStatus::Failed, wait.native_code};
    }

    auto result = socket_error_result(socket);
    if (result.status == ConnectStatus::Connected) {
        if (!set_socket_nonblocking(socket, false, native_code)) {
            return {ConnectStatus::Failed, native_code};
        }
    }
    return result;
}

} // namespace cpp_request::detail::platform
