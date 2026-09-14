#include <gtest/gtest.h>

#include "net/endpoint.hpp"
#include "net/resolver.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <algorithm>

namespace {

using cpp_request::ErrorCode;
using cpp_request::detail::net::Endpoint;
using cpp_request::detail::net::Resolver;

TEST(EndpointTest, CopiesNativeAddressStorage) {
    sockaddr_in source{};
    source.sin_family = AF_INET;
    source.sin_port = htons(8080);
    source.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    Endpoint endpoint{
        reinterpret_cast<const sockaddr*>(&source),
        sizeof(source),
        SOCK_STREAM,
        IPPROTO_TCP};

    ASSERT_TRUE(endpoint.valid());
    ASSERT_TRUE(endpoint.is_ipv4());
    EXPECT_FALSE(endpoint.is_ipv6());
    EXPECT_EQ(endpoint.socket_type(), SOCK_STREAM);
    EXPECT_EQ(endpoint.protocol(), IPPROTO_TCP);

    source.sin_port = 0;

    const auto* copied = reinterpret_cast<const sockaddr_in*>(endpoint.native_address());
    EXPECT_EQ(ntohs(copied->sin_port), 8080);
    EXPECT_EQ(endpoint.native_address_size(), sizeof(sockaddr_in));
}

TEST(ResolverTest, ResolvesNumericIpv4Loopback) {
    auto result = Resolver::resolve("127.0.0.1", 8080);

    ASSERT_TRUE(result) << "native resolver code: " << result.error().native_code;
    ASSERT_FALSE(result.value().empty());

    const auto candidate = std::find_if(
        result.value().begin(),
        result.value().end(),
        [](const Endpoint& endpoint) { return endpoint.is_ipv4(); });

    ASSERT_NE(candidate, result.value().end());
    EXPECT_EQ(candidate->socket_type(), SOCK_STREAM);
    EXPECT_EQ(candidate->protocol(), IPPROTO_TCP);

    const auto* address = reinterpret_cast<const sockaddr_in*>(candidate->native_address());
    EXPECT_EQ(ntohs(address->sin_port), 8080);
}

TEST(ResolverTest, ResolvesNumericIpv6Loopback) {
    auto result = Resolver::resolve("::1", 8081);

    ASSERT_TRUE(result) << "native resolver code: " << result.error().native_code;

    const auto candidate = std::find_if(
        result.value().begin(),
        result.value().end(),
        [](const Endpoint& endpoint) { return endpoint.is_ipv6(); });

    ASSERT_NE(candidate, result.value().end());
    EXPECT_EQ(candidate->socket_type(), SOCK_STREAM);
    EXPECT_EQ(candidate->protocol(), IPPROTO_TCP);

    const auto* address = reinterpret_cast<const sockaddr_in6*>(candidate->native_address());
    EXPECT_EQ(ntohs(address->sin6_port), 8081);
}

TEST(ResolverTest, InvalidHostnameReturnsResolveFailed) {
    auto result = Resolver::resolve("not a valid host name", 80);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ResolveFailed);
}

TEST(ResolverTest, EmptyHostnameReturnsResolveFailed) {
    auto result = Resolver::resolve({}, 80);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ResolveFailed);
}

} // namespace
