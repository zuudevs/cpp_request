#pragma once

#include <string_view>

#include <cpp_request/headers.hpp>

namespace cpp_request {

enum class Method {
    Get,
    Head,
    Post,
    Put,
    Patch,
    Delete
};

class Request {
public:
    Request(Method method, std::string_view url) noexcept
        : method_(method), url_(url) {}

    [[nodiscard]] Method method() const noexcept { return method_; }
    [[nodiscard]] std::string_view url() const noexcept { return url_; }

    [[nodiscard]] Headers& headers() noexcept { return headers_; }
    [[nodiscard]] const Headers& headers() const noexcept { return headers_; }

    void set_body(std::string_view body) noexcept { body_ = body; }
    [[nodiscard]] std::string_view body() const noexcept { return body_; }

private:
    Method method_;
    std::string_view url_;
    Headers headers_;
    std::string_view body_;
};

} // namespace cpp_request
