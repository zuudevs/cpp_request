#include <gtest/gtest.h>

#include <cpp_request/url.hpp>

using cpp_request::ErrorCode;
using cpp_request::Url;

TEST(UrlTest, ParsesBasicHttpUrl) {
    auto parsed = Url::parse("http://example.com/path/to?q=1");

    ASSERT_TRUE(parsed);
    const auto& url = parsed.value();
    EXPECT_EQ(url.scheme(), "http");
    EXPECT_EQ(url.host(), "example.com");
    EXPECT_EQ(url.port(), 80);
    EXPECT_FALSE(url.has_explicit_port());
    EXPECT_EQ(url.path(), "/path/to");
    EXPECT_EQ(url.query(), "q=1");
    EXPECT_EQ(url.target(), "/path/to?q=1");
}

TEST(UrlTest, UsesSlashAsDefaultPath) {
    auto parsed = Url::parse("http://example.com");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed.value().path(), "/");
    EXPECT_EQ(parsed.value().target(), "/");
}

TEST(UrlTest, AddsSlashBeforeQueryWhenPathIsMissing) {
    auto parsed = Url::parse("http://example.com?x=1");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed.value().path(), "/");
    EXPECT_EQ(parsed.value().query(), "x=1");
    EXPECT_EQ(parsed.value().target(), "/?x=1");
}

TEST(UrlTest, ParsesExplicitPort) {
    auto parsed = Url::parse("http://example.com:8080/api");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed.value().host(), "example.com");
    EXPECT_EQ(parsed.value().port(), 8080);
    EXPECT_TRUE(parsed.value().has_explicit_port());
}

TEST(UrlTest, ParsesBracketedIpv6Literal) {
    auto parsed = Url::parse("http://[2001:db8::1]:8080/a");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed.value().host(), "2001:db8::1");
    EXPECT_EQ(parsed.value().port(), 8080);
    EXPECT_TRUE(parsed.value().has_explicit_port());
    EXPECT_TRUE(parsed.value().host_is_ipv6_literal());
    EXPECT_EQ(parsed.value().target(), "/a");
}

TEST(UrlTest, ParsesIpv6LiteralWithEmbeddedIpv4) {
    auto parsed = Url::parse("http://[::ffff:192.0.2.128]/a");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed.value().host(), "::ffff:192.0.2.128");
    EXPECT_TRUE(parsed.value().host_is_ipv6_literal());
}

TEST(UrlTest, PreservesValidPercentEscapes) {
    auto parsed = Url::parse("http://example.com/a%20b?q=x%2Fy");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed.value().path(), "/a%20b");
    EXPECT_EQ(parsed.value().query(), "q=x%2Fy");
    EXPECT_EQ(parsed.value().target(), "/a%20b?q=x%2Fy");
}

TEST(UrlTest, FragmentIsNotPartOfRequestTarget) {
    auto parsed = Url::parse("http://example.com/a?x=1#section");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed.value().target(), "/a?x=1");
    EXPECT_EQ(parsed.value().query(), "x=1");
}

TEST(UrlTest, AcceptsSchemeCaseInsensitivelyAndNormalizesIt) {
    auto parsed = Url::parse("HTTP://example.com/");

    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed.value().scheme(), "http");
}

TEST(UrlTest, RejectsHttpsAsUnsupportedScheme) {
    auto parsed = Url::parse("https://example.com/");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::UnsupportedScheme);
}

TEST(UrlTest, RejectsMissingSchemeSeparator) {
    auto parsed = Url::parse("example.com/path");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidUrl);
}

TEST(UrlTest, RejectsMissingHost) {
    auto parsed = Url::parse("http:///path");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidUrl);
}

TEST(UrlTest, RejectsUserInfoInV1) {
    auto parsed = Url::parse("http://user@example.com/");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidUrl);
}

TEST(UrlTest, RejectsInvalidPort) {
    auto parsed = Url::parse("http://example.com:abc/");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidPort);
}

TEST(UrlTest, RejectsOutOfRangePort) {
    auto parsed = Url::parse("http://example.com:65536/");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidPort);
}

TEST(UrlTest, RejectsUnbracketedIpv6Literal) {
    auto parsed = Url::parse("http://2001:db8::1/");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidUrl);
}

TEST(UrlTest, RejectsBracketedNonIpv6Host) {
    auto parsed = Url::parse("http://[not-an-ipv6]/");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidUrl);
}

TEST(UrlTest, RejectsMalformedBracketedIpv6Literal) {
    auto parsed = Url::parse("http://[2001:::1]/");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidUrl);
}

TEST(UrlTest, RejectsIpv6ZoneIdentifierInV1) {
    auto parsed = Url::parse("http://[fe80::1%25eth0]/");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidUrl);
}

TEST(UrlTest, RejectsTruncatedPercentEscapeInPath) {
    auto parsed = Url::parse("http://example.com/a%2");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidUrl);
}

TEST(UrlTest, RejectsNonHexPercentEscapeInPath) {
    auto parsed = Url::parse("http://example.com/a%XZ");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidUrl);
}

TEST(UrlTest, RejectsMalformedPercentEscapeInQuery) {
    auto parsed = Url::parse("http://example.com/a?q=%GG");

    ASSERT_FALSE(parsed);
    EXPECT_EQ(parsed.error().code, ErrorCode::InvalidUrl);
}
