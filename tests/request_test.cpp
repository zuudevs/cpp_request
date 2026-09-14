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

TEST(RequestTest, ConstAccessorsExposeRequestState) {
    Request mutable_request{Method::Patch, "http://example.com/item/1"};
    mutable_request.set_body("patch-body");
    mutable_request.headers().add("X-Test", "value");

    const Request& request = mutable_request;

    EXPECT_EQ(request.method(), Method::Patch);
    EXPECT_EQ(request.url(), "http://example.com/item/1");
    EXPECT_EQ(request.body(), "patch-body");
    EXPECT_EQ(request.headers().get("x-test"), "value");
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
