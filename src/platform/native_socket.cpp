#include "platform/native_socket.hpp"

#ifndef _WIN32
#include <cerrno>
#include <unistd.h>
#endif

namespace cpp_request::detail::platform {
namespace {

void close_native_socket(NativeSocketHandle handle) noexcept {
#ifdef _WIN32
    ::closesocket(handle);
#else
    ::close(handle);
#endif
}

} // namespace

NativeSocket::~NativeSocket() noexcept {
    close();
}

void NativeSocket::reset(handle_type replacement) noexcept {
    if (handle_ == replacement) {
        return;
    }

    if (valid()) {
        close_native_socket(handle_);
    }

    handle_ = replacement;
}

int last_socket_error() noexcept {
#ifdef _WIN32
    return ::WSAGetLastError();
#else
    return errno;
#endif
}

} // namespace cpp_request::detail::platform
