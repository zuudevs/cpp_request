#include <gtest/gtest.h>

#include "http/response_parser.hpp"

#include <cpp_request/client.hpp>
#include <cpp_request/error.hpp>
#include <cpp_request/request.hpp>
#include <cpp_request/response_limits.hpp>

#include <utility>

namespace {

using cpp_request::Client;
using cpp_request::ErrorCode;
using cpp_request::Method;
using cpp_request::ResponseLimits;
using cpp_request::detail::http::ResponseParseProgress;
using cpp_request::detail::http::ResponseParser;

TEST(ResponseLimitsTest, DefaultsAreFiniteAndNonZero) {
    const ResponseLimits limits;

    EXPECT_GT(limits.max_head_bytes, 0u);
    EXPECT_GT(limits.max_body_bytes, 0u);
    EXPECT_GT(limits.max_chunk_line_bytes, 0u);
    EXPECT_GT(limits.max_trailer_bytes, 0u);
}

TEST(ResponseLimitsTest, ClientStoresConfiguredLimits) {
    Client client;
    ResponseLimits limits;
    limits.max_head_bytes = 1024;
    limits.max_body_bytes = 2048;
    limits.max_chunk_line_bytes = 128;
    limits.max_trailer_bytes = 512;

    client.set_response_limits(limits);

    EXPECT_EQ(client.response_limits().max_head_bytes, 1024u);
    EXPECT_EQ(client.response_limits().max_body_bytes, 2048u);
    EXPECT_EQ(client.response_limits().max_chunk_line_bytes, 128u);
    EXPECT_EQ(client.response_limits().max_trailer_bytes, 512u);
}

TEST(ResponseLimitsTest, ClientMovePreservesConfiguredLimits) {
    Client source;
    ResponseLimits limits;
    limits.max_head_bytes = 111;
    limits.max_body_bytes = 222;
    limits.max_chunk_line_bytes = 333;
    limits.max_trailer_bytes = 444;
    source.set_response_limits(limits);

    Client moved{std::move(source)};
    EXPECT_EQ(moved.response_limits().max_head_bytes, 111u);
    EXPECT_EQ(moved.response_limits().max_body_bytes, 222u);
    EXPECT_EQ(moved.response_limits().max_chunk_line_bytes, 333u);
    EXPECT_EQ(moved.response_limits().max_trailer_bytes, 444u);

    Client assigned;
    assigned = std::move(moved);
    EXPECT_EQ(assigned.response_limits().max_head_bytes, 111u);
    EXPECT_EQ(assigned.response_limits().max_body_bytes, 222u);
    EXPECT_EQ(assigned.response_limits().max_chunk_line_bytes, 333u);
    EXPECT_EQ(assigned.response_limits().max_trailer_bytes, 444u);
}

TEST(ResponseLimitsTest, RejectsOversizedResponseHeadBeforeTerminator) {
    ResponseLimits limits;
    limits.max_head_bytes = 24;
    ResponseParser parser{Method::Get, limits};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "X-Long: 1234567890");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ResponseLimitExceeded);
}

TEST(ResponseLimitsTest, RejectsDeclaredContentLengthAboveBodyLimit) {
    ResponseLimits limits;
    limits.max_body_bytes = 4;
    ResponseParser parser{Method::Get, limits};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ResponseLimitExceeded);
}

TEST(ResponseLimitsTest, AcceptsBodyExactlyAtConfiguredLimit) {
    ResponseLimits limits;
    limits.max_body_bytes = 5;
    ResponseParser parser{Method::Get, limits};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ResponseParseProgress::Complete);
    EXPECT_EQ(parser.response().body(), "hello");
}

TEST(ResponseLimitsTest, RejectsCloseDelimitedBodyAboveLimit) {
    ResponseLimits limits;
    limits.max_body_bytes = 4;
    ResponseParser parser{Method::Get, limits};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "\r\n"
        "hello");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ResponseLimitExceeded);
}

TEST(ResponseLimitsTest, RejectsChunkWhoseDeclaredSizeExceedsBodyBudget) {
    ResponseLimits limits;
    limits.max_body_bytes = 4;
    ResponseParser parser{Method::Get, limits};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ResponseLimitExceeded);
}

TEST(ResponseLimitsTest, RejectsOversizedChunkSizeLine) {
    ResponseLimits limits;
    limits.max_chunk_line_bytes = 3;
    ResponseParser parser{Method::Get, limits};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "1234");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ResponseLimitExceeded);
}

TEST(ResponseLimitsTest, RejectsOversizedTrailerSection) {
    ResponseLimits limits;
    limits.max_trailer_bytes = 8;
    ResponseParser parser{Method::Get, limits};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "0\r\n"
        "X: 123\r\n"
        "\r\n");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ResponseLimitExceeded);
}

} // namespace
