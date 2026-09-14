#pragma once

#include <string>
#include <string_view>

#include <cpp_request/headers.hpp>

namespace cpp_request {

namespace detail::http {
class ResponseParser;
}

class Response {
public:
    [[nodiscard]] int status_code() const noexcept { return status_code_; }
    [[nodiscard]] std::string_view reason() const noexcept { return reason_; }
    [[nodiscard]] const Headers& headers() const noexcept { return headers_; }
    [[nodiscard]] std::string_view body() const noexcept { return body_; }
    [[nodiscard]] const std::string& body_storage() const noexcept { return body_; }

private:
    friend class detail::http::ResponseParser;

    int status_code_{0};
    std::string reason_;
    Headers headers_;
    std::string body_;
};

} // namespace cpp_request
