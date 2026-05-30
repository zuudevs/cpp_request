/**
 * @file socket.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Internal socket wrapper for platform abstraction
 * @version 0.1.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <cstddef>

namespace zd::crq::core {

class Socket {
public:
    using handle_t = size_t;
    static constexpr handle_t invalid_handle = ~0;

    Socket() noexcept = default;
    ~Socket() noexcept;

    bool create(int family, int type, int protocol) noexcept;
    bool connect(const char* address, int addrlen) noexcept;
    
    int send(const char* data, int size) noexcept;
    int receive(char* buffer, int size) noexcept;
    
    bool valid() const noexcept;
    handle_t handle() const noexcept;
    void close() noexcept;

private:
    handle_t handle_{invalid_handle};
};

} // namespace zd::crq::core
