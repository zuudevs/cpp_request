#include "platform/network_runtime.hpp"

#ifdef _WIN32
#include <winsock2.h>
#endif

namespace cpp_request::detail::platform {

#ifdef _WIN32
namespace {

class WinsockRuntime final {
public:
    WinsockRuntime() noexcept {
        WSADATA data{};
        status_ = ::WSAStartup(MAKEWORD(2, 2), &data);
        if (status_ != 0) {
            return;
        }

        if (LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 2) {
            status_ = WSAVERNOTSUPPORTED;
            ::WSACleanup();
            return;
        }

        initialized_ = true;
    }

    ~WinsockRuntime() noexcept {
        if (initialized_) {
            ::WSACleanup();
        }
    }

    WinsockRuntime(const WinsockRuntime&) = delete;
    WinsockRuntime& operator=(const WinsockRuntime&) = delete;

    [[nodiscard]] NetworkRuntimeStatus status() const noexcept {
        return {initialized_, initialized_ ? 0 : status_};
    }

private:
    int status_{0};
    bool initialized_{false};
};

} // namespace
#endif

NetworkRuntimeStatus ensure_network_runtime() noexcept {
#ifdef _WIN32
    static const WinsockRuntime runtime;
    return runtime.status();
#else
    return {};
#endif
}

} // namespace cpp_request::detail::platform
