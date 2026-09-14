#pragma once

#include "net/endpoint.hpp"
#include "platform/native_socket.hpp"

#include <cpp_request/result.hpp>

#include <chrono>
#include <vector>

namespace cpp_request::detail::net {

class TcpConnection final {
public:
    TcpConnection() noexcept = default;

    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;
    TcpConnection(TcpConnection&&) noexcept = default;
    TcpConnection& operator=(TcpConnection&&) noexcept = default;

    [[nodiscard]] static Result<TcpConnection> connect(
        const std::vector<Endpoint>& endpoints,
        std::chrono::milliseconds timeout);

    [[nodiscard]] bool connected() const noexcept {
        return socket_.valid();
    }

    [[nodiscard]] platform::NativeSocketHandle native_handle() const noexcept {
        return socket_.get();
    }

    void close() noexcept {
        socket_.close();
    }

private:
    explicit TcpConnection(platform::NativeSocket socket) noexcept
        : socket_(std::move(socket)) {}

    platform::NativeSocket socket_;
};

} // namespace cpp_request::detail::net
