#pragma once

#include <cassert>
#include <type_traits>
#include <utility>
#include <variant>

#include <cpp_request/error.hpp>

namespace cpp_request {

template <typename T>
class [[nodiscard]] Result {
public:
    Result(T value)
        : storage_(std::move(value)) {}

    Result(Error error)
        : storage_(error) {}

    [[nodiscard]] bool has_value() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    T& value() & noexcept {
        assert(has_value());
        return std::get<T>(storage_);
    }

    const T& value() const& noexcept {
        assert(has_value());
        return std::get<T>(storage_);
    }

    T&& value() && noexcept {
        assert(has_value());
        return std::get<T>(std::move(storage_));
    }

    Error& error() & noexcept {
        assert(!has_value());
        return std::get<Error>(storage_);
    }

    const Error& error() const& noexcept {
        assert(!has_value());
        return std::get<Error>(storage_);
    }

private:
    std::variant<T, Error> storage_;
};

} // namespace cpp_request
