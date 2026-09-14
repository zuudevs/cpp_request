#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <cpp_request/result.hpp>

namespace cpp_request {

class Url {
public:
    [[nodiscard]] static Result<Url> parse(std::string_view input);

    [[nodiscard]] std::string_view scheme() const noexcept;
    [[nodiscard]] std::string_view host() const noexcept;
    [[nodiscard]] std::string_view path() const noexcept;
    [[nodiscard]] std::string_view query() const noexcept;
    [[nodiscard]] std::string_view target() const noexcept;

    [[nodiscard]] std::uint16_t port() const noexcept { return port_; }
    [[nodiscard]] bool has_explicit_port() const noexcept { return has_explicit_port_; }
    [[nodiscard]] bool host_is_ipv6_literal() const noexcept { return host_is_ipv6_literal_; }

private:
    std::string storage_;

    std::size_t scheme_begin_{0};
    std::size_t scheme_size_{0};
    std::size_t host_begin_{0};
    std::size_t host_size_{0};
    std::size_t path_begin_{0};
    std::size_t path_size_{0};
    std::size_t query_begin_{0};
    std::size_t query_size_{0};
    std::size_t target_begin_{0};
    std::size_t target_size_{0};

    std::uint16_t port_{80};
    bool has_explicit_port_{false};
    bool host_is_ipv6_literal_{false};
};

} // namespace cpp_request
