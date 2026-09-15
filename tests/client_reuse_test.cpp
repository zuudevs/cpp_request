#include <gtest/gtest.h>

#include "platform/native_socket.hpp"
#include "platform/network_runtime.hpp"

#include <cpp_request/client.hpp>
#include <cpp_request/request.hpp>

#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
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

TEST(ClientReuseTest, ReusesOneSocketForSequentialSameOriginRequests) {
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
        if (first_request.empty()) {
            return;
        }
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 3\r\n"
            "\r\n"
            "one");

        second_request = receive_request(peer.get());
        if (second_request.empty()) {
            return;
        }
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 3\r\n"
            "\r\n"
            "two");
    });

    Client client;
    configure_test_timeouts(client);

    auto first = client.get(make_url(server.port, "/one"));
    auto second = client.get(make_url(server.port, "/two"));

    worker.join();

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first.value().body(), "one");
    EXPECT_EQ(second.value().body(), "two");
    EXPECT_EQ(accepted.load(), 1);
    EXPECT_NE(first_request.find("GET /one HTTP/1.1\r\n"), std::string::npos);
    EXPECT_NE(second_request.find("GET /two HTTP/1.1\r\n"), std::string::npos);
}

TEST(ClientReuseTest, ConnectionCloseResponseForcesReconnect) {
    auto server = make_loopback_server();
    ASSERT_TRUE(server.listener);

    std::atomic<int> accepted{0};
    std::thread worker([&] {
        NativeSocket first_peer = accept_one(server.listener.get());
        if (!first_peer) {
            return;
        }
        ++accepted;
        (void)receive_request(first_peer.get());
        (void)send_all_native(
            first_peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 3\r\n"
            "Connection: close\r\n"
            "\r\n"
            "one");
        first_peer.close();

        NativeSocket second_peer = accept_one(server.listener.get());
        if (!second_peer) {
            return;
        }
        ++accepted;
        (void)receive_request(second_peer.get());
        (void)send_all_native(
            second_peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 3\r\n"
            "\r\n"
            "two");
    });

    Client client;
    configure_test_timeouts(client);

    auto first = client.get(make_url(server.port, "/one"));
    auto second = client.get(make_url(server.port, "/two"));

    worker.join();

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first.value().body(), "one");
    EXPECT_EQ(second.value().body(), "two");
    EXPECT_EQ(accepted.load(), 2);
}

TEST(ClientReuseTest, CloseDelimitedResponseForcesReconnect) {
    auto server = make_loopback_server();
    ASSERT_TRUE(server.listener);

    std::atomic<int> accepted{0};
    std::thread worker([&] {
        NativeSocket first_peer = accept_one(server.listener.get());
        if (!first_peer) {
            return;
        }
        ++accepted;
        (void)receive_request(first_peer.get());
        (void)send_all_native(
            first_peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "close-body");
        first_peer.close();

        NativeSocket second_peer = accept_one(server.listener.get());
        if (!second_peer) {
            return;
        }
        ++accepted;
        (void)receive_request(second_peer.get());
        (void)send_all_native(
            second_peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "fresh");
    });

    Client client;
    configure_test_timeouts(client);

    auto first = client.get(make_url(server.port, "/close"));
    auto second = client.get(make_url(server.port, "/fresh"));

    worker.join();

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first.value().body(), "close-body");
    EXPECT_EQ(second.value().body(), "fresh");
    EXPECT_EQ(accepted.load(), 2);
}

TEST(ClientReuseTest, DifferentPortReplacesRetainedConnection) {
    auto first_server = make_loopback_server();
    auto second_server = make_loopback_server();
    ASSERT_TRUE(first_server.listener);
    ASSERT_TRUE(second_server.listener);

    std::thread first_worker([&] {
        NativeSocket peer = accept_one(first_server.listener.get());
        if (!peer) {
            return;
        }
        (void)receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 3\r\n"
            "\r\n"
            "one");
        (void)wait_readable(peer.get(), std::chrono::seconds{3});
    });

    std::thread second_worker([&] {
        NativeSocket peer = accept_one(second_server.listener.get());
        if (!peer) {
            return;
        }
        (void)receive_request(peer.get());
        (void)send_all_native(
            peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 3\r\n"
            "\r\n"
            "two");
    });

    Client client;
    configure_test_timeouts(client);

    auto first = client.get(make_url(first_server.port));
    auto second = client.get(make_url(second_server.port));

    first_worker.join();
    second_worker.join();

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first.value().body(), "one");
    EXPECT_EQ(second.value().body(), "two");
}

TEST(ClientReuseTest, StaleKeepAliveFailureIsDiscardedWithoutAutomaticRetry) {
    auto server = make_loopback_server();
    ASSERT_TRUE(server.listener);

    std::promise<void> stale_closed_promise;
    auto stale_closed = stale_closed_promise.get_future();

    std::thread worker([&] {
        NativeSocket stale_peer = accept_one(server.listener.get());
        if (!stale_peer) {
            stale_closed_promise.set_value();
            return;
        }

        (void)receive_request(stale_peer.get());
        (void)send_all_native(
            stale_peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 3\r\n"
            "\r\n"
            "one");
        stale_peer.close();
        stale_closed_promise.set_value();

        NativeSocket fresh_peer = accept_one(server.listener.get(), std::chrono::seconds{6});
        if (!fresh_peer) {
            return;
        }
        (void)receive_request(fresh_peer.get());
        (void)send_all_native(
            fresh_peer.get(),
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "fresh");
    });

    Client client;
    configure_test_timeouts(client);

    auto first = client.get(make_url(server.port, "/first"));
    ASSERT_TRUE(first);
    EXPECT_EQ(first.value().body(), "one");

    stale_closed.wait();

    auto stale_attempt = client.get(make_url(server.port, "/stale"));
    EXPECT_FALSE(stale_attempt);

    auto fresh_attempt = client.get(make_url(server.port, "/fresh"));

    worker.join();

    ASSERT_TRUE(fresh_attempt);
    EXPECT_EQ(fresh_attempt.value().body(), "fresh");
}

} // namespace
