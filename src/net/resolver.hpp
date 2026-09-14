#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include <cpp_request/result.hpp>

#include "net/endpoint.hpp"

namespace cpp_request::detail::net {

class Resolver final {
public:
    [[nodiscard]] static Result<std::vector<Endpoint>> resolve(
        std::string_view host,
        std::uint16_t port);
};

} // namespace cpp_request::detail::net
