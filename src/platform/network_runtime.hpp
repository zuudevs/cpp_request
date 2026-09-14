#pragma once

namespace cpp_request::detail::platform {

struct NetworkRuntimeStatus {
    bool ok{true};
    int native_code{0};
};

[[nodiscard]] NetworkRuntimeStatus ensure_network_runtime() noexcept;

} // namespace cpp_request::detail::platform
