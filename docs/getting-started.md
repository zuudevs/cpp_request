# Getting started

This guide covers the stable cpp_request v1.0 public API.

## Requirements

- C++17
- CMake 3.21+ when building from source
- Windows, Linux, or macOS

`cpp_request` has no third-party runtime dependency. GoogleTest and Google Benchmark are development-only dependencies used by the project test and benchmark builds.

## Protocol scope

`cpp_request` v1 supports only `http://` URLs over plaintext TCP.

`https://` is rejected as an unsupported scheme before a TLS connection is attempted. TLS/HTTPS is intentionally post-v1 work.

## One-shot requests

For a single request, use the free helpers:

```cpp
#include <cpp_request/client.hpp>
#include <cpp_request/error.hpp>

#include <iostream>

int main() {
    auto result = cpp_request::get("http://127.0.0.1:8080/");
    if (!result) {
        const auto error = result.error();
        std::cerr << cpp_request::error_message(error.code);
        if (error.native_code != 0) {
            std::cerr << " (native=" << error.native_code << ')';
        }
        std::cerr << '\n';
        return 1;
    }

    const auto& response = result.value();
    std::cout << "status: " << response.status_code() << '\n';
    std::cout << response.body() << '\n';
}
```

Available one-shot helpers are `get`, `head`, `post`, `put`, `patch`, and `del`.

## Reusable Client

Use a persistent `Client` for repeated requests. When HTTP framing and peer behavior allow reuse, sequential requests to the same effective host and port can use the retained HTTP/1.1 connection.

```cpp
cpp_request::Client client;

auto first = client.get("http://127.0.0.1:8080/one");
auto second = client.get("http://127.0.0.1:8080/two");
```

There is no hidden retry of a request after a stale keep-alive failure. A failed stale connection is discarded; the caller decides whether to issue another request.

## POST body

The convenience body helpers accept a borrowed `std::string_view`:

```cpp
std::string body = R"({"name":"alpha"})";
auto result = client.post("http://127.0.0.1:8080/items", body);
```

For custom headers, construct `Request` explicitly.

## Custom headers and query parameters

```cpp
#include <cpp_request/client.hpp>
#include <cpp_request/request.hpp>

#include <string>

std::string body = R"({"name":"alpha"})";

cpp_request::Request request{
    cpp_request::Method::Post,
    "http://127.0.0.1:8080/items?existing=1"};

request.headers().set("Content-Type", "application/json");
request.headers().add("X-Trace", "first");
request.headers().add("X-Trace", "second");
request.add_query_param("search", "low level C++");
request.add_query_param("tag", "network/http");
request.set_body(body);

auto result = client.request(request);
```

Header names are looked up case-insensitively. Ordered duplicate fields are preserved. `Headers::get()` returns the first matching field.

Appended query parameters preserve insertion order and are percent-encoded by the request serializer. An existing raw query in the URL is preserved before appended parameters.

## Borrowed lifetime rules

`Request` intentionally uses non-owning views for its URL and body. This avoids an unconditional payload copy.

The URL and body storage must remain valid until `Client::request()` returns:

```cpp
std::string url = "http://127.0.0.1:8080/items";
std::string body = "payload";

cpp_request::Request request{cpp_request::Method::Post, url};
request.set_body(body);

auto result = client.request(request); // url/body still alive here
```

Headers and appended query parameters own their strings.

A successful `Response` owns its reason phrase, headers, and body. Views returned by `Response` stay valid while that `Response` object and its corresponding owned storage remain alive and unchanged.

## Timeouts

Connect, read, and write timeouts are configured separately:

```cpp
using namespace std::chrono_literals;

cpp_request::Client client;
client.set_connect_timeout(2s);
client.set_write_timeout(5s);
client.set_read_timeout(5s);
```

The connect timeout is the connection-attempt budget, the write timeout bounds request transmission, and the read timeout applies while waiting for response progress.

## Redirects

Redirect following is enabled by default with a maximum of 10 redirects.

```cpp
client.set_follow_redirects(true);
client.set_max_redirects(5);
```

Supported redirect status codes are 301, 302, 303, 307, and 308. Redirects to unsupported schemes such as HTTPS fail explicitly instead of silently changing transports.

## Response limits

Responses are memory-resident in v1, so finite limits are part of the public client configuration:

```cpp
cpp_request::ResponseLimits limits;
limits.max_head_bytes = 64 * 1024;
limits.max_body_bytes = 16 * 1024 * 1024;
limits.max_chunk_line_bytes = 8 * 1024;
limits.max_trailer_bytes = 64 * 1024;
client.set_response_limits(limits);
```

The defaults are 64 KiB response head, 64 MiB decoded body, 8 KiB chunk-size line, and 64 KiB aggregate trailers. Zero is a real zero-byte limit, not an unlimited sentinel.

A limit violation returns `ErrorCode::ResponseLimitExceeded`.

## Error handling

Expected URL, transport, timeout, HTTP framing, redirect, and response-limit failures are returned as `Result<Response>`.

```cpp
auto result = client.get("http://127.0.0.1:8080/");
if (!result) {
    const cpp_request::Error& error = result.error();
    std::cerr << cpp_request::error_message(error.code) << '\n';
    return;
}

const cpp_request::Response& response = result.value();
```

`Result<T>::value()` and `Result<T>::error()` have state preconditions. Check the result first; accessing the wrong state is programmer misuse.

## Thread safety

`Client` is move-only and owns mutable reusable-connection state. One `Client` is not guaranteed safe for concurrent requests from multiple threads.

Use one `Client` per independent execution context, or provide external synchronization around a shared instance.

## Install-tree consumption

Build and install:

```sh
cmake -S . -B build \
  -DCPP_REQUEST_BUILD_TESTS=OFF \
  -DCPP_REQUEST_BUILD_EXAMPLES=OFF
cmake --build build --config Release
cmake --install build --config Release --prefix /path/to/prefix
```

Consume:

```cmake
cmake_minimum_required(VERSION 3.21)
project(example LANGUAGES CXX)

find_package(cpp_request CONFIG REQUIRED)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE cpp_request::cpp_request)
target_compile_features(example PRIVATE cxx_std_17)
```

If the install prefix is not in a default search location, configure the consumer with:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/prefix
```

## Examples

The repository examples target a local HTTP server at `127.0.0.1:8080` and do not depend on public internet services.

Build them with:

```sh
cmake -S . -B build -DCPP_REQUEST_BUILD_EXAMPLES=ON
cmake --build build --config Release
```

Examples are demonstrations, not network-dependent automated tests.
