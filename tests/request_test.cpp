#include <gtest/gtest.h>

#include <cpp_request/request.hpp>

#include <string>

namespace {

using cpp_request::Method;
using cpp_request::Request;

TEST(RequestTest, StoresMethodAndBorrowedUrl) {
    std::string url = "http://example.com/items";
    Request request{Method::Get, url};

    EXPECT_EQ(request.method(), Method::Get);
    EXPECT_EQ(request.url(), url);
    EXPECT_EQ(request.url().data(), url.data());
}

TEST(RequestTest, BodyIsBorrowedWithoutCopy) {
    std::string body = R"({"name":"zuu"})";
    Request request{Method::Post, "http://example.com/items"};

    request.set_body(body);

    EXPECT_EQ(request.body(), body);
    EXPECT_EQ(request.body().data(), body.data());
}

TEST(RequestTest, OwnsHeaderStorageIndependentlyFromBorrowedRequestData) {
    std::string header_name = "Content-Type";
    std::string header_value = "application/json";

    Request request{Method::Post, "http://example.com/items"};
    request.headers().add(header_name, header_value);

    header_name.assign("changed");
    header_value.assign("changed");

    EXPECT_EQ(request.headers().get("content-type"), "application/json");
}

TEST(RequestTest, OwnsQueryParameterStorage) {
    std::string name = "search";
    std::string value = "hello world";

    Request request{Method::Get, "http://example.com/items"};
    request.add_query_param(name, value);

    name.assign("changed");
    value.assign("changed");

    ASSERT_EQ(request.query_params().size(), 1u);
    EXPECT_EQ(request.query_params()[0].name, "search");
    EXPECT_EQ(request.query_params()[0].value, "hello world");
}

TEST(RequestTest, PreservesRepeatedQueryParametersInInsertionOrder) {
    Request request{Method::Get, "http://example.com/items"};
    request.add_query_param("tag", "cpp");
    request.add_query_param("tag", "http");

    ASSERT_EQ(request.query_params().size(), 2u);
    EXPECT_EQ(request.query_params()[0].name, "tag");
    EXPECT_EQ(request.query_params()[0].value, "cpp");
    EXPECT_EQ(request.query_params()[1].name, "tag");
    EXPECT_EQ(request.query_params()[1].value, "http");
}

TEST(RequestTest, ConstAccessorsExposeRequestState) {
    Request mutable_request{Method::Patch, "http://example.com/item/1"};
    mutable_request.set_body("patch-body");
    mutable_request.headers().add("X-Test", "value");
    mutable_request.add_query_param("mode", "fast");

    const Request& request = mutable_request;

    EXPECT_EQ(request.method(), Method::Patch);
    EXPECT_EQ(request.url(), "http://example.com/item/1");
    EXPECT_EQ(request.body(), "patch-body");
    EXPECT_EQ(request.headers().get("x-test"), "value");
    ASSERT_EQ(request.query_params().size(), 1u);
    EXPECT_EQ(request.query_params()[0].name, "mode");
    EXPECT_EQ(request.query_params()[0].value, "fast");
}

TEST(RequestTest, SupportsAllV1Methods) {
    EXPECT_EQ((Request{Method::Get, "http://example.com"}.method()), Method::Get);
    EXPECT_EQ((Request{Method::Head, "http://example.com"}.method()), Method::Head);
    EXPECT_EQ((Request{Method::Post, "http://example.com"}.method()), Method::Post);
    EXPECT_EQ((Request{Method::Put, "http://example.com"}.method()), Method::Put);
    EXPECT_EQ((Request{Method::Patch, "http://example.com"}.method()), Method::Patch);
    EXPECT_EQ((Request{Method::Delete, "http://example.com"}.method()), Method::Delete);
}

} // namespace
