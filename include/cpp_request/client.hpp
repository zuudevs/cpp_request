#pragma once

#include <chrono>
#include <string_view>

#include <cpp_request/request.hpp>
#include <cpp_request/response.hpp>
#include <cpp_request/result.hpp>

namespace cpp_request {

class Client final {
public:
    Client() noexcept = default;

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

private:
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
