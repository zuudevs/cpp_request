#pragma once

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <cstddef>
#include <cstring>

namespace cpp_request::detail::net {

class Endpoint final {
public:
    Endpoint() noexcept = default;

    Endpoint(
        const sockaddr* address,
        std::size_t address_size,
        int socket_type,
        int protocol) noexcept
        : address_size_(address_size),
          socket_type_(socket_type),
          protocol_(protocol) {
        if (address != nullptr && address_size <= sizeof(address_)) {
            std::memcpy(&address_, address, address_size);
        } else {
            address_size_ = 0;
        }
    }

    [[nodiscard]] bool valid() const noexcept {
        return address_size_ != 0;
    }

    [[nodiscard]] int family() const noexcept {
        return valid() ? address_.ss_family : AF_UNSPEC;
    }

    [[nodiscard]] int socket_type() const noexcept {
        return socket_type_;
    }

    [[nodiscard]] int protocol() const noexcept {
        return protocol_;
    }

    [[nodiscard]] bool is_ipv4() const noexcept {
        return family() == AF_INET;
    }

    [[nodiscard]] bool is_ipv6() const noexcept {
        return family() == AF_INET6;
    }

    [[nodiscard]] const sockaddr* native_address() const noexcept {
        return reinterpret_cast<const sockaddr*>(&address_);
    }

    [[nodiscard]] std::size_t native_address_size() const noexcept {
        return address_size_;
    }

private:
    sockaddr_storage address_{};
    std::size_t address_size_{0};
    int socket_type_{SOCK_STREAM};
    int protocol_{0};
};

} // namespace cpp_request::detail::net
