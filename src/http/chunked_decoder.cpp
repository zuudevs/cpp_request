#include "http/chunked_decoder.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>

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

[[nodiscard]] constexpr bool is_ows(unsigned char ch) noexcept {
    return ch == ' ' || ch == '\t';
}

[[nodiscard]] bool would_exceed(
    std::size_t current,
    std::size_t addition,
    std::size_t limit) noexcept {
    return current > limit || addition > limit - current;
}

[[nodiscard]] bool valid_field_name(std::string_view name) noexcept {
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

void skip_ows(std::string_view text, std::size_t& cursor) noexcept {
    while (cursor < text.size()
        && is_ows(static_cast<unsigned char>(text[cursor]))) {
        ++cursor;
    }
}

[[nodiscard]] bool parse_token(
    std::string_view text,
    std::size_t& cursor) noexcept {
    const std::size_t begin = cursor;
    while (cursor < text.size()
        && is_tchar(static_cast<unsigned char>(text[cursor]))) {
        ++cursor;
    }
    return cursor != begin;
}

[[nodiscard]] constexpr bool is_qdtext(unsigned char ch) noexcept {
    return ch == '\t'
        || ch == ' '
        || ch == 0x21
        || (ch >= 0x23 && ch <= 0x5b)
        || (ch >= 0x5d && ch <= 0x7e)
        || ch >= 0x80;
}

[[nodiscard]] constexpr bool is_quoted_pair_char(unsigned char ch) noexcept {
    return ch == '\t'
        || ch == ' '
        || (ch >= 0x21 && ch <= 0x7e)
        || ch >= 0x80;
}

[[nodiscard]] bool parse_quoted_string(
    std::string_view text,
    std::size_t& cursor) noexcept {
    if (cursor >= text.size() || text[cursor] != '"') {
        return false;
    }
    ++cursor;

    while (cursor < text.size()) {
        const unsigned char ch = static_cast<unsigned char>(text[cursor]);
        if (ch == '"') {
            ++cursor;
            return true;
        }

        if (ch == '\\') {
            ++cursor;
            if (cursor >= text.size()
                || !is_quoted_pair_char(
                    static_cast<unsigned char>(text[cursor]))) {
                return false;
            }
            ++cursor;
            continue;
        }

        if (!is_qdtext(ch)) {
            return false;
        }
        ++cursor;
    }

    return false;
}

[[nodiscard]] bool valid_chunk_extensions(std::string_view extension) noexcept {
    if (extension.empty()) {
        return false;
    }

    std::size_t cursor = 0;
    while (true) {
        skip_ows(extension, cursor);
        if (!parse_token(extension, cursor)) {
            return false;
        }

        skip_ows(extension, cursor);
        if (cursor < extension.size() && extension[cursor] == '=') {
            ++cursor;
            skip_ows(extension, cursor);
            if (cursor >= extension.size()) {
                return false;
            }

            if (extension[cursor] == '"') {
                if (!parse_quoted_string(extension, cursor)) {
                    return false;
                }
            } else if (!parse_token(extension, cursor)) {
                return false;
            }
            skip_ows(extension, cursor);
        }

        if (cursor == extension.size()) {
            return true;
        }
        if (extension[cursor] != ';') {
            return false;
        }
        ++cursor;
        if (cursor == extension.size()) {
            return false;
        }
    }
}

[[nodiscard]] Result<std::size_t> parse_chunk_size(std::string_view line) noexcept {
    const std::size_t semicolon = line.find(';');
    std::string_view size_text = line.substr(0, semicolon);
    while (!size_text.empty()
        && is_ows(static_cast<unsigned char>(size_text.back()))) {
        size_text.remove_suffix(1);
    }

    if (size_text.empty()) {
        return Error{ErrorCode::InvalidChunkSize};
    }

    if (semicolon != std::string_view::npos
        && !valid_chunk_extensions(line.substr(semicolon + 1))) {
        return Error{ErrorCode::InvalidChunkFraming};
    }

    std::size_t value = 0;
    const char* const first = size_text.data();
    const char* const last = size_text.data() + size_text.size();
    const auto parsed = std::from_chars(first, last, value, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != last) {
        return Error{ErrorCode::InvalidChunkSize};
    }
    return value;
}

[[nodiscard]] bool valid_trailer_line(std::string_view line) noexcept {
    if (line.empty() || line.front() == ' ' || line.front() == '\t') {
        return false;
    }

    const std::size_t colon = line.find(':');
    if (colon == std::string_view::npos || colon == 0) {
        return false;
    }

    const std::string_view name = line.substr(0, colon);
    const std::string_view value = line.substr(colon + 1);
    if (!valid_field_name(name) || !valid_field_value(value)) {
        return false;
    }

    return !ascii_iequals(name, "Content-Length")
        && !ascii_iequals(name, "Transfer-Encoding");
}

void consume_prefix(std::string& input, std::size_t count) {
    if (count != 0) {
        input.erase(0, count);
    }
}

} // namespace

