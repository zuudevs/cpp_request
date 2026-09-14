#pragma once

#include "platform/native_socket.hpp"

namespace cpp_request::detail::platform {

[[nodiscard]] bool set_socket_nonblocking(
    NativeSocketHandle socket,
    bool enabled,
    int& native_code) noexcept;

} // namespace cpp_request::detail::platform
