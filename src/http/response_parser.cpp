#include "http/response_parser.hpp"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace cpp_request::detail::http {
namespace {

struct ParsedHead {
    int status_code{0};
    std::string reason;
    Headers headers;
};

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

[[nodiscard]] bool valid_header_name(std::string_view name) noexcept {
    if (name.empty()) {
        return false;
    }

    for (const unsigned char ch : name) {
        if (!is_tchar(ch)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool valid_field_value(std::string_view value) noexcept {
    for (const unsigned char ch : value) {
        if (ch == '\t') {
            continue;
        }
        if (ch < 0x20 || ch == 0x7f) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string_view trim_ows(std::string_view value) noexcept {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
        value.remove_prefix(1);
    }
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
        value.remove_suffix(1);
    }
    return value;
}

[[nodiscard]] bool contains_token(
    std::string_view value,
    std::string_view expected) noexcept {
    std::size_t begin = 0;
    while (begin <= value.size()) {
        const std::size_t comma = value.find(',', begin);
        const std::size_t end = comma == std::string_view::npos
            ? value.size()
            : comma;
        const std::string_view token = trim_ows(value.substr(begin, end - begin));
        if (ascii_iequals(token, expected)) {
            return true;
        }
        if (comma == std::string_view::npos) {
            break;
        }
        begin = comma + 1;
    }
    return false;
}

[[nodiscard]] bool valid_reason_phrase(std::string_view reason) noexcept {
    for (const unsigned char ch : reason) {
        if (ch == '\t') {
            continue;
        }
        if (ch < 0x20 || ch == 0x7f) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] Result<ParsedHead> parse_head(std::string_view block) {
    const std::size_t first_line_end = block.find("\r\n");
    const std::string_view status_line = first_line_end == std::string_view::npos
        ? block
        : block.substr(0, first_line_end);

    constexpr std::string_view kVersionPrefix = "HTTP/1.1 ";
    if (status_line.size() < kVersionPrefix.size() + 3
        || status_line.substr(0, kVersionPrefix.size()) != kVersionPrefix) {
        return Error{ErrorCode::InvalidStatusLine};
    }

    const std::size_t code_begin = kVersionPrefix.size();
    if (status_line[code_begin] < '0' || status_line[code_begin] > '9'
        || status_line[code_begin + 1] < '0' || status_line[code_begin + 1] > '9'
        || status_line[code_begin + 2] < '0' || status_line[code_begin + 2] > '9') {
        return Error{ErrorCode::InvalidStatusLine};
    }

    int status_code = 0;
    const char* const code_first = status_line.data() + code_begin;
    const char* const code_last = code_first + 3;
    const auto parsed_code = std::from_chars(code_first, code_last, status_code, 10);
    if (parsed_code.ec != std::errc{} || parsed_code.ptr != code_last
        || status_code < 100 || status_code > 999) {
        return Error{ErrorCode::InvalidStatusLine};
    }

    std::string_view reason;
    if (status_line.size() > code_begin + 3) {
        if (status_line[code_begin + 3] != ' ') {
            return Error{ErrorCode::InvalidStatusLine};
        }
        reason = status_line.substr(code_begin + 4);
        if (!valid_reason_phrase(reason)) {
            return Error{ErrorCode::InvalidStatusLine};
        }
    }

    ParsedHead result;
    result.status_code = status_code;
    result.reason.assign(reason.data(), reason.size());

    if (first_line_end == std::string_view::npos) {
        return result;
    }

    std::size_t line_begin = first_line_end + 2;
    while (line_begin < block.size()) {
        const std::size_t line_end = block.find("\r\n", line_begin);
        const std::size_t current_end = line_end == std::string_view::npos
            ? block.size()
            : line_end;
        const std::string_view line = block.substr(line_begin, current_end - line_begin);

        if (line.empty() || line.front() == ' ' || line.front() == '\t') {
            return Error{ErrorCode::InvalidHeader};
        }

        const std::size_t colon = line.find(':');
        if (colon == std::string_view::npos || colon == 0) {
            return Error{ErrorCode::InvalidHeader};
        }

        const std::string_view name = line.substr(0, colon);
        const std::string_view value = trim_ows(line.substr(colon + 1));
        if (!valid_header_name(name) || !valid_field_value(value)) {
            return Error{ErrorCode::InvalidHeader};
        }

        result.headers.add(name, value);

        if (line_end == std::string_view::npos) {
            break;
        }
        line_begin = line_end + 2;
    }

    return result;
}

[[nodiscard]] bool parse_content_length(
    std::string_view text,
    std::size_t& value) noexcept {
    if (text.empty()) {
        return false;
    }

    const char* const first = text.data();
    const char* const last = text.data() + text.size();
    const auto parsed = std::from_chars(first, last, value, 10);
    return parsed.ec == std::errc{} && parsed.ptr == last;
}

[[nodiscard]] bool is_no_body_status(int status_code) noexcept {
    return status_code == 101
        || status_code == 204
        || status_code == 205
        || status_code == 304;
}

[[nodiscard]] bool is_interim_status(int status_code) noexcept {
    return status_code >= 100 && status_code < 200 && status_code != 101;
}

} // namespace

ResponseParser::ResponseParser(Method request_method) noexcept
    : request_method_(request_method) {}

Result<ResponseParseProgress> ResponseParser::feed(std::string_view bytes) {
    if (!bytes.empty()) {
        buffer_.append(bytes.data(), bytes.size());
    }
    return process_buffer();
}

Result<ResponseParseProgress> ResponseParser::process_buffer() {
    while (true) {
        switch (stage_) {
        case Stage::Head: {
            const std::size_t head_end = buffer_.find("\r\n\r\n");
            if (head_end == std::string::npos) {
                return ResponseParseProgress::NeedMore;
            }

            auto parsed = parse_head(std::string_view{buffer_.data(), head_end});
            if (!parsed) {
                return parsed.error();
            }

            const std::size_t consumed = head_end + 4;
            buffer_.erase(0, consumed);

            ParsedHead head = std::move(parsed).value();
            if (is_interim_status(head.status_code)) {
                continue;
            }

            response_.status_code_ = head.status_code;
            response_.reason_ = std::move(head.reason);
            response_.headers_ = std::move(head.headers);

            connection_close_requested_ = false;
            std::size_t content_length_count = 0;
            std::size_t content_length = 0;
            std::size_t transfer_encoding_fields = 0;
            std::size_t transfer_encoding_tokens = 0;
            bool transfer_encoding_is_chunked = true;

            for (const auto& field : response_.headers_) {
                if (ascii_iequals(field.name, "Connection")
                    && contains_token(field.value, "close")) {
                    connection_close_requested_ = true;
                }

                if (ascii_iequals(field.name, "Content-Length")) {
                    ++content_length_count;
                    if (!parse_content_length(field.value, content_length)) {
                        return Error{ErrorCode::InvalidContentLength};
                    }
                    continue;
                }

                if (ascii_iequals(field.name, "Transfer-Encoding")) {
                    ++transfer_encoding_fields;
                    std::size_t token_begin = 0;
                    while (token_begin <= field.value.size()) {
                        const std::size_t comma = field.value.find(',', token_begin);
                        const std::size_t token_end = comma == std::string::npos
                            ? field.value.size()
                            : comma;
                        const std::string_view token = trim_ows(
                            std::string_view{field.value}.substr(
                                token_begin,
                                token_end - token_begin));
                        if (token.empty()) {
                            return Error{ErrorCode::MalformedResponse};
                        }
                        ++transfer_encoding_tokens;
                        if (!ascii_iequals(token, "chunked")) {
                            transfer_encoding_is_chunked = false;
                        }
                        if (comma == std::string::npos) {
                            break;
                        }
                        token_begin = comma + 1;
                    }
                }
            }

            if (content_length_count > 1) {
                return Error{ErrorCode::InvalidContentLength};
            }

            const bool no_body = request_method_ == Method::Head
                || is_no_body_status(response_.status_code_);
            if (no_body) {
                stage_ = Stage::Complete;
                return ResponseParseProgress::Complete;
            }

            if (transfer_encoding_fields != 0 && content_length_count != 0) {
                return Error{ErrorCode::ConflictingMessageFraming};
            }

            if (transfer_encoding_fields != 0) {
                if (transfer_encoding_tokens != 1 || !transfer_encoding_is_chunked) {
                    return Error{ErrorCode::MalformedResponse};
                }
                stage_ = Stage::ChunkedBody;
                return ResponseParseProgress::NeedsChunkedDecoder;
            }

            if (content_length_count == 1) {
                content_length_remaining_ = content_length;
                response_.body_.reserve(content_length);
                if (content_length_remaining_ == 0) {
                    stage_ = Stage::Complete;
                    return ResponseParseProgress::Complete;
                }
                stage_ = Stage::ContentLengthBody;
                continue;
            }

            close_delimited_ = true;
            stage_ = Stage::CloseDelimitedBody;
            continue;
        }

        case Stage::ContentLengthBody: {
            const std::size_t amount = std::min(
                content_length_remaining_,
                buffer_.size());
            if (amount != 0) {
                response_.body_.append(buffer_.data(), amount);
                buffer_.erase(0, amount);
                content_length_remaining_ -= amount;
            }

            if (content_length_remaining_ == 0) {
                stage_ = Stage::Complete;
                return ResponseParseProgress::Complete;
            }
            return ResponseParseProgress::NeedMore;
        }

        case Stage::CloseDelimitedBody:
            if (!buffer_.empty()) {
                response_.body_.append(buffer_);
                buffer_.clear();
            }
            return ResponseParseProgress::NeedMore;

        case Stage::ChunkedBody:
            return ResponseParseProgress::NeedsChunkedDecoder;

        case Stage::Complete:
            return ResponseParseProgress::Complete;
        }
    }
}

Result<ResponseParseProgress> ResponseParser::finish_eof() {
    switch (stage_) {
    case Stage::CloseDelimitedBody:
        if (!buffer_.empty()) {
            response_.body_.append(buffer_);
            buffer_.clear();
        }
        stage_ = Stage::Complete;
        return ResponseParseProgress::Complete;

    case Stage::Complete:
        return ResponseParseProgress::Complete;

    case Stage::ContentLengthBody:
    case Stage::Head:
    case Stage::ChunkedBody:
        return Error{ErrorCode::UnexpectedEof};
    }

    return Error{ErrorCode::Unknown};
}

bool ResponseParser::complete() const noexcept {
    return stage_ == Stage::Complete;
}

bool ResponseParser::connection_reusable() const noexcept {
    return complete()
        && !connection_close_requested_
        && !close_delimited_;
}

Response ResponseParser::take_response() {
    assert(complete());
    return std::move(response_);
}

} // namespace cpp_request::detail::http
