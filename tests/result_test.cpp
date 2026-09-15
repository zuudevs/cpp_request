#include <gtest/gtest.h>

#include <cpp_request/error.hpp>
#include <cpp_request/result.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace {

using cpp_request::Error;
using cpp_request::ErrorCode;
using cpp_request::Result;

static_assert(std::is_copy_constructible_v<Result<int>>);
static_assert(std::is_move_constructible_v<Result<int>>);
static_assert(!std::is_copy_constructible_v<Result<std::unique_ptr<int>>>);
static_assert(std::is_move_constructible_v<Result<std::unique_ptr<int>>>);

TEST(ResultTest, StoresSuccessValue) {
    Result<int> result{42};

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(result.value(), 42);
}

TEST(ResultTest, StoresFailureError) {
    Result<int> result{Error{ErrorCode::ConnectFailed, 17}};

    ASSERT_FALSE(result.has_value());
    ASSERT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.error().code, ErrorCode::ConnectFailed);
    EXPECT_EQ(result.error().native_code, 17);
}

TEST(ResultTest, SupportsConvertibleSuccessInput) {
    Result<std::string> result{"hello"};

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "hello");
}

TEST(ResultTest, SupportsMoveOnlyPayload) {
    Result<std::unique_ptr<int>> result{std::make_unique<int>(7)};

    ASSERT_TRUE(result);
    auto value = std::move(result).value();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 7);
}

TEST(ResultTest, ProvidesMutableAndConstAccess) {
    Result<std::string> result{std::string{"before"}};
    result.value() = "after";

    const Result<std::string>& const_result = result;
    EXPECT_EQ(const_result.value(), "after");
}

TEST(ResultTest, ExplicitFactoriesWorkForNormalPayloads) {
    auto success = Result<int>::success(11);
    auto failure = Result<int>::failure(Error{ErrorCode::ReadTimeout});

    ASSERT_TRUE(success);
    EXPECT_EQ(success.value(), 11);
    ASSERT_FALSE(failure);
    EXPECT_EQ(failure.error().code, ErrorCode::ReadTimeout);
}

TEST(ResultTest, ResultOfErrorCanRepresentSuccessAndFailureDistinctly) {
    auto success = Result<Error>::success(Error{ErrorCode::InvalidUrl, 3});
    auto failure = Result<Error>::failure(Error{ErrorCode::ResolveFailed, 9});

    ASSERT_TRUE(success);
    EXPECT_EQ(success.value().code, ErrorCode::InvalidUrl);
    EXPECT_EQ(success.value().native_code, 3);

    ASSERT_FALSE(failure);
    EXPECT_EQ(failure.error().code, ErrorCode::ResolveFailed);
    EXPECT_EQ(failure.error().native_code, 9);
}

TEST(ResultTest, DirectErrorConstructionRemainsFailureForResultOfError) {
    Result<Error> result{Error{ErrorCode::Unknown, 99}};

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::Unknown);
    EXPECT_EQ(result.error().native_code, 99);
}

} // namespace
