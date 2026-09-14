#include <gtest/gtest.h>

#include "http/response_parser.hpp"

#include <cpp_request/request.hpp>

namespace {

using cpp_request::Method;
using cpp_request::detail::http::ResponseParseProgress;
using cpp_request::detail::http::ResponseParser;

TEST(ResponseParserUpgradeTest, SwitchingProtocolsIsNotReusableHttpConnection) {
    ResponseParser parser{Method::Get};

    auto result = parser.feed(
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "\r\n");

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ResponseParseProgress::Complete);
    EXPECT_TRUE(parser.complete());
    EXPECT_FALSE(parser.connection_reusable());
}

} // namespace
