/**
 * @file client.h
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-05-30
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <string_view>
#include <expected>

#include "error.h"
#include "response.h"

namespace zd::crq {

class Request;

/**
 * @class HttpClient
 * @brief High-level HTTP client.
 *
 * Internally manages WinSock initialisation (WSAStartup / WSACleanup).
 * Each send() call opens a fresh TCP connection, sends the request, reads
 * the full response, closes the socket, and returns a parsed Response.
 *
 * Not copyable (owns OS resources through Pimpl).
 *
 * @note HTTPS is not yet supported. Use port 80 / scheme "http".
 */
class HttpClient {
public:
    HttpClient() noexcept;
    ~HttpClient() noexcept;

    HttpClient(const HttpClient&)            = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    // ------------------------------------------------------------------
    // Convenience helpers
    // ------------------------------------------------------------------

    std::expected<Response, Error> get(std::string_view url) noexcept;

    std::expected<Response, Error> post(
		std::string_view url,
        std::string_view body = {}
	) noexcept;

    std::expected<Response, Error> put(std::string_view url,
                                       std::string_view body = {}) noexcept;

    std::expected<Response, Error> del(std::string_view url) noexcept;

    // ------------------------------------------------------------------
    // Full-control path
    // ------------------------------------------------------------------

    /// Send a fully configured RequestBuilder.
    std::expected<Response, Error> send(const Request& req) noexcept;

private:
    class Impl;
    Impl* impl_;
};

} // namespace zd::crq