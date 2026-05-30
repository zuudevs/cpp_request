/**
 * @file test_example.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Example demonstrating various HTTP methods
 * @version 1.0.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#include <iostream>
#include "cpp_request/client.h"
#include "cpp_request/request.h"

int main() {
    zd::crq::HttpClient client;

    std::cout << "=== Testing HTTP GET ===\n";
    auto get_result = client.get("http://httpbin.org/get");
    if (get_result) {
        std::cout << "GET Success - Status: " << get_result->status_code() << "\n";
        std::cout << "Body size: " << get_result->body().size() << " bytes\n";
    } else {
        std::cerr << "GET Failed\n";
    }

    std::cout << "\n=== Testing HTTP POST ===\n";
    auto post_result = client.post("http://httpbin.org/post", R"({"key":"value"})");
    if (post_result) {
        std::cout << "POST Success - Status: " << post_result->status_code() << "\n";
        std::cout << "Body size: " << post_result->body().size() << " bytes\n";
    } else {
        std::cerr << "POST Failed\n";
    }

    std::cout << "\n=== Testing Custom Request ===\n";
    zd::crq::Request custom_req;
    custom_req
        .method(zd::crq::Method::Get)
        .url("http://httpbin.org/headers")
        .header("User-Agent", "cpp-request-client/1.0")
        .header("Accept", "application/json");

    auto custom_result = client.send(custom_req);
    if (custom_result) {
        std::cout << "Custom Request Success - Status: " << custom_result->status_code() << "\n";
    } else {
        std::cerr << "Custom Request Failed\n";
    }

    std::cout << "\n=== All Tests Completed Successfully ===\n";
    return 0;
}
