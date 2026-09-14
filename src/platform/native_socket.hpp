#pragma once

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <utility>

namespace cpp_request::detail::platform {

#ifdef _WIN32
using NativeSocketHandle = SOCKET;
inline constexpr NativeSocketHandle kInvalidSocket = INVALID_SOCKET;
#else
using NativeSocketHandle = int;
inline constexpr NativeSocketHandle kInvalidSocket = -1;
#endif

class NativeSocket final {
public:
    using handle_type = NativeSocketHandle;

    constexpr NativeSocket() noexcept = default;
    explicit constexpr NativeSocket(handle_type handle) noexcept
        : handle_(handle) {}

    ~NativeSocket() noexcept;

    NativeSocket(const NativeSocket&) = delete;
    NativeSocket& operator=(const NativeSocket&) = delete;

    NativeSocket(NativeSocket&& other) noexcept
        : handle_(other.release()) {}

    NativeSocket& operator=(NativeSocket&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    [[nodiscard]] constexpr bool valid() const noexcept {
        return handle_ != kInvalidSocket;
    }

    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return valid();
    }

    [[nodiscard]] constexpr handle_type get() const noexcept {
        return handle_;
    }

    [[nodiscard]] handle_type release() noexcept {
        return std::exchange(handle_, kInvalidSocket);
    }

    void reset(handle_type replacement = kInvalidSocket) noexcept;
    void close() noexcept {
        reset();
    }

private:
    handle_type handle_{kInvalidSocket};
};

[[nodiscard]] int last_socket_error() noexcept;

} // namespace cpp_request::detail::platform
