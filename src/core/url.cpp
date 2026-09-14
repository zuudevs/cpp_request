#include <cpp_request/url.hpp>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>

namespace cpp_request {
namespace {

[[nodiscard]] bool ascii_iequals(std::string_view lhs, std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        const auto a = static_cast<unsigned char>(lhs[i]);
        const auto b = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(a) != std::tolower(b)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool contains_forbidden_url_byte(std::string_view input) noexcept {
    for (const unsigned char ch : input) {
        if (ch <= 0x20 || ch == 0x7f) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] Result<std::uint16_t> parse_port(std::string_view text) noexcept {
    if (text.empty()) {
        return Error{ErrorCode::InvalidPort};
    }

    unsigned int value = 0;
    const char* const begin = text.data();
    const char* const end = text.data() + text.size();
    const auto parsed = std::from_chars(begin, end, value, 10);

    if (parsed.ec != std::errc{} || parsed.ptr != end || value == 0 ||
        value > std::numeric_limits<std::uint16_t>::max()) {
        return Error{ErrorCode::InvalidPort};
    }

    return static_cast<std::uint16_t>(value);
}

} // namespace

Result<Url> Url::parse(std::string_view input) {
    if (input.empty() || contains_forbidden_url_byte(input)) {
        return Error{ErrorCode::InvalidUrl};
    }

    const std::size_t scheme_separator = input.find("://");
    if (scheme_separator == std::string_view::npos || scheme_separator == 0) {
        return Error{ErrorCode::InvalidUrl};
    }

    const std::string_view input_scheme = input.substr(0, scheme_separator);
    if (!ascii_iequals(input_scheme, "http")) {
        return Error{ErrorCode::UnsupportedScheme};
    }

    const std::size_t fragment = input.find('#');
    const std::string_view without_fragment = input.substr(0, fragment);

    Url result;
    result.storage_.assign(without_fragment.data(), without_fragment.size());
    std::transform(
        result.storage_.begin(),
        result.storage_.begin() + static_cast<std::ptrdiff_t>(scheme_separator),
        result.storage_.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    result.scheme_begin_ = 0;
    result.scheme_size_ = scheme_separator;

    const std::size_t authority_begin = scheme_separator + 3;
    if (authority_begin >= result.storage_.size()) {
        return Error{ErrorCode::InvalidUrl};
    }

    std::size_t authority_end = result.storage_.find_first_of("/?", authority_begin);
    if (authority_end == std::string::npos) {
        authority_end = result.storage_.size();
    }
    if (authority_end == authority_begin) {
        return Error{ErrorCode::InvalidUrl};
    }

    const std::string_view authority{
        result.storage_.data() + authority_begin,
        authority_end - authority_begin};

    if (authority.find('@') != std::string_view::npos) {
        return Error{ErrorCode::InvalidUrl};
    }

    if (authority.front() == '[') {
        const std::size_t closing = authority.find(']');
        if (closing == std::string_view::npos || closing == 1) {
            return Error{ErrorCode::InvalidUrl};
        }

        result.host_is_ipv6_literal_ = true;
        result.host_begin_ = authority_begin + 1;
        result.host_size_ = closing - 1;

        const std::string_view remainder = authority.substr(closing + 1);
        if (!remainder.empty()) {
            if (remainder.front() != ':') {
                return Error{ErrorCode::InvalidUrl};
            }
            auto port = parse_port(remainder.substr(1));
            if (!port) {
                return port.error();
            }
            result.port_ = port.value();
            result.has_explicit_port_ = true;
        }
    } else {
        const std::size_t colon = authority.rfind(':');
        if (colon != std::string_view::npos) {
            if (authority.find(':') != colon) {
                return Error{ErrorCode::InvalidUrl};
            }

            const std::string_view host = authority.substr(0, colon);
            if (host.empty()) {
                return Error{ErrorCode::InvalidUrl};
            }

            auto port = parse_port(authority.substr(colon + 1));
            if (!port) {
                return port.error();
            }

            result.host_begin_ = authority_begin;
            result.host_size_ = host.size();
            result.port_ = port.value();
            result.has_explicit_port_ = true;
        } else {
            result.host_begin_ = authority_begin;
            result.host_size_ = authority.size();
        }
    }

    if (result.host_size_ == 0) {
        return Error{ErrorCode::InvalidUrl};
    }

    if (authority_end == result.storage_.size()) {
        result.storage_.push_back('/');
    } else if (result.storage_[authority_end] == '?') {
        result.storage_.insert(authority_end, 1, '/');
    }

    const std::size_t path_begin = authority_end;
    const std::size_t query_marker = result.storage_.find('?', path_begin);

    result.path_begin_ = path_begin;
    if (query_marker == std::string::npos) {
        result.path_size_ = result.storage_.size() - path_begin;
        result.query_begin_ = result.storage_.size();
        result.query_size_ = 0;
    } else {
        result.path_size_ = query_marker - path_begin;
        result.query_begin_ = query_marker + 1;
        result.query_size_ = result.storage_.size() - result.query_begin_;
    }

    result.target_begin_ = path_begin;
    result.target_size_ = result.storage_.size() - path_begin;

    return result;
}

std::string_view Url::scheme() const noexcept {
    return {storage_.data() + scheme_begin_, scheme_size_};
}

std::string_view Url::host() const noexcept {
    return {storage_.data() + host_begin_, host_size_};
}

std::string_view Url::path() const noexcept {
    return {storage_.data() + path_begin_, path_size_};
}

std::string_view Url::query() const noexcept {
    return {storage_.data() + query_begin_, query_size_};
}

std::string_view Url::target() const noexcept {
    return {storage_.data() + target_begin_, target_size_};
}

} // namespace cpp_request
