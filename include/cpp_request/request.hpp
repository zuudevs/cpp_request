#pragma once

#include <string>
#include <string_view>
#include <vector>

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
    struct QueryParam final {
        std::string name;
        std::string value;
    };

    Request(Method method, std::string_view url) noexcept
        : method_(method), url_(url) {}

    [[nodiscard]] Method method() const noexcept { return method_; }
    [[nodiscard]] std::string_view url() const noexcept { return url_; }

    [[nodiscard]] Headers& headers() noexcept { return headers_; }
    [[nodiscard]] const Headers& headers() const noexcept { return headers_; }

    void set_body(std::string_view body) noexcept { body_ = body; }
    [[nodiscard]] std::string_view body() const noexcept { return body_; }

    void add_query_param(std::string_view name, std::string_view value) {
        QueryParam param{std::string{name}, std::string{value}};
        query_params_.push_back(std::move(param));
    }

    [[nodiscard]] const std::vector<QueryParam>& query_params() const noexcept {
        return query_params_;
    }

private:
    Method method_;
    std::string_view url_;
    Headers headers_;
    std::string_view body_;
    std::vector<QueryParam> query_params_;
};

} // namespace cpp_request
