/**
 * @file response.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief HTTP response implementation
 * @version 0.1.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#include "cpp_request/response.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace zd::crq {

namespace {

std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return str;
}

// Parse a single header line "Key: Value" into pair
bool parse_header_line(std::string_view line, std::string& key,
                       std::string& value) noexcept {
    auto colon_pos = line.find(':');
    if (colon_pos == std::string_view::npos) {
        return false;
    }

    key = std::string(line.substr(0, colon_pos));

    // Skip whitespace after colon
    size_t value_start = colon_pos + 1;
    while (value_start < line.length() && std::isspace(line[value_start])) {
        value_start++;
    }

    value = std::string(line.substr(value_start));

    // Trim trailing whitespace
    while (!value.empty() && std::isspace(value.back())) {
        value.pop_back();
    }

    return true;
}

} // namespace

std::expected<Response, Error> Response::parse(
    std::vector<char> raw) noexcept {
    Response resp;

    if (raw.empty()) {
        return std::unexpected(Error::InvalidArgument);
    }

    // Find the double CRLF that separates headers from body
    std::string full_response(raw.begin(), raw.end());
    size_t header_end = full_response.find("\r\n\r\n");

    if (header_end == std::string::npos) {
        // Try with just LF (some servers might not use CRLF properly)
        header_end = full_response.find("\n\n");
        if (header_end == std::string::npos) {
            return std::unexpected(Error::InvalidArgument);
        }
        // Adjust for LF-only separator
        header_end += 2;
    } else {
        header_end += 4; // CRLF + CRLF
    }

    std::string headers_section = full_response.substr(0, header_end);

    // Extract body
    if (header_end < full_response.size()) {
        resp.body_.assign(full_response.begin() + header_end,
                          full_response.end());
    }

    // Parse headers section
    std::istringstream header_stream(headers_section);
    std::string line;

    // Parse status line
    if (!std::getline(header_stream, line)) {
        return std::unexpected(Error::InvalidArgument);
    }

    // Remove any trailing \r
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    // Parse "HTTP/1.x NNN Message"
    std::istringstream status_line(line);
    std::string http_version;
    int status_code = 0;
    std::string status_message;

    if (!(status_line >> http_version >> status_code)) {
        return std::unexpected(Error::InvalidArgument);
    }

    std::getline(status_line, status_message);
    if (!status_message.empty() && status_message[0] == ' ') {
        status_message = status_message.substr(1);
    }

    resp.status_code_ = status_code;
    resp.status_message_ = status_message;

    // Parse header lines
    while (std::getline(header_stream, line)) {
        // Remove trailing \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            break; // End of headers
        }

        std::string key, value;
        if (parse_header_line(line, key, value)) {
            resp.headers_[to_lower(key)] = value;
        }
    }

    return resp;
}

std::string_view Response::header(std::string_view name) const noexcept {
    auto key = to_lower(std::string(name));
    auto it = headers_.find(key);
    if (it != headers_.end()) {
        return it->second;
    }
    return {};
}

} // namespace zd::crq
