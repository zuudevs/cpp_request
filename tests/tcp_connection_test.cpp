#include <gtest/gtest.h>

#include "net/tcp_connection.hpp"
#include "platform/native_socket.hpp"
#include "platform/network_runtime.hpp"

#include <chrono>
#include <type_traits>
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
using cpp_request::detail::platform::kInvalidSocket;

static_assert(!std::is_copy_constructible_v<TcpConnection>);
static_assert(!std::is_copy_assignable_v<TcpConnection>);
static_assert(std::is_nothrow_move_constructible_v<TcpConnection>);
static_assert(std::is_nothrow_move_assignable_v<TcpConnection>);

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

    Endpoint endpoint{
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address),
        SOCK_STREAM,
        IPPROTO_TCP};

    return {std::move(listener), endpoint};
}

TEST(TcpConnectionTest, ConnectsToIpv4LoopbackListener) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);
    ASSERT_TRUE(listener.endpoint.valid());

    auto result = TcpConnection::connect(
        std::vector<Endpoint>{listener.endpoint},
        std::chrono::seconds{2});

    ASSERT_TRUE(result);
    EXPECT_TRUE(result.value().connected());
    EXPECT_NE(result.value().native_handle(), kInvalidSocket);
}

TEST(TcpConnectionTest, FallsBackToLaterCandidate) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);

    const Endpoint invalid_socket_candidate{
        listener.endpoint.native_address(),
        listener.endpoint.native_address_size(),
        -1,
        listener.endpoint.protocol()};

    std::vector<Endpoint> candidates;
    candidates.push_back(invalid_socket_candidate);
    candidates.push_back(listener.endpoint);

    auto result = TcpConnection::connect(candidates, std::chrono::seconds{2});

    ASSERT_TRUE(result);
    EXPECT_TRUE(result.value().connected());
}

TEST(TcpConnectionTest, ZeroTimeoutFailsImmediately) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);

    auto result = TcpConnection::connect(
        std::vector<Endpoint>{listener.endpoint},
        std::chrono::milliseconds{0});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ConnectTimeout);
}

TEST(TcpConnectionTest, EmptyCandidateListIsConnectFailure) {
    auto result = TcpConnection::connect({}, std::chrono::seconds{1});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ConnectFailed);
}

TEST(TcpConnectionTest, CloseInvalidatesConnection) {
    auto listener = make_ipv4_listener();
    ASSERT_TRUE(listener.socket);

    auto result = TcpConnection::connect(
        std::vector<Endpoint>{listener.endpoint},
        std::chrono::seconds{2});
    ASSERT_TRUE(result);

    result.value().close();
    EXPECT_FALSE(result.value().connected());
}

} // namespace
