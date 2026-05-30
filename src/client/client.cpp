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
#include "transport/winsock_init.hpp"
#include "cpp_request/request.h"
#include "cpp_request/response.h"

namespace zd::crq {

class HttpClient::Impl {
public:
  Impl() noexcept {
    auto result = wsa_init_.init(); // Initialize WinSock
    // Ignore init errors for now - library will fail on first connect if needed
    (void)result;
  }

  std::expected<void, Error> connect(std::string_view host,
                                     uint16_t port) noexcept {
    return winsock_.connect(host, port);
  }

  std::expected<size_t, Error> send_raw(std::string_view request) noexcept {
    return winsock_.send(request);
  }

  std::expected<std::vector<char>, Error> receive() noexcept {
    return winsock_.receive();
  }

  std::expected<std::vector<char>, Error> receive_all() noexcept {
    return winsock_.receive_all();
  }

  void close() noexcept { winsock_.close(); }

  std::expected<Response, Error> send_request(const Request& req) noexcept {
    // Build the raw request
    auto built = req.build();
    if (!built) {
      return std::unexpected(built.error());
    }

    // Connect to the host
    auto conn_result = winsock_.connect(req.host(), req.port());
    if (!conn_result) {
      return std::unexpected(conn_result.error());
    }

    // Send the request
    auto send_result = winsock_.send(built.value());
    if (!send_result) {
      winsock_.close();
      return std::unexpected(send_result.error());
    }

    // Receive the response
    auto recv_result = winsock_.receive_all();
    if (!recv_result) {
      winsock_.close();
      return std::unexpected(recv_result.error());
    }

    winsock_.close();

    // Parse the response
    auto parse_result = Response::parse(std::move(recv_result.value()));
    if (!parse_result) {
      return std::unexpected(parse_result.error());
    }

    return parse_result.value();
  }

private:
  platform::WinSockInit wsa_init_{};
  platform::WinSock winsock_{};
};

HttpClient::HttpClient() noexcept : impl_(new Impl()) {}

HttpClient::~HttpClient() noexcept { delete impl_; }

std::expected<void, Error> HttpClient::connect(std::string_view host,
                                               uint16_t port) noexcept {
  return impl_->connect(host, port);
}

std::expected<size_t, Error>
HttpClient::send_raw(std::string_view request) noexcept {
  return impl_->send_raw(request);
}

std::expected<std::vector<char>, Error> HttpClient::receive() noexcept {
  return impl_->receive();
}

std::expected<std::vector<char>, Error> HttpClient::receive_all() noexcept {
  return impl_->receive_all();
}

void HttpClient::close() noexcept { impl_->close(); }

std::expected<Response, Error> HttpClient::get(std::string_view url) noexcept {
  Request req;
  req.method(Method::Get).url(url);
  return impl_->send_request(req);
}

std::expected<Response, Error> HttpClient::post(
    std::string_view url, std::string_view body) noexcept {
  Request req;
  req.method(Method::Post).url(url);
  if (!body.empty()) {
    req.body(body);
  }
  return impl_->send_request(req);
}

std::expected<Response, Error> HttpClient::put(std::string_view url,
                                               std::string_view body) noexcept {
  Request req;
  req.method(Method::Put).url(url);
  if (!body.empty()) {
    req.body(body);
  }
  return impl_->send_request(req);
}

std::expected<Response, Error> HttpClient::del(std::string_view url) noexcept {
  Request req;
  req.method(Method::Delete).url(url);
  return impl_->send_request(req);
}

std::expected<Response, Error> HttpClient::send(
    const Request& req) noexcept {
  return impl_->send_request(req);
}

} // namespace zd::crq
