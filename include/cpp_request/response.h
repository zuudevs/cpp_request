/**
 * @file response.h
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Public HTTP response type
 * @version 0.1.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "error.h"
#include <expected>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace zd::crq {

/**
 * @class Response
 * @brief Parsed HTTP/1.x response.
 *
 * Construct via Response::parse(). All members are populated from the
 * raw byte stream returned by the socket layer.
 */
class Response {
public:
    /**
     * @brief Parse raw HTTP response bytes into a Response.
     *
     * @param raw  Complete response bytes (headers + body).
     * @return     Parsed Response, or an Error if the data is malformed.
     */
    static std::expected<Response, Error> parse(std::vector<char> raw) noexcept;

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------

    /// HTTP status code (e.g. 200, 404).
    int status_code() const noexcept { return status_code_; }

    /// Reason phrase from the status line (e.g. "OK", "Not Found").
    const std::string& status_message() const noexcept { return status_message_; }

    /// Raw body bytes.
    const std::vector<char>& body() const noexcept { return body_; }

    /// Body as a UTF-8 string (copies).
    std::string body_str() const { return {body_.begin(), body_.end()}; }

    /// True when status_code is in the 2xx range.
    bool ok() const noexcept { return status_code_ >= 200 && status_code_ < 300; }

    /**
     * @brief Case-insensitive header lookup.
     *
     * @param name  Header field name (case-insensitive).
     * @return      Value string_view, or empty view if the header is absent.
     */
    std::string_view header(std::string_view name) const noexcept;

private:
    int status_code_{};
    std::string status_message_{};
    std::unordered_map<std::string, std::string> headers_{};
    std::vector<char> body_{};
};

} // namespace zd::crq