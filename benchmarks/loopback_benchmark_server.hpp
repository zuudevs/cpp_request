#pragma once

#include "platform/native_socket.hpp"
#include "platform/network_runtime.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#endif

namespace cpp_request::bench {

class LoopbackBenchmarkServer final {
public:
    explicit LoopbackBenchmarkServer(std::size_t body_size = 32)
        : response_(
            "HTTP/1.1 200 OK\r\nContent-Length: "
            + std::to_string(body_size)
            + "\r\n\r\n"
            + std::string(body_size, 'x')) {
        const auto runtime = detail::platform::ensure_network_runtime();
        if (!runtime.ok) {
            return;
        }

        detail::platform::NativeSocket listener{
            ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)};
        if (!listener) {
            return;
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = 0;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (::bind(
                listener.get(),
                reinterpret_cast<const sockaddr*>(&address),
                static_cast<int>(sizeof(address))) != 0
            || ::listen(listener.get(), 16) != 0) {
            return;
        }

#ifdef _WIN32
        int address_size = sizeof(address);
#else
        socklen_t address_size = sizeof(address);
#endif
        if (::getsockname(
                listener.get(),
                reinterpret_cast<sockaddr*>(&address),
                &address_size) != 0) {
            return;
        }

        port_ = ntohs(address.sin_port);
        listener_ = std::move(listener);
        worker_ = std::thread([this] { run(); });
    }

    ~LoopbackBenchmarkServer() {
        stop_.store(true, std::memory_order_relaxed);
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    LoopbackBenchmarkServer(const LoopbackBenchmarkServer&) = delete;
    LoopbackBenchmarkServer& operator=(const LoopbackBenchmarkServer&) = delete;

    [[nodiscard]] bool valid() const noexcept {
        return static_cast<bool>(listener_) && port_ != 0;
    }

    [[nodiscard]] std::uint16_t port() const noexcept { return port_; }

    [[nodiscard]] std::string url(std::string_view target = "/") const {
        return "http://127.0.0.1:" + std::to_string(port_) + std::string{target};
    }

private:
    using NativeSocket = detail::platform::NativeSocket;
    using NativeSocketHandle = detail::platform::NativeSocketHandle;

    static bool wait_readable(
        NativeSocketHandle socket,
        std::chrono::milliseconds timeout) noexcept {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(socket, &read_set);

        timeval value{};
        value.tv_sec = static_cast<long>(timeout.count() / 1000);
        value.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

#ifdef _WIN32
        return ::select(0, &read_set, nullptr, nullptr, &value) > 0;
#else
        return ::select(socket + 1, &read_set, nullptr, nullptr, &value) > 0;
#endif
    }

    static int receive_native(
        NativeSocketHandle socket,
        char* data,
        std::size_t size) noexcept {
#ifdef _WIN32
        return ::recv(socket, data, static_cast<int>(size), 0);
#else
        return static_cast<int>(::recv(socket, data, size, 0));
#endif
    }

    static int send_native(
        NativeSocketHandle socket,
        const char* data,
        std::size_t size) noexcept {
#ifdef _WIN32
        return ::send(socket, data, static_cast<int>(size), 0);
#else
        return static_cast<int>(::send(socket, data, size, 0));
#endif
    }

    static bool send_all(NativeSocketHandle socket, std::string_view data) {
        std::size_t written = 0;
        while (written < data.size()) {
            const int count = send_native(
                socket,
                data.data() + written,
                data.size() - written);
            if (count <= 0) {
                return false;
            }
            written += static_cast<std::size_t>(count);
        }
        return true;
    }

    bool receive_request(NativeSocketHandle socket) const {
        std::string request;
        std::array<char, 4096> buffer{};

        while (!stop_.load(std::memory_order_relaxed)) {
            if (!wait_readable(socket, std::chrono::milliseconds{100})) {
                continue;
            }

            const int count = receive_native(socket, buffer.data(), buffer.size());
            if (count <= 0) {
                return false;
            }

            request.append(buffer.data(), static_cast<std::size_t>(count));
            if (request.find("\r\n\r\n") != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    void serve_peer(NativeSocketHandle socket) {
        while (!stop_.load(std::memory_order_relaxed)) {
            if (!receive_request(socket)) {
                return;
            }
            if (!send_all(socket, response_)) {
                return;
            }
        }
    }

    void run() {
        while (!stop_.load(std::memory_order_relaxed)) {
            if (!listener_ || !wait_readable(
                    listener_.get(),
                    std::chrono::milliseconds{100})) {
                continue;
            }

            NativeSocket peer{::accept(listener_.get(), nullptr, nullptr)};
            if (!peer) {
                continue;
            }
            serve_peer(peer.get());
        }
    }

    NativeSocket listener_;
    std::uint16_t port_{0};
    std::atomic<bool> stop_{false};
    std::thread worker_;
    std::string response_;
};

} // namespace cpp_request::bench
