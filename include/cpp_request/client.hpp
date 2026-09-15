#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <cpp_request/request.hpp>
#include <cpp_request/response.hpp>
#include <cpp_request/result.hpp>

namespace cpp_request {

class Client final {
public:
    Client() noexcept = default;
    ~Client() noexcept;

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&& other) noexcept;
    Client& operator=(Client&& other) noexcept;

    [[nodiscard]] Result<Response> request(const Request& request);

    [[nodiscard]] Result<Response> get(std::string_view url);
    [[nodiscard]] Result<Response> head(std::string_view url);
    [[nodiscard]] Result<Response> post(
        std::string_view url,
        std::string_view body = {});
    [[nodiscard]] Result<Response> put(
        std::string_view url,
        std::string_view body = {});
    [[nodiscard]] Result<Response> patch(
        std::string_view url,
        std::string_view body = {});
    [[nodiscard]] Result<Response> del(std::string_view url);

    void set_connect_timeout(std::chrono::milliseconds timeout) noexcept {
        connect_timeout_ = timeout;
    }

    void set_read_timeout(std::chrono::milliseconds timeout) noexcept {
        read_timeout_ = timeout;
    }

    void set_write_timeout(std::chrono::milliseconds timeout) noexcept {
        write_timeout_ = timeout;
    }

    void set_follow_redirects(bool enabled) noexcept {
        follow_redirects_ = enabled;
    }

    void set_max_redirects(std::size_t count) noexcept {
        max_redirects_ = count;
    }

private:
    [[nodiscard]] Result<Response> execute_once(const Request& request);
    void close_reusable_connection() noexcept;

    std::uintptr_t reusable_socket_token_{0};
    bool has_reusable_connection_{false};
    std::string reusable_host_;
    std::uint16_t reusable_port_{0};

    bool follow_redirects_{true};
    std::size_t max_redirects_{10};

    std::chrono::milliseconds connect_timeout_{5000};
    std::chrono::milliseconds read_timeout_{30000};
    std::chrono::milliseconds write_timeout_{30000};
};

[[nodiscard]] Result<Response> get(std::string_view url);
[[nodiscard]] Result<Response> head(std::string_view url);
[[nodiscard]] Result<Response> post(
    std::string_view url,
    std::string_view body = {});
[[nodiscard]] Result<Response> put(
    std::string_view url,
    std::string_view body = {});
[[nodiscard]] Result<Response> patch(
    std::string_view url,
    std::string_view body = {});
[[nodiscard]] Result<Response> del(std::string_view url);

} // namespace cpp_request
