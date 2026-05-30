/**
 * @file client.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Public API implementation
 * @version 1.0.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#include "client.hpp"
#include "platform/winsock.hpp"

namespace zd::crq {

class HttpClient::Impl {
public:
  Impl() noexcept = default;

  std::expected<void, Error> connect(std::string_view host,
                                     uint16_t port) noexcept {
    return winsock_.connect(host, port);
  }

  std::expected<size_t, Error> send(std::string_view request) noexcept {
    return winsock_.send(request);
  }

  std::expected<std::vector<char>, Error> receive() noexcept {
    return winsock_.receive();
  }

  std::expected<std::vector<char>, Error> receive_all() noexcept {
    return winsock_.receive_all();
  }

  void close() noexcept { winsock_.close(); }

private:
  platform::WinSock winsock_{};
};

HttpClient::HttpClient() noexcept : impl_(new Impl()) {}

HttpClient::~HttpClient() noexcept { delete impl_; }

std::expected<void, Error> HttpClient::connect(std::string_view host,
                                               uint16_t port) noexcept {
  return impl_->connect(host, port);
}

std::expected<size_t, Error>
HttpClient::send(std::string_view request) noexcept {
  return impl_->send(request);
}

std::expected<std::vector<char>, Error> HttpClient::receive() noexcept {
  return impl_->receive();
}

std::expected<std::vector<char>, Error> HttpClient::receive_all() noexcept {
  return impl_->receive_all();
}

void HttpClient::close() noexcept { impl_->close(); }

} // namespace zd::crq
