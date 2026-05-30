// /**
//  * @file main.cpp
//  * @author zuudevs (zuudevs@gmail.com)
//  * @brief Example usage of cpp_request library
//  * @version 1.0.0
//  * @date 2026-05-30
//  *
//  * @copyright Copyright (c) 2026
//  */

// #include <iostream>
// #include "cpp_request/client.h"
// #include "cpp_request/request.h"

// int main() {
//     zd::crq::HttpClient client;

//     // Example 1: Simple GET request
//     std::cout << "Making GET request...\n";
//     auto result = client.get("http://httpbin.org/get");
    
//     if (!result) {
//         std::cerr << "Request failed\n";
//         return 1;
//     }

//     auto& response = result.value();
    
//     // Print response details
//     std::cout << "Status Code: " << response.status_code() << "\n";
//     std::cout << "Status Message: " << response.status_message() << "\n";
//     std::cout << "Response Body Size: " << response.body().size() << " bytes\n";
    
//     // Example 2: POST with custom headers
//     std::cout << "\n--- Making POST request ---\n";
//     zd::crq::Request post_req;
//     post_req
//         .method(zd::crq::Method::Post)
//         .url("http://httpbin.org/post")
//         .header("Content-Type", "application/json")
//         .body(R"({"name":"cpp_request","version":"1.0"})");

//     auto post_result = client.send(post_req);
//     if (post_result) {
//         std::cout << "POST Status: " << post_result->status_code() << "\n";
//         std::cout << "POST Body Size: " << post_result->body().size() << " bytes\n";
//     } else {
//         std::cerr << "POST failed\n";
//     }

//     std::cout << "\nExamples completed successfully\n";
//     return 0;
// }


#include <print>
#include "cpp_request/client.h"
#include "cpp_request/request.h"

using namespace zd;

int main() {
	crq::HttpClient client;

	std::println("Send HTTP Request");
	
	auto req = crq::Request()
		.method(zd::crq::Method::Get)
		.url("http://httpbin.org/get")
		.header("Authorization", "TOKEN")
		.header("Content-Type", "application/json");

	if (auto res = client.send(req); !res) {
		std::println(stderr, "[ERROR] {}: {}", static_cast<int>(res.error()), crq::translate_error(res.error()));
	} else {
		std::println("{}", res->body_str());
	}
}