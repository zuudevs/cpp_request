#pragma once

#include <string>
#include <string_view>

#include <cpp_request/request.hpp>
#include <cpp_request/result.hpp>
#include <cpp_request/url.hpp>

namespace cpp_request::detail::http {

struct SerializedRequest final {
    Url url;
    std::string head;
    std::string_view body;
};

[[nodiscard]] Result<SerializedRequest> serialize_request(const Request& request);

} // namespace cpp_request::detail::http
