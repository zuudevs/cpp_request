#include <gtest/gtest.h>

#include "http/request_serializer.hpp"

#include <cpp_request/error.hpp>
#include <cpp_request/request.hpp>

#include <string>

namespace {

using cpp_request::ErrorCode;
using cpp_request::Method;
using cpp_request::Request;
using cpp_request::detail::http::serialize_request;

TEST(RequestSerializerTest, SerializesBasicGetRequest) {
    Request request{Method::Get, "http://example.com/items?limit=10"};

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_EQ(
        result.value().head,
        "GET /items?limit=10 HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n");
    EXPECT_TRUE(result.value().body.empty());
}

TEST(RequestSerializerTest, SerializesAllSupportedMethods) {
    struct Case {
        Method method;
        const char* token;
    };
    const Case cases[] = {
        {Method::Get, "GET"},
        {Method::Head, "HEAD"},
        {Method::Post, "POST"},
        {Method::Put, "PUT"},
        {Method::Patch, "PATCH"},
        {Method::Delete, "DELETE"},
    };

    for (const auto& test_case : cases) {
        Request request{test_case.method, "http://example.com/"};
        auto result = serialize_request(request);
        ASSERT_TRUE(result);
        EXPECT_EQ(
            result.value().head.substr(0, std::string{test_case.token}.size()),
            test_case.token);
    }
}

TEST(RequestSerializerTest, PreservesCustomHeadersAndDuplicates) {
    Request request{Method::Get, "http://example.com/"};
    request.headers().add("X-Test", "one");
    request.headers().add("X-Test", "two");

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(result.value().head.find("X-Test: one\r\n"), std::string::npos);
    EXPECT_NE(result.value().head.find("X-Test: two\r\n"), std::string::npos);
}

TEST(RequestSerializerTest, UsesCallerProvidedHostWithoutGeneratingAnother) {
    Request request{Method::Get, "http://example.com/"};
    request.headers().add("Host", "virtual.example");

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(result.value().head.find("Host: virtual.example\r\n"), std::string::npos);
    EXPECT_EQ(result.value().head.find("Host: example.com\r\n"), std::string::npos);
}

TEST(RequestSerializerTest, GeneratedHostIncludesExplicitPort) {
    Request request{Method::Get, "http://example.com:8080/"};

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(result.value().head.find("Host: example.com:8080\r\n"), std::string::npos);
}

TEST(RequestSerializerTest, GeneratedHostFormatsIpv6Literal) {
    Request request{Method::Get, "http://[::1]:8080/health"};

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(result.value().head.find("Host: [::1]:8080\r\n"), std::string::npos);
}

TEST(RequestSerializerTest, AddsContentLengthForNonEmptyBodyWithoutCopyingBody) {
    std::string body = "hello";
    Request request{Method::Post, "http://example.com/upload"};
    request.set_body(body);

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(result.value().head.find("Content-Length: 5\r\n"), std::string::npos);
    EXPECT_EQ(result.value().body, body);
    EXPECT_EQ(result.value().body.data(), body.data());
}

TEST(RequestSerializerTest, AcceptsMatchingCallerContentLength) {
    Request request{Method::Post, "http://example.com/upload"};
    request.set_body("abc");
    request.headers().add("Content-Length", "3");

    auto result = serialize_request(request);

    ASSERT_TRUE(result);
    EXPECT_NE(result.value().head.find("Content-Length: 3\r\n"), std::string::npos);
}

TEST(RequestSerializerTest, RejectsMismatchedContentLength) {
    Request request{Method::Post, "http://example.com/upload"};
    request.set_body("abc");
    request.headers().add("Content-Length", "2");

    auto result = serialize_request(request);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidContentLength);
}

TEST(RequestSerializerTest, RejectsDuplicateContentLength) {
    Request request{Method::Post, "http://example.com/upload"};
    request.set_body("abc");
    request.headers().add("Content-Length", "3");
    request.headers().add("content-length", "3");

    auto result = serialize_request(request);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidContentLength);
}

TEST(RequestSerializerTest, RejectsDuplicateHost) {
    Request request{Method::Get, "http://example.com/"};
    request.headers().add("Host", "example.com");
    request.headers().add("host", "example.com");

    auto result = serialize_request(request);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidHeader);
}

TEST(RequestSerializerTest, RejectsHeaderNameWithInvalidTokenByte) {
    Request request{Method::Get, "http://example.com/"};
    request.headers().add("Bad Header", "value");

    auto result = serialize_request(request);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidHeader);
}

TEST(RequestSerializerTest, RejectsHeaderValueWithCrLfInjection) {
    Request request{Method::Get, "http://example.com/"};
    request.headers().add("X-Test", "ok\r\nInjected: yes");

    auto result = serialize_request(request);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidHeader);
}

TEST(RequestSerializerTest, RejectsTransferEncodingInV1Requests) {
    Request request{Method::Post, "http://example.com/"};
    request.set_body("abc");
    request.headers().add("Transfer-Encoding", "chunked");

    auto result = serialize_request(request);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ConflictingMessageFraming);
}

TEST(RequestSerializerTest, PropagatesUrlParseFailure) {
    Request request{Method::Get, "https://example.com/"};

    auto result = serialize_request(request);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::UnsupportedScheme);
}

} // namespace
