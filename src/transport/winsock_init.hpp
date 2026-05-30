/**
 * @file winsock_init.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Windows socket initialization (internal)
 * @version 0.1.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "cpp_request/error.h"
#include <expected>

#pragma comment(lib, "ws2_32.lib")

namespace zd::crq::platform {

class WinSockInit {
public:
  WinSockInit() noexcept = default;
  std::expected<void, zd::crq::Error> init() noexcept;
  ~WinSockInit() noexcept;

private:
  struct WSAData *data_ = nullptr;
};

} // namespace zd::crq::platform
