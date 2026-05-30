/**
 * @file winsock_init.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Windows socket initialization implementation
 * @version 0.1.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#include "winsock_init.hpp"
#include <cstring>
#include <winsock2.h>

namespace zd::crq::platform {

std::expected<void, zd::crq::Error> WinSockInit::init() noexcept {
  WSAData wsa_data{};
  auto res = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);

  if (res != 0) {
    return std::unexpected{zd::crq::to_error(res)};
  }

  // Store for cleanup
  data_ = new WSAData(wsa_data);
  return {};
}

WinSockInit::~WinSockInit() noexcept {
  ::WSACleanup();
  delete data_;
  data_ = nullptr;
}

} // namespace zd::crq::platform