Result<ChunkDecodeProgress> ChunkedDecoder::process(
    std::string& input,
    std::string& output) {
    std::size_t cursor = 0;

    while (true) {
        switch (stage_) {
        case Stage::SizeLine: {
            const std::size_t line_end = input.find("\r\n", cursor);
            if (line_end == std::string::npos) {
                if (input.size() - cursor > limits_.max_chunk_line_bytes) {
                    return Error{ErrorCode::ResponseLimitExceeded};
                }
                consume_prefix(input, cursor);
                return ChunkDecodeProgress::NeedMore;
            }

            if (line_end - cursor > limits_.max_chunk_line_bytes) {
                return Error{ErrorCode::ResponseLimitExceeded};
            }

            const std::string_view line{
                input.data() + cursor,
                line_end - cursor};
            auto size = parse_chunk_size(line);
            if (!size) {
                return size.error();
            }

            if (size.value() != 0
                && would_exceed(output.size(), size.value(), limits_.max_body_bytes)) {
                return Error{ErrorCode::ResponseLimitExceeded};
            }

            cursor = line_end + 2;
            chunk_remaining_ = size.value();
            stage_ = chunk_remaining_ == 0
                ? Stage::Trailers
                : Stage::Data;
            continue;
        }

        case Stage::Data: {
            const std::size_t available = input.size() - cursor;
            const std::size_t amount = std::min(chunk_remaining_, available);
            if (amount != 0) {
                output.append(input.data() + cursor, amount);
                cursor += amount;
                chunk_remaining_ -= amount;
            }

            if (chunk_remaining_ != 0) {
                consume_prefix(input, cursor);
                return ChunkDecodeProgress::NeedMore;
            }

            stage_ = Stage::DataCrlf;
            continue;
        }

        case Stage::DataCrlf:
            if (input.size() - cursor < 2) {
                consume_prefix(input, cursor);
                return ChunkDecodeProgress::NeedMore;
            }
            if (input[cursor] != '\r' || input[cursor + 1] != '\n') {
                return Error{ErrorCode::InvalidChunkFraming};
            }
            cursor += 2;
            stage_ = Stage::SizeLine;
            continue;

        case Stage::Trailers: {
            const std::size_t line_end = input.find("\r\n", cursor);
            if (line_end == std::string::npos) {
                const std::size_t pending = input.size() - cursor;
                if (would_exceed(
                        trailer_bytes_seen_,
                        pending,
                        limits_.max_trailer_bytes)) {
                    return Error{ErrorCode::ResponseLimitExceeded};
                }
                consume_prefix(input, cursor);
                return ChunkDecodeProgress::NeedMore;
            }

            const std::size_t line_bytes = line_end - cursor + 2;
            if (would_exceed(
                    trailer_bytes_seen_,
                    line_bytes,
                    limits_.max_trailer_bytes)) {
                return Error{ErrorCode::ResponseLimitExceeded};
            }
            trailer_bytes_seen_ += line_bytes;

            const std::string_view line{
                input.data() + cursor,
                line_end - cursor};
            cursor = line_end + 2;

            if (line.empty()) {
                stage_ = Stage::Complete;
                consume_prefix(input, cursor);
                return ChunkDecodeProgress::Complete;
            }

            if (!valid_trailer_line(line)) {
                return Error{ErrorCode::InvalidHeader};
            }
            continue;
        }

        case Stage::Complete:
            consume_prefix(input, cursor);
            return ChunkDecodeProgress::Complete;
        }
    }
}

} // namespace cpp_request::detail::http
