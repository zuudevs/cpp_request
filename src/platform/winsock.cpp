/**
 * @file winsock.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Windows socket implementation
 * @version 0.1.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#include "winsock.hpp"
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "platform/winsock.hpp"

namespace zd::crq::platform {

std::expected<void, zd::crq::Error> WinSock::connect(std::string_view host,
                                                     uint16_t port) noexcept {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  addrinfo *info = nullptr;
  auto service = std::to_string(port);

  auto res = ::getaddrinfo(host.data(), service.c_str(), &hints, &info);

  if (res != 0) {
    return std::unexpected{zd::crq::to_error(res)};
  }

  if (!socket_.create(info->ai_family, info->ai_socktype, info->ai_protocol)) {
    ::freeaddrinfo(info);
    return std::unexpected{zd::crq::Error::InvalidSocket};
  }

  bool conn_res = socket_.connect(reinterpret_cast<const char *>(info->ai_addr),
                                  static_cast<int>(info->ai_addrlen));

  ::freeaddrinfo(info);

  if (!conn_res) {
    return std::unexpected{zd::crq::to_error(::WSAGetLastError())};
  }

  return {};
}

std::expected<size_t, zd::crq::Error>
WinSock::send(std::string_view req) noexcept {
  if (!socket_.valid()) {
    return std::unexpected{zd::crq::Error::InvalidSocket};
  }

  auto res = socket_.send(req.data(), static_cast<int>(req.size()));

  if (res == SOCKET_ERROR) {
    return std::unexpected{zd::crq::to_error(::WSAGetLastError())};
  }

  return static_cast<size_t>(res);
}

std::expected<std::vector<char>, zd::crq::Error> WinSock::receive() noexcept {
  if (!socket_.valid()) {
    return std::unexpected{zd::crq::Error::InvalidSocket};
  }

  std::vector<char> data(8192);
  auto bytes = socket_.receive(data.data(), static_cast<int>(data.size()));

  if (bytes == SOCKET_ERROR) {
    return std::unexpected{zd::crq::to_error(::WSAGetLastError())};
  }

  data.resize(static_cast<size_t>(bytes));
  return data;
}

std::expected<std::vector<char>, zd::crq::Error>
WinSock::receive_all() noexcept {
  if (!socket_.valid()) {
    return std::unexpected{zd::crq::Error::InvalidSocket};
  }

  std::vector<char> result;
  std::vector<char> buffer(8192);

  while (true) {
    auto bytes =
        socket_.receive(buffer.data(), static_cast<int>(buffer.size()));

    if (bytes == SOCKET_ERROR) {
      return std::unexpected{zd::crq::to_error(::WSAGetLastError())};
    }

    if (bytes == 0) {
      break;
    }

    result.insert(result.end(), buffer.begin(), buffer.begin() + bytes);
  }

  return result;
}

void WinSock::close() noexcept { socket_.close(); }

} // namespace zd::crq::platform
