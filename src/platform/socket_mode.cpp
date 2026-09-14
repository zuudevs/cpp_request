#include "platform/socket_mode.hpp"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <cerrno>
#include <fcntl.h>
#endif

namespace cpp_request::detail::platform {

bool set_socket_nonblocking(
    NativeSocketHandle socket,
    bool enabled,
    int& native_code) noexcept {
    native_code = 0;

#ifdef _WIN32
    u_long mode = enabled ? 1UL : 0UL;
    if (::ioctlsocket(socket, FIONBIO, &mode) == 0) {
        return true;
    }
    native_code = ::WSAGetLastError();
    return false;
#else
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
#endif
}

} // namespace cpp_request::detail::platform
