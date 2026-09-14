#include <gtest/gtest.h>

#include <cpp_request/headers.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace {

using cpp_request::Headers;

TEST(HeadersTest, AddPreservesDuplicatesAndInsertionOrder) {
    Headers headers;
    headers.add("Set-Cookie", "a=1");
    headers.add("X-Trace", "first");
    headers.add("set-cookie", "b=2");

    ASSERT_EQ(headers.size(), 3U);

    auto it = headers.begin();
    EXPECT_EQ(it->name, "Set-Cookie");
    EXPECT_EQ(it->value, "a=1");
    ++it;
    EXPECT_EQ(it->name, "X-Trace");
    EXPECT_EQ(it->value, "first");
    ++it;
    EXPECT_EQ(it->name, "set-cookie");
    EXPECT_EQ(it->value, "b=2");
}

TEST(HeadersTest, LookupIsAsciiCaseInsensitiveAndReturnsFirstValue) {
    Headers headers;
    headers.add("Content-Type", "text/plain");
    headers.add("content-type", "application/json");

    EXPECT_TRUE(headers.contains("CONTENT-TYPE"));
    EXPECT_EQ(headers.get("content-TYPE"), "text/plain");
    EXPECT_FALSE(headers.contains("Accept"));
    EXPECT_TRUE(headers.get("Accept").empty());
}

TEST(HeadersTest, SetReplacesAllMatchingFields) {
    Headers headers;
    headers.add("X-Test", "one");
    headers.add("Other", "keep");
    headers.add("x-test", "two");

    headers.set("X-TEST", "final");

    EXPECT_EQ(headers.size(), 2U);
    EXPECT_EQ(headers.get("x-test"), "final");

    std::size_t matches = 0;
    for (const auto& field : headers) {
        if (field.name == "X-TEST") {
            ++matches;
        }
    }
    EXPECT_EQ(matches, 1U);
}

TEST(HeadersTest, InsertedFieldsOwnTheirStorage) {
    Headers headers;
    std::string name = "X-Owned";
    std::string value = "original";

    headers.add(name, value);

    name.assign("changed");
    value.assign("mutated");

    EXPECT_TRUE(headers.contains("X-Owned"));
    EXPECT_EQ(headers.get("X-Owned"), "original");
}

TEST(HeadersTest, SetAddsFieldWhenMissing) {
    Headers headers;
    headers.set("Accept", "application/json");

    ASSERT_EQ(headers.size(), 1U);
    EXPECT_EQ(headers.get("accept"), "application/json");
}

} // namespace
