#include "http/response_parser.hpp"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace cpp_request::detail::http {
namespace {

struct ParsedHead {
    int status_code{0};
    std::string reason;
    Headers headers;
};

struct FramingInfo {
    std::size_t content_length_count{0};
    std::size_t content_length{0};
    std::size_t transfer_encoding_fields{0};
    std::size_t transfer_encoding_tokens{0};
    bool transfer_encoding_is_chunked{true};
    bool connection_close_requested{false};
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

    for (const char ch : name) {
        const auto byte = static_cast<unsigned char>(ch);
        if (!is_tchar(byte)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool valid_field_value(std::string_view value) noexcept {
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

[[nodiscard]] std::string_view trim_ows(std::string_view value) noexcept {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
        value.remove_prefix(1);
    }
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
        value.remove_suffix(1);
    }
    return value;
}

[[nodiscard]] bool would_exceed(
    std::size_t current,
    std::size_t addition,
    std::size_t limit) noexcept {
    return current > limit || addition > limit - current;
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
    for (const char ch : reason) {
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

[[nodiscard]] Result<FramingInfo> analyze_framing(const Headers& headers) {
    FramingInfo framing;

    for (const auto& field : headers) {
        if (ascii_iequals(field.name, "Connection")
            && contains_token(field.value, "close")) {
            framing.connection_close_requested = true;
        }

        if (ascii_iequals(field.name, "Content-Length")) {
            ++framing.content_length_count;
            if (!parse_content_length(field.value, framing.content_length)) {
                return Error{ErrorCode::InvalidContentLength};
            }
            continue;
        }

        if (!ascii_iequals(field.name, "Transfer-Encoding")) {
            continue;
        }

        ++framing.transfer_encoding_fields;
        const std::string_view field_value{field.value};
        std::size_t token_begin = 0;
        while (token_begin <= field_value.size()) {
            const std::size_t comma = field_value.find(',', token_begin);
            const std::size_t token_end = comma == std::string_view::npos
                ? field_value.size()
                : comma;
            const std::string_view token = trim_ows(field_value.substr(
                token_begin,
                token_end - token_begin));
            if (token.empty()) {
                return Error{ErrorCode::MalformedResponse};
            }

            ++framing.transfer_encoding_tokens;
            if (!ascii_iequals(token, "chunked")) {
                framing.transfer_encoding_is_chunked = false;
            }

            if (comma == std::string_view::npos) {
                break;
            }
            token_begin = comma + 1;
        }
    }

    if (framing.content_length_count > 1) {
        return Error{ErrorCode::InvalidContentLength};
    }

    return framing;
}

[[nodiscard]] bool is_header_terminated_response(
    Method request_method,
    int status_code) noexcept {
    return request_method == Method::Head
        || status_code == 101
        || status_code == 204
        || status_code == 304;
}

[[nodiscard]] bool is_interim_status(int status_code) noexcept {
    return status_code >= 100 && status_code < 200 && status_code != 101;
}

[[nodiscard]] bool framing_header_forbidden_on_status(int status_code) noexcept {
    return status_code == 101 || status_code == 204;
}

} // namespace

ResponseParser::ResponseParser(
    Method request_method,
    ResponseLimits limits) noexcept
    : request_method_(request_method),
      limits_(limits),
      chunked_decoder_(limits) {}

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
                if (buffer_.size() > limits_.max_head_bytes) {
                    return Error{ErrorCode::ResponseLimitExceeded};
                }
                return ResponseParseProgress::NeedMore;
            }

            if (would_exceed(head_end, 4, limits_.max_head_bytes)) {
                return Error{ErrorCode::ResponseLimitExceeded};
            }

            auto parsed = parse_head(std::string_view{buffer_.data(), head_end});
            if (!parsed) {
                return parsed.error();
            }

            buffer_.erase(0, head_end + 4);

            ParsedHead head = std::move(parsed).value();
            auto framing_result = analyze_framing(head.headers);
            if (!framing_result) {
                return framing_result.error();
            }
            const FramingInfo framing = framing_result.value();

            if (is_interim_status(head.status_code)) {
                if (framing.content_length_count != 0
                    || framing.transfer_encoding_fields != 0) {
                    return Error{ErrorCode::MalformedResponse};
                }
                continue;
            }

            response_.status_code_ = head.status_code;
            response_.reason_ = std::move(head.reason);
            response_.headers_ = std::move(head.headers);
            connection_close_requested_ = framing.connection_close_requested
                || response_.status_code_ == 101;

            if (framing.transfer_encoding_fields != 0
                && framing.content_length_count != 0) {
                return Error{ErrorCode::ConflictingMessageFraming};
            }

            if (framing_header_forbidden_on_status(response_.status_code_)
                && (framing.content_length_count != 0
                    || framing.transfer_encoding_fields != 0)) {
                return Error{ErrorCode::MalformedResponse};
            }

            if (is_header_terminated_response(request_method_, response_.status_code_)) {
                stage_ = Stage::Complete;
                return ResponseParseProgress::Complete;
            }

            if (response_.status_code_ == 205) {
                body_forbidden_ = true;

                if (framing.transfer_encoding_fields != 0) {
                    if (framing.transfer_encoding_tokens != 1
                        || !framing.transfer_encoding_is_chunked) {
                        return Error{ErrorCode::MalformedResponse};
                    }
                    stage_ = Stage::ChunkedBody;
                    continue;
                }

                if (framing.content_length_count == 1) {
                    if (framing.content_length != 0) {
                        return Error{ErrorCode::InvalidContentLength};
                    }
                    stage_ = Stage::Complete;
                    return ResponseParseProgress::Complete;
                }

                close_delimited_ = true;
                stage_ = Stage::CloseDelimitedBody;
                continue;
            }

            if (framing.transfer_encoding_fields != 0) {
                if (framing.transfer_encoding_tokens != 1
                    || !framing.transfer_encoding_is_chunked) {
                    return Error{ErrorCode::MalformedResponse};
                }
                stage_ = Stage::ChunkedBody;
                continue;
            }

            if (framing.content_length_count == 1) {
                if (framing.content_length > limits_.max_body_bytes) {
                    return Error{ErrorCode::ResponseLimitExceeded};
                }
                content_length_remaining_ = framing.content_length;
                response_.body_.reserve(framing.content_length);
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
            if (body_forbidden_ && !buffer_.empty()) {
                return Error{ErrorCode::MalformedResponse};
            }
            if (!buffer_.empty()) {
                if (would_exceed(
                        response_.body_.size(),
                        buffer_.size(),
                        limits_.max_body_bytes)) {
                    return Error{ErrorCode::ResponseLimitExceeded};
                }
                response_.body_.append(buffer_);
                buffer_.clear();
            }
            return ResponseParseProgress::NeedMore;

        case Stage::ChunkedBody: {
            auto decoded = chunked_decoder_.process(buffer_, response_.body_);
            if (!decoded) {
                return decoded.error();
            }
            if (body_forbidden_ && !response_.body_.empty()) {
                return Error{ErrorCode::MalformedResponse};
            }
            if (decoded.value() == ChunkDecodeProgress::NeedMore) {
                return ResponseParseProgress::NeedMore;
            }
            stage_ = Stage::Complete;
            return ResponseParseProgress::Complete;
        }

        case Stage::Complete:
            return ResponseParseProgress::Complete;
        }
    }
}

Result<ResponseParseProgress> ResponseParser::finish_eof() {
    switch (stage_) {
    case Stage::CloseDelimitedBody:
        if (body_forbidden_ && !buffer_.empty()) {
            return Error{ErrorCode::MalformedResponse};
        }
        if (!buffer_.empty()) {
            if (would_exceed(
                    response_.body_.size(),
                    buffer_.size(),
                    limits_.max_body_bytes)) {
                return Error{ErrorCode::ResponseLimitExceeded};
            }
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
