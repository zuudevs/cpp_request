/**
 * @file main.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Example usage of cpp_request library
 * @version 1.0.0
 * @date 2026-05-30
 *
 * @copyright Copyright (c) 2026
 */

#include <iostream>
#include "cpp_request/request.h"

int main() {
    zd::crq::HttpClient client;

    // Example: Connect to example.com
    auto conn_result = client.connect("https://service-order-api-production.up.railway.app/health", 443);
    if (!conn_result) {
        std::cerr << "Failed to connect\n";
        return 1;
    }

    // Send HTTP GET request
    std::string request = "GET / HTTP/1.1\r\n"
                         "Host: https://service-order-api-production.up.railway.app/health\r\n"
                         "Connection: close\r\n"
                         "\r\n";

    auto send_result = client.send(request);
    if (!send_result) {
        std::cerr << "Failed to send request\n";
        return 1;
    }

    // Receive response
    auto recv_result = client.receive_all();
    if (!recv_result) {
        std::cerr << "Failed to receive response\n";
        return 1;
    }

    // Print response
    const auto& response = recv_result.value();
    std::cout.write(response.data(), response.size());
    std::cout << "\n";

    client.close();
    return 0;
}
