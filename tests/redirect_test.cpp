#include <gtest/gtest.h>

#include "http/redirect.hpp"

#include <cpp_request/error.hpp>
#include <cpp_request/request.hpp>
#include <cpp_request/url.hpp>

#include <string>

namespace {

using cpp_request::ErrorCode;
using cpp_request::Method;
using cpp_request::Url;
using cpp_request::detail::http::redirect_behavior;
using cpp_request::detail::http::resolve_redirect_location;

TEST(RedirectTest, ResolvesAbsoluteHttpLocation) {
    auto base = Url::parse("http://example.com/a/b?x=1");
    ASSERT_TRUE(base);

    auto result = resolve_redirect_location(
        base.value(),
        "http://other.test:8080/next?q=2#fragment");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "http://other.test:8080/next?q=2");
}

TEST(RedirectTest, ResolvesSchemeRelativeLocation) {
    auto base = Url::parse("http://example.com/a/b");
    ASSERT_TRUE(base);

    auto result = resolve_redirect_location(base.value(), "//other.test/next");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "http://other.test/next");
}

TEST(RedirectTest, ResolvesAbsolutePathAndDotSegments) {
    auto base = Url::parse("http://example.com/a/b/c");
    ASSERT_TRUE(base);

    auto result = resolve_redirect_location(base.value(), "/x/./y/../z");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "http://example.com/x/z");
}

TEST(RedirectTest, ResolvesRelativePathAgainstCurrentDirectory) {
    auto base = Url::parse("http://example.com/a/b/c?old=1");
    ASSERT_TRUE(base);

    auto result = resolve_redirect_location(base.value(), "../next?q=2");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "http://example.com/a/next?q=2");
}

TEST(RedirectTest, ResolvesQueryOnlyLocation) {
    auto base = Url::parse("http://example.com/a/b?old=1");
    ASSERT_TRUE(base);

    auto result = resolve_redirect_location(base.value(), "?new=2");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "http://example.com/a/b?new=2");
}

TEST(RedirectTest, FragmentIsNeverSentToServer) {
    auto base = Url::parse("http://example.com/a/b?x=1");
    ASSERT_TRUE(base);

    auto result = resolve_redirect_location(base.value(), "#section");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "http://example.com/a/b?x=1");
}

TEST(RedirectTest, PreservesExplicitPortAndIpv6Authority) {
    auto base = Url::parse("http://[::1]:8080/a/b");
    ASSERT_TRUE(base);

    auto result = resolve_redirect_location(base.value(), "../next");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), "http://[::1]:8080/next");
}

TEST(RedirectTest, RejectsHttpsLocationExplicitly) {
    auto base = Url::parse("http://example.com/a");
    ASSERT_TRUE(base);

    auto result = resolve_redirect_location(base.value(), "https://example.com/b");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::UnsupportedRedirectScheme);
}

TEST(RedirectTest, EmptyLocationIsMissing) {
    auto base = Url::parse("http://example.com/a");
    ASSERT_TRUE(base);

    auto result = resolve_redirect_location(base.value(), "");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::MissingRedirectLocation);
}

TEST(RedirectTest, PostBecomesGetFor301And302) {
    const auto moved = redirect_behavior(301, Method::Post);
    EXPECT_EQ(moved.method, Method::Get);
    EXPECT_FALSE(moved.preserve_body);

    const auto found = redirect_behavior(302, Method::Post);
    EXPECT_EQ(found.method, Method::Get);
    EXPECT_FALSE(found.preserve_body);
}

TEST(RedirectTest, OtherMethodsRemainFor301And302) {
    const auto put = redirect_behavior(301, Method::Put);
    EXPECT_EQ(put.method, Method::Put);
    EXPECT_TRUE(put.preserve_body);

    const auto patch = redirect_behavior(302, Method::Patch);
    EXPECT_EQ(patch.method, Method::Patch);
    EXPECT_TRUE(patch.preserve_body);
}

TEST(RedirectTest, SeeOtherUsesGetExceptForHead) {
    const auto post = redirect_behavior(303, Method::Post);
    EXPECT_EQ(post.method, Method::Get);
    EXPECT_FALSE(post.preserve_body);

    const auto head = redirect_behavior(303, Method::Head);
    EXPECT_EQ(head.method, Method::Head);
    EXPECT_FALSE(head.preserve_body);
}

TEST(RedirectTest, TemporaryAndPermanentRedirectPreserveMethodAndBody) {
    const auto temporary = redirect_behavior(307, Method::Post);
    EXPECT_EQ(temporary.method, Method::Post);
    EXPECT_TRUE(temporary.preserve_body);

    const auto permanent = redirect_behavior(308, Method::Patch);
    EXPECT_EQ(permanent.method, Method::Patch);
    EXPECT_TRUE(permanent.preserve_body);
}

} // namespace
