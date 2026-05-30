/**
 * @file winsock.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Windows socket implementation (internal)
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

#include "core/socket.hpp"
#include "cpp_request/error.h"

namespace zd::crq::platform {

class WinSock {
public:
  WinSock() noexcept = default;

  std::expected<void, zd::crq::Error> connect(std::string_view host,
                                              uint16_t port) noexcept;

  std::expected<size_t, zd::crq::Error> send(std::string_view req) noexcept;

  std::expected<std::vector<char>, zd::crq::Error> receive() noexcept;

  std::expected<std::vector<char>, zd::crq::Error> receive_all() noexcept;

  void close() noexcept;

private:
  zd::crq::core::Socket socket_{};
};

} // namespace zd::crq::platform
