#include "net/resolver.hpp"

#include "platform/network_runtime.hpp"

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#endif

#include <array>
#include <charconv>
#include <memory>
#include <string>
#include <system_error>

namespace cpp_request::detail::net {
namespace {

struct AddrInfoDeleter {
    void operator()(addrinfo* value) const noexcept {
        if (value != nullptr) {
            ::freeaddrinfo(value);
        }
    }
};

using AddrInfoPtr = std::unique_ptr<addrinfo, AddrInfoDeleter>;

} // namespace

Result<std::vector<Endpoint>> Resolver::resolve(
    std::string_view host,
    std::uint16_t port) {
    if (host.empty() || host.find('\0') != std::string_view::npos) {
        return Error{ErrorCode::ResolveFailed};
    }

    const auto runtime = platform::ensure_network_runtime();
    if (!runtime.ok) {
        return Error{ErrorCode::ResolveFailed, runtime.native_code};
    }

    std::string host_storage{host};

    std::array<char, 6> service{};
    const auto converted = std::to_chars(
        service.data(),
        service.data() + service.size() - 1,
        port);
    if (converted.ec != std::errc{}) {
        return Error{ErrorCode::ResolveFailed};
    }
    *converted.ptr = '\0';

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_NUMERICSERV;

    addrinfo* raw_results = nullptr;
    const int status = ::getaddrinfo(
        host_storage.c_str(),
        service.data(),
        &hints,
        &raw_results);

    if (status != 0) {
        return Error{ErrorCode::ResolveFailed, status};
    }

    AddrInfoPtr results{raw_results};
    std::vector<Endpoint> endpoints;

    for (const addrinfo* current = results.get(); current != nullptr; current = current->ai_next) {
        if (current->ai_addr == nullptr) {
            continue;
        }

        if (current->ai_family != AF_INET && current->ai_family != AF_INET6) {
            continue;
        }

        const auto address_size = static_cast<std::size_t>(current->ai_addrlen);
        if (address_size == 0 || address_size > sizeof(sockaddr_storage)) {
            continue;
        }

        const int socket_type = current->ai_socktype != 0 ? current->ai_socktype : SOCK_STREAM;
        const int protocol = current->ai_protocol != 0 ? current->ai_protocol : IPPROTO_TCP;

        endpoints.emplace_back(
            current->ai_addr,
            address_size,
            socket_type,
            protocol);
    }

    if (endpoints.empty()) {
        return Error{ErrorCode::ResolveFailed};
    }

    return endpoints;
}

} // namespace cpp_request::detail::net
