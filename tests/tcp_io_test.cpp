#include <gtest/gtest.h>

#include "net/tcp_connection.hpp"
#include "platform/native_socket.hpp"
#include "platform/network_runtime.hpp"

#include <array>
#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

namespace {

using cpp_request::ErrorCode;
using cpp_request::detail::net::Endpoint;
using cpp_request::detail::net::TcpConnection;
using cpp_request::detail::platform::NativeSocket;
using cpp_request::detail::platform::NativeSocketHandle;
using cpp_request::detail::platform::kInvalidSocket;

struct LoopbackListener {
    NativeSocket socket;
    Endpoint endpoint;
};

LoopbackListener make_ipv4_listener() {
    const auto runtime = cpp_request::detail::platform::ensure_network_runtime();
    if (!runtime.ok) {
        return {};
    }

    NativeSocket listener{::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)};
    if (!listener) {
        return {};
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = 0;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (::bind(
            listener.get(),
            reinterpret_cast<const sockaddr*>(&address),
            static_cast<int>(sizeof(address))) != 0) {
        return {};
    }

    if (::listen(listener.get(), 1) != 0) {
        return {};
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
        return {};
    }

    return {
        std::move(listener),
        Endpoint{
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address),
            SOCK_STREAM,
            IPPROTO_TCP}};
}

NativeSocket accept_one(NativeSocketHandle listener) {
    return NativeSocket{::accept(listener, nullptr, nullptr)};
}

int receive_native(
    NativeSocketHandle socket,
    char* buffer,
    std::size_t capacity) {
#ifdef _WIN32
    return ::recv(
        socket,
        buffer,
        static_cast<int>(capacity),
        0);
#else
    return static_cast<int>(::recv(socket, buffer, capacity, 0));
#endif
}

int send_native(
    NativeSocketHandle socket,
    const char* data,
    std::size_t size) {
#ifdef _WIN32
    return ::send(socket, data, static_cast<int>(size), 0);
#else
    return static_cast<int>(::send(socket, data, size, 0));
#endif
}

TEST(TcpIoTest, WriteAllTransfersCompletePayload) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);

    auto connection_result = TcpConnection::connect(
        std::vector<Endpoint>{listener.endpoint},
        std::chrono::seconds{2});
    ASSERT_TRUE(connection_result);

    NativeSocket peer = accept_one(listener.socket.get());
    ASSERT_TRUE(peer);

    std::string payload(2 * 1024 * 1024, 'x');
    std::string received;
    received.reserve(payload.size());

    std::thread reader([&] {
        std::array<char, 8192> buffer{};
        while (received.size() < payload.size()) {
            const int count = receive_native(
                peer.get(),
                buffer.data(),
                buffer.size());
            if (count <= 0) {
                break;
            }
            received.append(buffer.data(), static_cast<std::size_t>(count));
        }
    });

    auto write_result = connection_result.value().write_all(
        payload,
        std::chrono::seconds{3});

    reader.join();

    ASSERT_TRUE(write_result);
    EXPECT_EQ(write_result.value(), payload.size());
    EXPECT_EQ(received, payload);
    EXPECT_TRUE(connection_result.value().connected());
}

TEST(TcpIoTest, ReadSomeReturnsAvailableBytes) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);

    auto connection_result = TcpConnection::connect(
        std::vector<Endpoint>{listener.endpoint},
        std::chrono::seconds{2});
    ASSERT_TRUE(connection_result);

    NativeSocket peer = accept_one(listener.socket.get());
    ASSERT_TRUE(peer);

    const std::string message = "hello from loopback";
    ASSERT_EQ(
        send_native(peer.get(), message.data(), message.size()),
        static_cast<int>(message.size()));

    std::array<char, 64> buffer{};
    auto read_result = connection_result.value().read_some(
        buffer.data(),
        buffer.size(),
        std::chrono::seconds{2});

    ASSERT_TRUE(read_result);
    EXPECT_EQ(
        std::string_view(buffer.data(), read_result.value()),
        message);
    EXPECT_TRUE(connection_result.value().connected());
}

TEST(TcpIoTest, ReadTimeoutClosesConnection) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);

    auto connection_result = TcpConnection::connect(
        std::vector<Endpoint>{listener.endpoint},
        std::chrono::seconds{2});
    ASSERT_TRUE(connection_result);

    NativeSocket peer = accept_one(listener.socket.get());
    ASSERT_TRUE(peer);

    std::array<char, 16> buffer{};
    auto read_result = connection_result.value().read_some(
        buffer.data(),
        buffer.size(),
        std::chrono::milliseconds{75});

    ASSERT_FALSE(read_result);
    EXPECT_EQ(read_result.error().code, ErrorCode::ReadTimeout);
    EXPECT_FALSE(connection_result.value().connected());
}

TEST(TcpIoTest, PeerCloseIsReportedAsConnectionClosed) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);

    auto connection_result = TcpConnection::connect(
        std::vector<Endpoint>{listener.endpoint},
        std::chrono::seconds{2});
    ASSERT_TRUE(connection_result);

    NativeSocket peer = accept_one(listener.socket.get());
    ASSERT_TRUE(peer);
    peer.close();

    std::array<char, 16> buffer{};
    auto read_result = connection_result.value().read_some(
        buffer.data(),
        buffer.size(),
        std::chrono::seconds{2});

    ASSERT_FALSE(read_result);
    EXPECT_EQ(read_result.error().code, ErrorCode::ConnectionClosed);
    EXPECT_FALSE(connection_result.value().connected());
}

TEST(TcpIoTest, WriteTimeoutClosesConnection) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);

    auto connection_result = TcpConnection::connect(
        std::vector<Endpoint>{listener.endpoint},
        std::chrono::seconds{2});
    ASSERT_TRUE(connection_result);

    NativeSocket peer = accept_one(listener.socket.get());
    ASSERT_TRUE(peer);

    const std::string payload = "timeout";
    auto write_result = connection_result.value().write_all(
        payload,
        std::chrono::milliseconds{0});

    ASSERT_FALSE(write_result);
    EXPECT_EQ(write_result.error().code, ErrorCode::WriteTimeout);
    EXPECT_FALSE(connection_result.value().connected());
}

TEST(TcpIoTest, EmptyWriteIsSuccessfulNoOp) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);

    auto connection_result = TcpConnection::connect(
        std::vector<Endpoint>{listener.endpoint},
        std::chrono::seconds{2});
    ASSERT_TRUE(connection_result);

    NativeSocket peer = accept_one(listener.socket.get());
    ASSERT_TRUE(peer);

    auto write_result = connection_result.value().write_all(
        {},
        std::chrono::milliseconds{0});

    ASSERT_TRUE(write_result);
    EXPECT_EQ(write_result.value(), 0U);
    EXPECT_TRUE(connection_result.value().connected());
}

TEST(TcpIoTest, ZeroCapacityReadIsSuccessfulNoOp) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);

    auto connection_result = TcpConnection::connect(
        std::vector<Endpoint>{listener.endpoint},
        std::chrono::seconds{2});
    ASSERT_TRUE(connection_result);

    NativeSocket peer = accept_one(listener.socket.get());
    ASSERT_TRUE(peer);

    auto read_result = connection_result.value().read_some(
        nullptr,
        0,
        std::chrono::milliseconds{0});

    ASSERT_TRUE(read_result);
    EXPECT_EQ(read_result.value(), 0U);
    EXPECT_TRUE(connection_result.value().connected());
}

} // namespace
