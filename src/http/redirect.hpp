#pragma once

#include <string>
#include <string_view>

#include <cpp_request/request.hpp>
#include <cpp_request/result.hpp>
#include <cpp_request/url.hpp>

namespace cpp_request::detail::http {

struct RedirectBehavior {
    Method method;
    bool preserve_body;
};

[[nodiscard]] bool is_redirect_status(int status_code) noexcept;

[[nodiscard]] RedirectBehavior redirect_behavior(
    int status_code,
    Method current_method) noexcept;

[[nodiscard]] Result<std::string> resolve_redirect_location(
    const Url& base,
    std::string_view location);

} // namespace cpp_request::detail::http
