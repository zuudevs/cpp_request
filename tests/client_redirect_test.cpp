#include <gtest/gtest.h>

#include "platform/native_socket.hpp"
#include "platform/network_runtime.hpp"

#include <cpp_request/client.hpp>
#include <cpp_request/error.hpp>
#include <cpp_request/request.hpp>

#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/select.h>
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

    if (::listen(listener.get(), 4) != 0) {
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

    return {std::move(listener), ntohs(address.sin_port)};
}

bool wait_readable(
    NativeSocketHandle socket,
    std::chrono::milliseconds timeout) {
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

NativeSocket accept_one(
    NativeSocketHandle listener,
    std::chrono::milliseconds timeout = std::chrono::seconds{4}) {
    if (!wait_readable(listener, timeout)) {
        return {};
    }
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

std::string receive_request(
    NativeSocketHandle socket,
    std::chrono::milliseconds timeout = std::chrono::seconds{3}) {
    std::string request;
    std::array<char, 4096> buffer{};

    for (;;) {
        if (!wait_readable(socket, timeout)) {
            return request;
        }

        const int count = receive_native(socket, buffer.data(), buffer.size());
        if (count <= 0) {
            return request;
        }

        request.append(buffer.data(), static_cast<std::size_t>(count));
        const std::size_t head_end = request.find("\r\n\r\n");
        if (head_end == std::string::npos) {
            continue;
        }

        const std::size_t expected_size = head_end + 4
            + parse_content_length(std::string_view{request}.substr(0, head_end + 2));
        if (request.size() >= expected_size) {
            return request;
        }
    }
}

std::string make_url(std::uint16_t port, std::string_view target = "/") {
    return "http://127.0.0.1:" + std::to_string(port) + std::string{target};
}

void configure_test_timeouts(Client& client) {
    client.set_connect_timeout(std::chrono::seconds{2});
    client.set_write_timeout(std::chrono::seconds{2});
    client.set_read_timeout(std::chrono::seconds{2});
}

TEST(ClientRedirectTest, FollowsRelativeRedirectOnReusableConnection) {
    auto server = make_loopback_server();
    ASSERT_TRUE(server.listener);

    std::atomic<int> accepted{0};
    std::string first_request;
    std::string second_request;

    std::thread worker([&] {
        NativeSocket peer = accept_one(server.listener.get());
        if (!peer) {
            return;
        }
        ++accepted;

        first_request = receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 302 Found\r\n"
            "Location: ../final?x=1\r\n"
            "Content-Length: 0\r\n"
            "\r\n");

        second_request = receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 2\r\n"
            "\r\n"
            "ok");
    });

    Client client;
    configure_test_timeouts(client);
    auto result = client.get(make_url(server.port, "/a/start"));

    worker.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().status_code(), 200);
    EXPECT_EQ(result.value().body(), "ok");
    EXPECT_EQ(accepted.load(), 1);
    EXPECT_NE(first_request.find("GET /a/start HTTP/1.1\r\n"), std::string::npos);
    EXPECT_NE(second_request.find("GET /final?x=1 HTTP/1.1\r\n"), std::string::npos);
}

TEST(ClientRedirectTest, CanDisableAutomaticRedirectFollowing) {
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
            "HTTP/1.1 302 Found\r\n"
            "Location: /ignored\r\n"
            "Content-Length: 0\r\n"
            "\r\n");
    });

    Client client;
    configure_test_timeouts(client);
    client.set_follow_redirects(false);
    auto result = client.get(make_url(server.port, "/start"));

    worker.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().status_code(), 302);
}

TEST(ClientRedirectTest, EnforcesConfiguredRedirectLimit) {
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
            "HTTP/1.1 302 Found\r\n"
            "Location: /two\r\n"
            "Content-Length: 0\r\n"
            "\r\n");

        (void)receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 302 Found\r\n"
            "Location: /three\r\n"
            "Content-Length: 0\r\n"
            "\r\n");
    });

    Client client;
    configure_test_timeouts(client);
    client.set_max_redirects(1);
    auto result = client.get(make_url(server.port, "/one"));

    worker.join();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::RedirectLimitExceeded);
}

