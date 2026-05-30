/**
 * @file client.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Internal client header
 * @version 0.1.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <vector>

#include "cpp_request/error.h"

namespace zd::crq {

class Request;
class Response;

/**
 * @class HttpClient
 * @brief Public API for making HTTP requests
 */
class HttpClient {
public:
  HttpClient() noexcept;
  ~HttpClient() noexcept;

  /// High-level convenience methods
  std::expected<Response, Error> get(std::string_view url) noexcept;
  std::expected<Response, Error> post(std::string_view url,
                                      std::string_view body = {}) noexcept;
  std::expected<Response, Error> put(std::string_view url,
                                     std::string_view body = {}) noexcept;
  std::expected<Response, Error> del(std::string_view url) noexcept;
  std::expected<Response, Error> send(const Request& req) noexcept;

  /// Low-level methods
  std::expected<void, Error> connect(std::string_view host,
                                     uint16_t port) noexcept;
  std::expected<size_t, Error> send_raw(std::string_view request) noexcept;
  std::expected<std::vector<char>, Error> receive() noexcept;
  std::expected<std::vector<char>, Error> receive_all() noexcept;
  void close() noexcept;

private:
  class Impl;
  Impl *impl_;
};

} // namespace zd::crq
