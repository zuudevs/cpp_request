/**
 * @file request.h
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Public HTTP client interface
 * @version 0.2.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "error.h"
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace zd::crq {

enum class Method : uint8_t {
    Get,
    Post,
    Put,
    Patch,
    Delete,
    Head,
    Options,
};

// ---------------------------------------------------------------------------

/**
 * @class Request
 * @brief Fluent builder for HTTP/1.1 requests.
 *
 * Usage:
 * @code
 *   auto req = Request{}
 *       .method(Method::Post)
 *       .url("http://example.com/api/data")
 *       .header("Content-Type", "application/json")
 *       .body(R"({"key":"value"})");
 *
 *   auto resp = client.send(req);
 * @endcode
 */
class Request {
public:
    Request() noexcept = default;

    /// HTTP method (default: GET).
    Request& method(Method m) noexcept;

    /// Full URL — scheme, host, optional port, path, and query string.
    /// Example: "http://api.example.com:8080/v1/items?page=1"
    Request& url(std::string_view url) noexcept;

    /// Append a request header. Calling twice with the same key overwrites.
    Request& header(std::string_view key, std::string_view value) noexcept;

    /// Set the request body from a string view.
    Request& body(std::string_view data) noexcept;

    /// Set the request body from a byte buffer (move).
    Request& body(std::vector<char> data) noexcept;

    // ------------------------------------------------------------------
    // Parsed URL components (available after url() is called)
    // ------------------------------------------------------------------
    std::string_view scheme() const noexcept { return scheme_; }
    std::string_view host()   const noexcept { return host_;   }
    std::string_view path()   const noexcept { return path_;   }
    uint16_t         port()   const noexcept { return port_;   }

    /**
     * @brief Serialise the request into a raw HTTP/1.1 wire-format string.
     *
     * Automatically adds:
     *  - Host header
     *  - Content-Length (when body is non-empty)
     *  - Connection: close (unless overridden by the caller)
     *
     * @return Raw request bytes, or Error::InvalidArgument if no host was set.
     */
    std::expected<std::string, Error> build() const noexcept;

private:
    Method method_{Method::Get};
    std::string scheme_{"http"};
    std::string host_{};
    std::string path_{"/"};
    uint16_t    port_{80};
    std::unordered_map<std::string, std::string> headers_{};
    std::vector<char> body_{};

    void parse_url(std::string_view url) noexcept;
    std::string_view method_str() const noexcept;
};

} // namespace zd::crq