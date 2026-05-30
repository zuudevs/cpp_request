/**
 * @file client.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
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

/**
 * @class HttpClient
 * @brief Public API for making HTTP requests
 *
 * This is the main interface users should interact with.
 * All internal implementation details are hidden.
 */
class HttpClient {
public:
  HttpClient() noexcept;
  ~HttpClient() noexcept;

  /// Connect to a remote host
  std::expected<void, zd::crq::Error> connect(std::string_view host,
                                              uint16_t port) noexcept;

  /// Send HTTP request
  std::expected<size_t, zd::crq::Error> send(std::string_view request) noexcept;

  /// Receive response (single read)
  std::expected<std::vector<char>, zd::crq::Error> receive() noexcept;

  /// Receive all response data
  std::expected<std::vector<char>, zd::crq::Error> receive_all() noexcept;

  /// Close connection
  void close() noexcept;

private:
  class Impl;
  Impl *impl_;
};

} // namespace zd::crq