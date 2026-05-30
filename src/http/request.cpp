/**
 * @file request.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief HTTP request implementation
 * @version 0.2.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#include "cpp_request/request.h"
#include <cctype>
#include <sstream>

namespace zd::crq {

Request& Request::method(Method m) noexcept {
    method_ = m;
    return *this;
}

Request& Request::url(std::string_view url) noexcept {
    parse_url(url);
    return *this;
}

Request& Request::header(std::string_view key, std::string_view value) noexcept {
    headers_[std::string(key)] = std::string(value);
    return *this;
}

Request& Request::body(std::string_view data) noexcept {
    body_.clear();
    body_.insert(body_.end(), data.begin(), data.end());
    return *this;
}

Request& Request::body(std::vector<char> data) noexcept {
    body_ = std::move(data);
    return *this;
}

std::expected<std::string, Error> Request::build() const noexcept {
    if (host_.empty()) {
        return std::unexpected(Error::InvalidArgument);
    }

    std::ostringstream oss;

    // Request line
    oss << method_str() << " " << (path_.empty() ? "/" : path_);
    if (!path_.empty() && path_.find('?') != std::string::npos) {
        // Path already contains query string
    }
    oss << " HTTP/1.1\r\n";

    // Host header (always required)
    oss << "Host: " << host_;
    if ((scheme_ == "http" && port_ != 80) ||
        (scheme_ == "https" && port_ != 443)) {
        oss << ":" << port_;
    }
    oss << "\r\n";

    // Content-Length header (if body present)
    if (!body_.empty()) {
        oss << "Content-Length: " << body_.size() << "\r\n";
    }

    // Add user headers
    for (const auto& [key, value] : headers_) {
        oss << key << ": " << value << "\r\n";
    }

    // Add Connection: close if not already specified
    bool has_connection = false;
    for (const auto& [key, value] : headers_) {
        if (key == "Connection" || key == "connection") {
            has_connection = true;
            break;
        }
    }
    if (!has_connection) {
        oss << "Connection: close\r\n";
    }

    oss << "\r\n";

    std::string result = oss.str();

    // Append body
    if (!body_.empty()) {
        result.insert(result.end(), body_.begin(), body_.end());
    }

    return result;
}

void Request::parse_url(std::string_view url) noexcept {
    size_t pos = 0;

    // Parse scheme
    size_t scheme_end = url.find("://");
    if (scheme_end != std::string_view::npos) {
        scheme_ = std::string(url.substr(0, scheme_end));
        pos = scheme_end + 3;
    } else {
        scheme_ = "http";
        pos = 0;
    }

    // Parse host:port/path
    size_t host_end = url.find_first_of(":/", pos);
    if (host_end == std::string_view::npos) {
        host_end = url.length();
    }

    host_ = std::string(url.substr(pos, host_end - pos));

    pos = host_end;

    // Parse port (optional)
    if (pos < url.length() && url[pos] == ':') {
        pos++;
        size_t port_end = url.find('/', pos);
        if (port_end == std::string_view::npos) {
            port_end = url.length();
        }
        std::string port_str(url.substr(pos, port_end - pos));
        port_ = std::stoi(port_str);
        pos = port_end;
    } else {
        // Default port based on scheme
        if (scheme_ == "https") {
            port_ = 443;
        } else {
            port_ = 80;
        }
    }

    // Parse path and query
    if (pos < url.length() && url[pos] == '/') {
        path_ = std::string(url.substr(pos));
    } else {
        path_ = "/";
    }
}

std::string_view Request::method_str() const noexcept {
    switch (method_) {
        case Method::Get:
            return "GET";
        case Method::Post:
            return "POST";
        case Method::Put:
            return "PUT";
        case Method::Patch:
            return "PATCH";
        case Method::Delete:
            return "DELETE";
        case Method::Head:
            return "HEAD";
        case Method::Options:
            return "OPTIONS";
        default:
            return "GET";
    }
}

} // namespace zd::crq
