#include <gtest/gtest.h>

#include "http/request_serializer.hpp"

#include <cpp_request/request.hpp>

#include <string>

namespace {

using cpp_request::Method;
using cpp_request::Request;
using cpp_request::detail::http::effective_request_url;
using cpp_request::detail::http::serialize_request;

TEST(QueryTest, PercentEncodesAppendedParameters) {
    Request request{Method::Get, "http://example.com/search"};
    request.add_query_param("q", "hello world");
    request.add_query_param("reserved", "a/b?c&d=e");
    request.add_query_param("safe", "AZaz09-._~");

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(
        result.value().head.find(
            "GET /search?q=hello%20world&reserved=a%2Fb%3Fc%26d%3De&safe=AZaz09-._~ HTTP/1.1\r\n"),
        std::string::npos);
}

TEST(QueryTest, AppendsAfterExistingRawQuery) {
    Request request{Method::Get, "http://example.com/search?lang=en"};
    request.add_query_param("q", "cpp http");

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(
        result.value().head.find(
            "GET /search?lang=en&q=cpp%20http HTTP/1.1\r\n"),
        std::string::npos);
}

TEST(QueryTest, HandlesExistingEmptyQueryMarker) {
    Request request{Method::Get, "http://example.com/search?"};
    request.add_query_param("q", "value");

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(
        result.value().head.find("GET /search?q=value HTTP/1.1\r\n"),
        std::string::npos);
}

TEST(QueryTest, PreservesRepeatedParametersInInsertionOrder) {
    Request request{Method::Get, "http://example.com/items"};
    request.add_query_param("tag", "cpp");
    request.add_query_param("tag", "http");

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(
        result.value().head.find("GET /items?tag=cpp&tag=http HTTP/1.1\r\n"),
        std::string::npos);
}

TEST(QueryTest, PercentEncodesUtf8Bytes) {
    Request request{Method::Get, "http://example.com/search"};
    const std::string value{"caf\xC3\xA9"};
    request.add_query_param("q", value);

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(
        result.value().head.find("GET /search?q=caf%C3%A9 HTTP/1.1\r\n"),
        std::string::npos);
}

TEST(QueryTest, EffectiveUrlIncludesParametersAndDropsFragment) {
    Request request{Method::Get, "http://example.com/search?raw=1#section"};
    request.add_query_param("q", "hello world");

    EXPECT_EQ(
        effective_request_url(request),
        "http://example.com/search?raw=1&q=hello%20world");
}

} // namespace
