#pragma once

#include "net/endpoint.hpp"
#include "platform/native_socket.hpp"

#include <cpp_request/result.hpp>

#include <chrono>
#include <cstddef>
#include <string_view>
#include <utility>
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

    [[nodiscard]] static TcpConnection adopt_native_handle(
        platform::NativeSocketHandle handle) noexcept {
        return TcpConnection{platform::NativeSocket{handle}};
    }

    [[nodiscard]] Result<std::size_t> write_all(
        std::string_view data,
        std::chrono::milliseconds timeout);

    [[nodiscard]] Result<std::size_t> read_some(
        char* buffer,
        std::size_t capacity,
        std::chrono::milliseconds timeout);

    [[nodiscard]] bool connected() const noexcept {
        return socket_.valid();
    }

    [[nodiscard]] platform::NativeSocketHandle native_handle() const noexcept {
        return socket_.get();
    }

    [[nodiscard]] platform::NativeSocketHandle release_native_handle() noexcept {
        return socket_.release();
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
