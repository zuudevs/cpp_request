#include "http/request_serializer.hpp"

#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cpp_request::detail::http {
namespace {

[[nodiscard]] constexpr std::string_view method_token(Method method) noexcept {
    switch (method) {
    case Method::Get: return "GET";
    case Method::Head: return "HEAD";
    case Method::Post: return "POST";
    case Method::Put: return "PUT";
    case Method::Patch: return "PATCH";
    case Method::Delete: return "DELETE";
    }
    return {};
}

[[nodiscard]] constexpr bool method_has_content_semantics(Method method) noexcept {
    return method == Method::Post
        || method == Method::Put
        || method == Method::Patch;
}

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

[[nodiscard]] constexpr bool is_tchar(unsigned char ch) noexcept {
    if ((ch >= '0' && ch <= '9')
        || (ch >= 'A' && ch <= 'Z')
        || (ch >= 'a' && ch <= 'z')) {
        return true;
    }

    switch (ch) {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
        return true;
    default:
        return false;
    }
}

[[nodiscard]] constexpr bool is_unreserved(unsigned char ch) noexcept {
    return (ch >= '0' && ch <= '9')
        || (ch >= 'A' && ch <= 'Z')
        || (ch >= 'a' && ch <= 'z')
        || ch == '-'
        || ch == '.'
        || ch == '_'
        || ch == '~';
}

void append_percent_encoded(std::string& output, std::string_view input) {
    constexpr char kHex[] = "0123456789ABCDEF";
    for (const char ch : input) {
        const auto byte = static_cast<unsigned char>(ch);
        if (is_unreserved(byte)) {
            output.push_back(ch);
            continue;
        }

        output.push_back('%');
        output.push_back(kHex[(byte >> 4) & 0x0f]);
        output.push_back(kHex[byte & 0x0f]);
    }
}

void append_query_params(
    std::string& output,
    const std::vector<Request::QueryParam>& params) {
    if (params.empty()) {
        return;
    }

    const bool has_query = output.find('?') != std::string::npos;
    if (!has_query) {
        output.push_back('?');
    } else if (!output.empty() && output.back() != '?' && output.back() != '&') {
        output.push_back('&');
    }

    for (std::size_t index = 0; index < params.size(); ++index) {
        if (index != 0) {
            output.push_back('&');
        }
        append_percent_encoded(output, params[index].name);
        output.push_back('=');
        append_percent_encoded(output, params[index].value);
    }
}

[[nodiscard]] bool valid_header_name(std::string_view name) noexcept {
    if (name.empty()) {
        return false;
    }

    for (const char ch : name) {
        const auto byte = static_cast<unsigned char>(ch);
        if (!is_tchar(byte)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool valid_header_value(std::string_view value) noexcept {
    for (const char ch : value) {
        const auto byte = static_cast<unsigned char>(ch);
        if (byte == '\t') {
            continue;
        }
        if (byte < 0x20u || byte == 0x7fu) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool parse_content_length(
    std::string_view text,
    std::size_t& value) noexcept {
    if (text.empty()) {
        return false;
    }

    const char* const begin = text.data();
    const char* const end = text.data() + text.size();
    const auto parsed = std::from_chars(begin, end, value, 10);
    return parsed.ec == std::errc{} && parsed.ptr == end;
}

void append_decimal(std::string& output, std::size_t value) {
    char buffer[32]{};
    const auto converted = std::to_chars(
        buffer,
        buffer + sizeof(buffer),
        value,
        10);
    output.append(buffer, converted.ptr);
}

void append_port(std::string& output, std::uint16_t port) {
    char buffer[8]{};
    const auto converted = std::to_chars(
        buffer,
        buffer + sizeof(buffer),
        port,
        10);
    output.append(buffer, converted.ptr);
}

void append_generated_host(std::string& output, const Url& url) {
    output.append("Host: ");
    if (url.host_is_ipv6_literal()) {
        output.push_back('[');
        output.append(url.host().data(), url.host().size());
        output.push_back(']');
    } else {
        output.append(url.host().data(), url.host().size());
    }

    if (url.has_explicit_port()) {
        output.push_back(':');
        append_port(output, url.port());
    }
    output.append("\r\n");
}

[[nodiscard]] std::size_t encoded_query_reserve(
    const std::vector<Request::QueryParam>& params) noexcept {
    std::size_t result = 0;
    for (const auto& param : params) {
        result += (param.name.size() + param.value.size()) * 3 + 2;
    }
    return result;
}

} // namespace

std::string effective_request_url(const Request& request) {
    const std::string_view raw = request.url();
    const std::size_t fragment = raw.find('#');
    const std::string_view without_fragment = raw.substr(0, fragment);

    std::string result{without_fragment};
    result.reserve(result.size() + encoded_query_reserve(request.query_params()));
    append_query_params(result, request.query_params());
    return result;
}

Result<SerializedRequest> serialize_request(const Request& request) {
    auto parsed_url = Url::parse(request.url());
    if (!parsed_url) {
        return parsed_url.error();
    }

    const std::string_view method = method_token(request.method());
    if (method.empty()) {
        return Error{ErrorCode::Unknown};
    }

    std::size_t host_count = 0;
    std::size_t content_length_count = 0;
    std::size_t declared_content_length = 0;

    for (const auto& field : request.headers()) {
        if (!valid_header_name(field.name) || !valid_header_value(field.value)) {
            return Error{ErrorCode::InvalidHeader};
        }

        if (ascii_iequals(field.name, "Host")) {
            ++host_count;
            if (field.value.empty()) {
                return Error{ErrorCode::InvalidHeader};
            }
        } else if (ascii_iequals(field.name, "Content-Length")) {
            ++content_length_count;
            if (!parse_content_length(field.value, declared_content_length)) {
                return Error{ErrorCode::InvalidContentLength};
            }
        } else if (ascii_iequals(field.name, "Transfer-Encoding")) {
            return Error{ErrorCode::ConflictingMessageFraming};
        }
    }

    if (host_count > 1) {
        return Error{ErrorCode::InvalidHeader};
    }
    if (content_length_count > 1) {
        return Error{ErrorCode::InvalidContentLength};
    }
    if (content_length_count == 1
        && declared_content_length != request.body().size()) {
        return Error{ErrorCode::InvalidContentLength};
    }

    SerializedRequest serialized;
    serialized.url = std::move(parsed_url).value();
    serialized.body = request.body();

    std::string request_target{serialized.url.target()};
    request_target.reserve(
        request_target.size() + encoded_query_reserve(request.query_params()));
    append_query_params(request_target, request.query_params());

    std::size_t reserve_size = method.size()
        + 1
        + request_target.size()
        + sizeof(" HTTP/1.1\r\n") - 1
        + request.headers().size() * 16
        + 64;
    for (const auto& field : request.headers()) {
        reserve_size += field.name.size() + field.value.size();
    }
    serialized.head.reserve(reserve_size);

    serialized.head.append(method.data(), method.size());
    serialized.head.push_back(' ');
    serialized.head.append(request_target);
    serialized.head.append(" HTTP/1.1\r\n");

    if (host_count == 0) {
        append_generated_host(serialized.head, serialized.url);
    }

    for (const auto& field : request.headers()) {
        serialized.head.append(field.name);
        serialized.head.append(": ");
        serialized.head.append(field.value);
        serialized.head.append("\r\n");
    }

    if (content_length_count == 0
        && (!request.body().empty() || method_has_content_semantics(request.method()))) {
        serialized.head.append("Content-Length: ");
        append_decimal(serialized.head, request.body().size());
        serialized.head.append("\r\n");
    }

    serialized.head.append("\r\n");
    return serialized;
}

} // namespace cpp_request::detail::http
