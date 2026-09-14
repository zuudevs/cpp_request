#include <gtest/gtest.h>

#include "http/chunked_decoder.hpp"

#include <cpp_request/error.hpp>

#include <string>
#include <string_view>

namespace {

using cpp_request::ErrorCode;
using cpp_request::detail::http::ChunkDecodeProgress;
using cpp_request::detail::http::ChunkedDecoder;

TEST(ChunkedDecoderTest, DecodesMultipleChunks) {
    ChunkedDecoder decoder;
    std::string input =
        "4\r\nWiki\r\n"
        "5\r\npedia\r\n"
        "0\r\n\r\n";
    std::string output;

    auto result = decoder.process(input, output);

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ChunkDecodeProgress::Complete);
    EXPECT_EQ(output, "Wikipedia");
    EXPECT_TRUE(input.empty());
}

TEST(ChunkedDecoderTest, HandlesOneByteAtATime) {
    ChunkedDecoder decoder;
    std::string input;
    std::string output;
    const std::string wire = "3\r\nabc\r\n2\r\nde\r\n0\r\n\r\n";

    ChunkDecodeProgress progress = ChunkDecodeProgress::NeedMore;
    for (const char ch : wire) {
        input.push_back(ch);
        auto result = decoder.process(input, output);
        ASSERT_TRUE(result);
        progress = result.value();
    }

    EXPECT_EQ(progress, ChunkDecodeProgress::Complete);
    EXPECT_EQ(output, "abcde");
    EXPECT_TRUE(input.empty());
}

TEST(ChunkedDecoderTest, ConsumesExtensionsAndTrailersAndPreservesFollowingBytes) {
    ChunkedDecoder decoder;
    std::string input =
        "4;foo=bar\r\nWiki\r\n"
        "0;end=yes\r\n"
        "X-Checksum: abc123\r\n"
        "X-Trace: done\r\n"
        "\r\n"
        "NEXT";
    std::string output;

    auto result = decoder.process(input, output);

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ChunkDecodeProgress::Complete);
    EXPECT_EQ(output, "Wiki");
    EXPECT_EQ(input, "NEXT");
}

TEST(ChunkedDecoderTest, RejectsInvalidChunkSize) {
    ChunkedDecoder decoder;
    std::string input = "xyz\r\n";
    std::string output;

    auto result = decoder.process(input, output);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidChunkSize);
}

TEST(ChunkedDecoderTest, RejectsMissingChunkDataCrlf) {
    ChunkedDecoder decoder;
    std::string input = "1\r\naXY";
    std::string output;

    auto result = decoder.process(input, output);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidChunkFraming);
}

TEST(ChunkedDecoderTest, RejectsMalformedTrailer) {
    ChunkedDecoder decoder;
    std::string input =
        "0\r\n"
        "Bad Trailer: value\r\n"
        "\r\n";
    std::string output;

    auto result = decoder.process(input, output);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidHeader);
}

TEST(ChunkedDecoderTest, RejectsFramingFieldsInTrailers) {
    ChunkedDecoder decoder;
    std::string input =
        "0\r\n"
        "Content-Length: 4\r\n"
        "\r\n";
    std::string output;

    auto result = decoder.process(input, output);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code, ErrorCode::InvalidHeader);
}

TEST(ChunkedDecoderTest, IncompleteChunkNeedsMoreData) {
    ChunkedDecoder decoder;
    std::string input = "5\r\nabc";
    std::string output;

    auto result = decoder.process(input, output);

    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), ChunkDecodeProgress::NeedMore);
    EXPECT_EQ(output, "abc");
    EXPECT_TRUE(input.empty());
}

} // namespace
