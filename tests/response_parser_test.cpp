#include <gtest/gtest.h>

#include "http/response_parser.hpp"

#include <cpp_request/error.hpp>
#include <cpp_request/request.hpp>

#include <string>
#include <string_view>

namespace {

using cpp_request::ErrorCode;
using cpp_request::Method;
using cpp_request::detail::http::ResponseParseProgress;
using cpp_request::detail::http::ResponseParser;

TEST(ResponseParserTest, ParsesContentLengthResponseAcrossArbitraryChunks) {
    ResponseParser parser{Method::Get};

    const std::string wire =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 11\r\n"
        "X-Test: one\r\n"
        "X-Test: two\r\n"
        "\r\n"
        "hello world";

    for (const char ch : wire) {
        auto result = parser.feed(std::string_view{&ch, 1});
        ASSERT_TRUE(result);
    }

    ASSERT_TRUE(parser.complete());
    EXPECT_TRUE(parser.connection_reusable());

    auto response = parser.take_response();
    EXPECT_EQ(response.status_code(), 200);
    EXPECT_EQ(response.reason(), "OK");
    EXPECT_EQ(response.headers().get("content-type"), "text/plain");
    EXPECT_EQ(response.headers().get("x-test"), "one");
    EXPECT_EQ(response.body(), "hello world");
}

TEST(ResponseParserTest, PreservesBytesAfterContentLengthBody) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "helloHTTP/1.1 204 No Content\r\n\r\n");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ResponseParseProgress::Complete);
    EXPECT_EQ(parser.response().body(), "hello");
    EXPECT_EQ(
        parser.pending_bytes(),
        "HTTP/1.1 204 No Content\r\n\r\n");
}

TEST(ResponseParserTest, HeadCompletesAfterHeadersAndLeavesFollowingBytesPending) {
    ResponseParser parser{Method::Head};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 1234\r\n"
        "\r\n"
        "next-response-bytes");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ResponseParseProgress::Complete);
    EXPECT_TRUE(parser.response().body().empty());
    EXPECT_EQ(parser.pending_bytes(), "next-response-bytes");
    EXPECT_TRUE(parser.connection_reusable());
}

TEST(ResponseParserTest, CloseDelimitedBodyCompletesOnlyAtEof) {
    ResponseParser parser{Method::Get};

    auto first = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "hello ");
    ASSERT_TRUE(first);
    EXPECT_EQ(first.value(), ResponseParseProgress::NeedMore);

    auto second = parser.feed("world");
    ASSERT_TRUE(second);
    EXPECT_EQ(second.value(), ResponseParseProgress::NeedMore);
    EXPECT_FALSE(parser.complete());

    auto eof = parser.finish_eof();
    ASSERT_TRUE(eof);
    EXPECT_EQ(eof.value(), ResponseParseProgress::Complete);
    EXPECT_EQ(parser.response().body(), "hello world");
    EXPECT_FALSE(parser.connection_reusable());
}

TEST(ResponseParserTest, ConnectionClosePreventsReuseEvenWithContentLength) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "Connection: keep-alive, close\r\n"
        "\r\n"
        "ok");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ResponseParseProgress::Complete);
    EXPECT_FALSE(parser.connection_reusable());
}

TEST(ResponseParserTest, ChunkedResponseHandsOffWithoutConsumingBodyBytes) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "4\r\nWiki\r\n0\r\n\r\n");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ResponseParseProgress::NeedsChunkedDecoder);
    EXPECT_FALSE(parser.complete());
    EXPECT_EQ(parser.pending_bytes(), "4\r\nWiki\r\n0\r\n\r\n");
}

TEST(ResponseParserTest, RejectsTransferEncodingAndContentLengthTogether) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Length: 4\r\n"
        "\r\n");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::ConflictingMessageFraming);
}

TEST(ResponseParserTest, RejectsDuplicateContentLength) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 4\r\n"
        "Content-Length: 4\r\n"
        "\r\n");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidContentLength);
}

TEST(ResponseParserTest, RejectsInvalidContentLength) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 4x\r\n"
        "\r\n");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidContentLength);
}

TEST(ResponseParserTest, RejectsMalformedStatusLine) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed("HTTP/1.0 200 OK\r\n\r\n");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidStatusLine);
}

TEST(ResponseParserTest, RejectsMalformedHeader) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Bad Header: value\r\n"
        "\r\n");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidHeader);
}

TEST(ResponseParserTest, ConsumesInterimResponseBeforeFinalResponse) {
    ResponseParser parser{Method::Post};

    auto result = parser.feed(
        "HTTP/1.1 100 Continue\r\n"
        "X-Interim: ignored\r\n"
        "\r\n"
        "HTTP/1.1 201 Created\r\n"
        "Content-Length: 2\r\n"
        "\r\n"
        "ok");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ResponseParseProgress::Complete);
    EXPECT_EQ(parser.response().status_code(), 201);
    EXPECT_EQ(parser.response().reason(), "Created");
    EXPECT_FALSE(parser.response().headers().contains("X-Interim"));
    EXPECT_EQ(parser.response().body(), "ok");
}

TEST(ResponseParserTest, NoBodyStatusCompletesAtHeaders) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed(
        "HTTP/1.1 204 No Content\r\n"
        "Content-Length: 0\r\n"
        "\r\n");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ResponseParseProgress::Complete);
    EXPECT_TRUE(parser.response().body().empty());
    EXPECT_TRUE(parser.connection_reusable());
}

TEST(ResponseParserTest, PrematureEofForContentLengthIsError) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "abc");
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ResponseParseProgress::NeedMore);

    auto eof = parser.finish_eof();
    ASSERT_FALSE(eof);
    EXPECT_EQ(eof.error().code, ErrorCode::UnexpectedEof);
}

} // namespace
