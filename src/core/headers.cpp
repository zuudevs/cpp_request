#include <cpp_request/headers.hpp>

#include <algorithm>
#include <iterator>
#include <utility>

namespace cpp_request {
namespace {

constexpr char ascii_lower(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch + ('a' - 'A'));
    }
    return ch;
}

bool ascii_iequals(std::string_view lhs, std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (ascii_lower(lhs[index]) != ascii_lower(rhs[index])) {
            return false;
        }
    }
    return true;
}

} // namespace

void Headers::add(std::string_view name, std::string_view value) {
    fields_.push_back(Field{std::string{name}, std::string{value}});
}

void Headers::set(std::string_view name, std::string_view value) {
    std::string owned_name{name};
    std::string owned_value{value};

    const auto first = std::find_if(
        fields_.begin(),
        fields_.end(),
        [&owned_name](const Field& field) {
            return ascii_iequals(field.name, owned_name);
        });

    if (first == fields_.end()) {
        fields_.push_back(Field{std::move(owned_name), std::move(owned_value)});
        return;
    }

    first->name = owned_name;
    first->value = std::move(owned_value);

    fields_.erase(
        std::remove_if(
            std::next(first),
            fields_.end(),
            [&owned_name](const Field& field) {
                return ascii_iequals(field.name, owned_name);
            }),
        fields_.end());
}

bool Headers::contains(std::string_view name) const noexcept {
    return std::any_of(
        fields_.begin(),
        fields_.end(),
        [name](const Field& field) {
            return ascii_iequals(field.name, name);
        });
}

std::string_view Headers::get(std::string_view name) const noexcept {
    const auto found = std::find_if(
        fields_.begin(),
        fields_.end(),
        [name](const Field& field) {
            return ascii_iequals(field.name, name);
        });

    if (found == fields_.end()) {
        return {};
    }
    return found->value;
}

} // namespace cpp_request
