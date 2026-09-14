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

[[nodiscard]] bool valid_chunk_extension(std::string_view extension) noexcept {
    for (const unsigned char ch : extension) {
        if (ch == '\t') {
            continue;
        }
        if (ch < 0x20 || ch == 0x7f) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] Result<std::size_t> parse_chunk_size(std::string_view line) noexcept {
    const std::size_t semicolon = line.find(';');
    const std::string_view size_text = line.substr(0, semicolon);
    if (size_text.empty()) {
        return Error{ErrorCode::InvalidChunkSize};
    }

    if (semicolon != std::string_view::npos
        && !valid_chunk_extension(line.substr(semicolon + 1))) {
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
                consume_prefix(input, cursor);
                return ChunkDecodeProgress::NeedMore;
            }

            const std::string_view line{
                input.data() + cursor,
                line_end - cursor};
            auto size = parse_chunk_size(line);
            if (!size) {
                return size.error();
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
                consume_prefix(input, cursor);
                return ChunkDecodeProgress::NeedMore;
            }

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
