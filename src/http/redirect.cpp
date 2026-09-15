#include "http/redirect.hpp"

#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <cpp_request/error.hpp>

namespace cpp_request::detail::http {
namespace {

[[nodiscard]] constexpr char ascii_lower(char ch) noexcept {
    return ch >= 'A' && ch <= 'Z'
        ? static_cast<char>(ch + ('a' - 'A'))
        : ch;
}

[[nodiscard]] bool ascii_iequals(
    std::string_view lhs,
    std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (ascii_lower(lhs[index]) != ascii_lower(rhs[index])) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool is_scheme_char(char ch) noexcept {
    const unsigned char value = static_cast<unsigned char>(ch);
    return std::isalnum(value) != 0
        || ch == '+'
        || ch == '-'
        || ch == '.';
}

[[nodiscard]] std::size_t scheme_end(std::string_view value) noexcept {
    if (value.empty()
        || !((value.front() >= 'A' && value.front() <= 'Z')
            || (value.front() >= 'a' && value.front() <= 'z'))) {
        return std::string_view::npos;
    }

    for (std::size_t index = 1; index < value.size(); ++index) {
        const char ch = value[index];
        if (ch == ':') {
            return index;
        }
        if (ch == '/' || ch == '?' || ch == '#') {
            return std::string_view::npos;
        }
        if (!is_scheme_char(ch)) {
            return std::string_view::npos;
        }
    }
    return std::string_view::npos;
}

[[nodiscard]] std::string build_origin(const Url& base) {
    std::string origin{"http://"};
    if (base.host_is_ipv6_literal()) {
        origin.push_back('[');
        origin.append(base.host());
        origin.push_back(']');
    } else {
        origin.append(base.host());
    }

    if (base.has_explicit_port()) {
        origin.push_back(':');
        origin.append(std::to_string(base.port()));
    }
    return origin;
}

[[nodiscard]] std::string remove_dot_segments(std::string_view path) {
    std::vector<std::string_view> segments;
    std::size_t begin = path.empty() || path.front() != '/' ? 0 : 1;

    while (begin <= path.size()) {
        const std::size_t slash = path.find('/', begin);
        const std::size_t end = slash == std::string_view::npos
            ? path.size()
            : slash;
        const std::string_view segment = path.substr(begin, end - begin);

        if (segment == "..") {
            if (!segments.empty()) {
                segments.pop_back();
            }
        } else if (segment != ".") {
            segments.push_back(segment);
        }

        if (slash == std::string_view::npos) {
            break;
        }
        begin = slash + 1;
    }

    std::string normalized{"/"};
    for (std::size_t index = 0; index < segments.size(); ++index) {
        if (index != 0) {
            normalized.push_back('/');
        }
        normalized.append(segments[index]);
    }

    const bool ends_with_dot = path.size() >= 2
        && path.substr(path.size() - 2) == "/.";
    const bool ends_with_dot_dot = path.size() >= 3
        && path.substr(path.size() - 3) == "/..";
    const bool keep_trailing_slash = !path.empty()
        && (path.back() == '/' || ends_with_dot || ends_with_dot_dot);
    if (keep_trailing_slash && normalized.back() != '/') {
        normalized.push_back('/');
    }
    return normalized;
}

[[nodiscard]] std::string strip_fragment(std::string value) {
    const std::size_t fragment = value.find('#');
    if (fragment != std::string::npos) {
        value.resize(fragment);
    }
    return value;
}

[[nodiscard]] Result<std::string> validate_redirect_url(std::string candidate) {
    candidate = strip_fragment(std::move(candidate));
    auto parsed = Url::parse(candidate);
    if (!parsed) {
        if (parsed.error().code == ErrorCode::UnsupportedScheme) {
            return Error{ErrorCode::UnsupportedRedirectScheme};
        }
        return parsed.error();
    }
    return candidate;
}

} // namespace

bool is_redirect_status(int status_code) noexcept {
    return status_code == 301
        || status_code == 302
        || status_code == 303
        || status_code == 307
        || status_code == 308;
}

RedirectBehavior redirect_behavior(
    int status_code,
    Method current_method) noexcept {
    if (status_code == 303) {
        return {
            current_method == Method::Head ? Method::Head : Method::Get,
            false};
    }

    if ((status_code == 301 || status_code == 302)
        && current_method == Method::Post) {
        return {Method::Get, false};
    }

    return {current_method, true};
}

Result<std::string> resolve_redirect_location(
    const Url& base,
    std::string_view location) {
    if (location.empty()) {
        return Error{ErrorCode::MissingRedirectLocation};
    }

    const std::size_t explicit_scheme = scheme_end(location);
    if (explicit_scheme != std::string_view::npos) {
        if (!ascii_iequals(location.substr(0, explicit_scheme), "http")) {
            return Error{ErrorCode::UnsupportedRedirectScheme};
        }
        return validate_redirect_url(std::string{location});
    }

    if (location.rfind("//", 0) == 0) {
        return validate_redirect_url(std::string{"http:"} + std::string{location});
    }

    const std::string origin = build_origin(base);

    if (location.front() == '#') {
        return validate_redirect_url(origin + std::string{base.target()} + std::string{location});
    }

    if (location.front() == '?') {
        return validate_redirect_url(origin + std::string{base.path()} + std::string{location});
    }

    const std::size_t suffix_begin = location.find_first_of("?#");
    const std::string_view relative_path = location.substr(0, suffix_begin);
    const std::string_view suffix = suffix_begin == std::string_view::npos
        ? std::string_view{}
        : location.substr(suffix_begin);

    std::string merged_path;
    if (!relative_path.empty() && relative_path.front() == '/') {
        merged_path.assign(relative_path);
    } else {
        const std::string_view base_path = base.path();
        const std::size_t slash = base_path.rfind('/');
        if (slash == std::string_view::npos) {
            merged_path = "/";
        } else {
            merged_path.assign(base_path.substr(0, slash + 1));
        }
        merged_path.append(relative_path);
    }

    return validate_redirect_url(
        origin + remove_dot_segments(merged_path) + std::string{suffix});
}

} // namespace cpp_request::detail::http
