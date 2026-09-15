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
    static_assert(!std::is_void_v<T>, "Result<void> is not implemented yet");

    template <
        typename U,
        std::enable_if_t<
            std::is_constructible_v<T, U&&>
                && !std::is_same_v<std::decay_t<U>, Error>
                && !std::is_same_v<T, Error>,
            int> = 0>
    Result(U&& value)
        : storage_(std::in_place_index<0>, std::forward<U>(value)) {}

    Result(Error error)
        : storage_(std::in_place_index<1>, error) {}

    [[nodiscard]] static Result success(T value) {
        return Result{ValueTag{}, std::move(value)};
    }

    [[nodiscard]] static Result failure(Error error) {
        return Result{ErrorTag{}, error};
    }

    [[nodiscard]] bool has_value() const noexcept {
        return storage_.index() == 0;
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    T& value() & noexcept {
        auto* value = std::get_if<0>(&storage_);
        assert(value != nullptr);
        return *value;
    }

    const T& value() const& noexcept {
        const auto* value = std::get_if<0>(&storage_);
        assert(value != nullptr);
        return *value;
    }

    T&& value() && noexcept {
        auto* value = std::get_if<0>(&storage_);
        assert(value != nullptr);
        return std::move(*value);
    }

    Error& error() & noexcept {
        auto* error = std::get_if<1>(&storage_);
        assert(error != nullptr);
        return *error;
    }

    const Error& error() const& noexcept {
        const auto* error = std::get_if<1>(&storage_);
        assert(error != nullptr);
        return *error;
    }

private:
    struct ValueTag final {};
    struct ErrorTag final {};

    Result(ValueTag, T value)
        : storage_(std::in_place_index<0>, std::move(value)) {}

    Result(ErrorTag, Error error)
        : storage_(std::in_place_index<1>, error) {}

    std::variant<T, Error> storage_;
};

} // namespace cpp_request
