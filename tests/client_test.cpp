#include <gtest/gtest.h>

#include "platform/native_socket.hpp"
#include "platform/network_runtime.hpp"

#include <cpp_request/client.hpp>
#include <cpp_request/error.hpp>
#include <cpp_request/request.hpp>

#include <array>
#include <charconv>
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
#include <sys/socket.h>
#endif

namespace {

using cpp_request::Client;
using cpp_request::ErrorCode;
using cpp_request::Method;
using cpp_request::Request;
using cpp_request::detail::platform::NativeSocket;
using cpp_request::detail::platform::NativeSocketHandle;

struct LoopbackServer {
    NativeSocket listener;
    std::uint16_t port{0};
};

LoopbackServer make_loopback_server() {
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
        ntohs(address.sin_port)};
}

NativeSocket accept_one(NativeSocketHandle listener) {
    return NativeSocket{::accept(listener, nullptr, nullptr)};
}

int receive_native(
    NativeSocketHandle socket,
    char* buffer,
    std::size_t capacity) {
#ifdef _WIN32
    return ::recv(socket, buffer, static_cast<int>(capacity), 0);
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

bool send_all_native(NativeSocketHandle socket, std::string_view data) {
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

std::size_t parse_content_length(std::string_view headers) {
    constexpr std::string_view kName = "Content-Length:";
    const std::size_t position = headers.find(kName);
    if (position == std::string_view::npos) {
        return 0;
    }

    std::size_t begin = position + kName.size();
    while (begin < headers.size() && headers[begin] == ' ') {
        ++begin;
    }

    const std::size_t end = headers.find("\r\n", begin);
    const std::string_view value = headers.substr(begin, end - begin);
    std::size_t length = 0;
    const auto parsed = std::from_chars(
        value.data(),
        value.data() + value.size(),
        length,
        10);
    return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size()
        ? length
        : 0;
}

std::string receive_request(NativeSocketHandle socket) {
    std::string request;
    std::array<char, 4096> buffer{};
    std::size_t expected_size = 0;

    for (;;) {
        const int count = receive_native(socket, buffer.data(), buffer.size());
        if (count <= 0) {
            break;
        }

        request.append(buffer.data(), static_cast<std::size_t>(count));
        const std::size_t head_end = request.find("\r\n\r\n");
        if (head_end != std::string::npos) {
            expected_size = head_end + 4
                + parse_content_length(std::string_view{request}.substr(0, head_end + 2));
            if (request.size() >= expected_size) {
                break;
            }
        }
    }

    return request;
}

std::string make_url(std::uint16_t port, std::string_view target = "/") {
    return "http://127.0.0.1:" + std::to_string(port) + std::string{target};
}

void configure_test_timeouts(Client& client) {
    client.set_connect_timeout(std::chrono::seconds{2});
    client.set_write_timeout(std::chrono::seconds{2});
    client.set_read_timeout(std::chrono::seconds{2});
}

TEST(ClientTest, ExecutesRequestEndToEnd) {
    auto server = make_loopback_server();
    ASSERT_TRUE(server.listener);

    std::string received_request;
    std::thread worker([&] {
        NativeSocket peer = accept_one(server.listener.get());
        if (!peer) {
            return;
        }
        received_request = receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 5\r\n"
            "X-Server: loopback\r\n"
            "\r\n"
            "hello");
    });

    const std::string url = make_url(server.port, "/hello?x=1");
    Request request{Method::Get, url};
    request.headers().add("X-Client", "cpp_request");

    Client client;
    configure_test_timeouts(client);
    auto result = client.request(request);

    worker.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().status_code(), 200);
    EXPECT_EQ(result.value().body(), "hello");
    EXPECT_EQ(result.value().headers().get("x-server"), "loopback");
    EXPECT_NE(received_request.find("GET /hello?x=1 HTTP/1.1\r\n"), std::string::npos);
    EXPECT_NE(received_request.find("X-Client: cpp_request\r\n"), std::string::npos);
    EXPECT_NE(
        received_request.find(
            "Host: 127.0.0.1:" + std::to_string(server.port) + "\r\n"),
        std::string::npos);
}

TEST(ClientTest, SendsBodyAndDecodesChunkedResponse) {
    auto server = make_loopback_server();
    ASSERT_TRUE(server.listener);

    std::string received_request;
    std::thread worker([&] {
        NativeSocket peer = accept_one(server.listener.get());
        if (!peer) {
            return;
        }
        received_request = receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 201 Created\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "4\r\nWiki\r\n"
            "5\r\npedia\r\n"
            "0\r\n\r\n");
    });

    const std::string url = make_url(server.port, "/submit");
    Client client;
    configure_test_timeouts(client);
    auto result = client.post(url, "payload");

    worker.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().status_code(), 201);
    EXPECT_EQ(result.value().body(), "Wikipedia");
    EXPECT_NE(received_request.find("POST /submit HTTP/1.1\r\n"), std::string::npos);
    EXPECT_NE(received_request.find("Content-Length: 7\r\n"), std::string::npos);
    ASSERT_GE(received_request.size(), 7U);
    EXPECT_EQ(received_request.substr(received_request.size() - 7), "payload");
}

TEST(ClientTest, CompletesCloseDelimitedResponseOnPeerClose) {
    auto server = make_loopback_server();
    ASSERT_TRUE(server.listener);

    std::thread worker([&] {
        NativeSocket peer = accept_one(server.listener.get());
        if (!peer) {
            return;
        }
        (void)receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "close-delimited");
        peer.close();
    });

    Client client;
    configure_test_timeouts(client);
    auto result = client.get(make_url(server.port));

    worker.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().body(), "close-delimited");
}

TEST(ClientTest, PropagatesProtocolErrors) {
    auto server = make_loopback_server();
    ASSERT_TRUE(server.listener);

    std::thread worker([&] {
        NativeSocket peer = accept_one(server.listener.get());
        if (!peer) {
            return;
        }
        (void)receive_request(peer.get());
        (void)send_all_native(peer.get(), "HTTP/1.0 200 OK\r\n\r\n");
    });

    Client client;
    configure_test_timeouts(client);
    auto result = client.get(make_url(server.port));

    worker.join();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidStatusLine);
}

TEST(ClientTest, RejectsUnsupportedSchemeBeforeNetworking) {
    Client client;
    auto result = client.get("https://example.com/");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::UnsupportedScheme);
}

} // namespace