TEST(ClientRedirectTest, Post302BecomesGetAndDropsBodyHeaders) {
    auto server = make_loopback_server();
    ASSERT_TRUE(server.listener);

    std::string first_request;
    std::string second_request;

    std::thread worker([&] {
        NativeSocket peer = accept_one(server.listener.get());
        if (!peer) {
            return;
        }

        first_request = receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 302 Found\r\n"
            "Location: /after\r\n"
            "Content-Length: 0\r\n"
            "\r\n");

        second_request = receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 2\r\n"
            "\r\n"
            "ok");
    });

    const std::string url = make_url(server.port, "/submit");
    Request request{Method::Post, url};
    request.headers().add("Content-Type", "text/plain");
    request.set_body("payload");

    Client client;
    configure_test_timeouts(client);
    auto result = client.request(request);

    worker.join();

    ASSERT_TRUE(result);
    EXPECT_NE(first_request.find("POST /submit HTTP/1.1\r\n"), std::string::npos);
    EXPECT_NE(first_request.find("Content-Length: 7\r\n"), std::string::npos);
    EXPECT_NE(second_request.find("GET /after HTTP/1.1\r\n"), std::string::npos);
    EXPECT_EQ(second_request.find("Content-Length:"), std::string::npos);
    EXPECT_EQ(second_request.find("Content-Type:"), std::string::npos);
    EXPECT_EQ(second_request.find("payload"), std::string::npos);
}

TEST(ClientRedirectTest, Redirect307PreservesMethodBodyAndEntityHeaders) {
    auto server = make_loopback_server();
    ASSERT_TRUE(server.listener);

    std::string second_request;

    std::thread worker([&] {
        NativeSocket peer = accept_one(server.listener.get());
        if (!peer) {
            return;
        }

        (void)receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 307 Temporary Redirect\r\n"
            "Location: /again\r\n"
            "Content-Length: 0\r\n"
            "\r\n");

        second_request = receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 2\r\n"
            "\r\n"
            "ok");
    });

    const std::string url = make_url(server.port, "/submit");
    Request request{Method::Post, url};
    request.headers().add("Content-Type", "text/plain");
    request.set_body("payload");

    Client client;
    configure_test_timeouts(client);
    auto result = client.request(request);

    worker.join();

    ASSERT_TRUE(result);
    EXPECT_NE(second_request.find("POST /again HTTP/1.1\r\n"), std::string::npos);
    EXPECT_NE(second_request.find("Content-Length: 7\r\n"), std::string::npos);
    EXPECT_NE(second_request.find("Content-Type: text/plain\r\n"), std::string::npos);
    ASSERT_GE(second_request.size(), 7U);
    EXPECT_EQ(second_request.substr(second_request.size() - 7), "payload");
}

TEST(ClientRedirectTest, CrossOriginRedirectDropsCredentialsAndRegeneratesHost) {
    auto source = make_loopback_server();
    auto destination = make_loopback_server();
    ASSERT_TRUE(source.listener);
    ASSERT_TRUE(destination.listener);

    std::string source_request;
    std::string destination_request;

    std::thread worker([&] {
        NativeSocket first_peer = accept_one(source.listener.get());
        if (!first_peer) {
            return;
        }
        source_request = receive_request(first_peer.get());

        const std::string redirect =
            "HTTP/1.1 302 Found\r\n"
            "Location: " + make_url(destination.port, "/final") + "\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
        (void)send_all_native(first_peer.get(), redirect);

        NativeSocket second_peer = accept_one(destination.listener.get());
        if (!second_peer) {
            return;
        }
        destination_request = receive_request(second_peer.get());
        (void)send_all_native(
            second_peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 2\r\n"
            "\r\n"
            "ok");
    });

    const std::string url = make_url(source.port, "/start");
    Request request{Method::Get, url};
    request.headers().add("Host", "override.invalid");
    request.headers().add("Authorization", "Bearer secret");
    request.headers().add("Cookie", "session=secret");

    Client client;
    configure_test_timeouts(client);
    auto result = client.request(request);

    worker.join();

    ASSERT_TRUE(result);
    EXPECT_NE(source_request.find("Authorization: Bearer secret\r\n"), std::string::npos);
    EXPECT_NE(source_request.find("Cookie: session=secret\r\n"), std::string::npos);
    EXPECT_EQ(destination_request.find("Authorization:"), std::string::npos);
    EXPECT_EQ(destination_request.find("Cookie:"), std::string::npos);
    EXPECT_EQ(destination_request.find("override.invalid"), std::string::npos);
    EXPECT_NE(
        destination_request.find(
            "Host: 127.0.0.1:" + std::to_string(destination.port) + "\r\n"),
        std::string::npos);
}

TEST(ClientRedirectTest, RejectsRedirectToHttps) {
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
            "HTTP/1.1 302 Found\r\n"
            "Location: https://example.com/final\r\n"
            "Content-Length: 0\r\n"
            "\r\n");
    });

    Client client;
    configure_test_timeouts(client);
    auto result = client.get(make_url(server.port, "/start"));

    worker.join();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::UnsupportedRedirectScheme);
}

TEST(ClientRedirectTest, RedirectWithoutLocationIsStructuredError) {
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
            "HTTP/1.1 301 Moved Permanently\r\n"
            "Content-Length: 0\r\n"
            "\r\n");
    });

    Client client;
    configure_test_timeouts(client);
    auto result = client.get(make_url(server.port, "/start"));

    worker.join();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::MissingRedirectLocation);
}

} // namespace
