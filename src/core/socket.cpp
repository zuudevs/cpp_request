/**
 * @file socket.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Socket wrapper implementation
 * @version 0.2.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#include "socket.hpp"
#include <winsock2.h>

namespace zd::crq::core {

Socket::~Socket() noexcept {
    close();
}

bool Socket::create(int family, int type, int protocol) noexcept {
    close(); // Release any existing handle before allocating a new one.
    handle_ = static_cast<handle_t>(::socket(family, type, protocol));
    return valid();
}

bool Socket::connect(const char* address, int addrlen) noexcept {
    if (!valid()) return false;
    int res = ::connect(handle_,
                        reinterpret_cast<const sockaddr*>(address),
                        addrlen);
    return res != SOCKET_ERROR;
}

int Socket::send(const char* data, int size) noexcept {
    if (!valid()) return SOCKET_ERROR;
    return ::send(handle_, data, size, 0);
}

int Socket::receive(char* buffer, int size) noexcept {
    if (!valid()) return SOCKET_ERROR;
    return ::recv(handle_, buffer, size, 0);
}

bool Socket::valid() const noexcept {
    return handle_ != invalid_handle &&
           handle_ != static_cast<handle_t>(INVALID_SOCKET);
}

Socket::handle_t Socket::handle() const noexcept {
    return handle_;
}

void Socket::close() noexcept {
    if (valid()) {
        ::closesocket(static_cast<SOCKET>(handle_));
        handle_ = invalid_handle;
    }
}

} // namespace zd::crq::core