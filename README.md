# cpp_request

`cpp_request` is a lightweight synchronous HTTP/1.1 client for C++17 built directly on native OS sockets.

The v1 scope is intentionally small: plaintext HTTP over TCP, predictable synchronous execution, structured errors, and zero third-party runtime dependencies.

> HTTPS/TLS is not supported in v1.

## Highlights

- C++17
- HTTP/1.1 over plaintext TCP
- Windows, Linux, and macOS
- native Winsock2 / POSIX sockets
- GET, HEAD, POST, PUT, PATCH, DELETE
- custom headers and query parameters
- IPv4 and IPv6
- configurable connect/read/write timeouts
- bounded automatic redirects
- HTTP/1.1 keep-alive connection reuse
- `Content-Length`, close-delimited, and chunked response bodies
- configurable in-memory response limits
- structured `Result<T>` / `Error` model
- installable CMake package: `cpp_request::cpp_request`
- zero third-party runtime dependencies

## Quick start

```cpp
#include <cpp_request/client.hpp>
#include <cpp_request/error.hpp>

#include <iostream>

int main() {
    auto result = cpp_request::get("http://127.0.0.1:8080/");
    if (!result) {
        std::cerr << cpp_request::error_message(result.error().code) << '\n';
        return 1;
    }

    const auto& response = result.value();
    std::cout << response.status_code() << '\n';
    std::cout << response.body() << '\n';
}
```

The one-shot helpers create a temporary `Client`. For repeated requests to the same origin, keep a `Client` alive so eligible HTTP/1.1 connections can be reused.

## Build

Requirements:

- CMake 3.21+
- a C++17 compiler

```sh
cmake -S . -B build -DCPP_REQUEST_BUILD_TESTS=OFF
cmake --build build --config Release
```

To build the examples too:

```sh
cmake -S . -B build -DCPP_REQUEST_BUILD_EXAMPLES=ON
cmake --build build --config Release
```

## Install and consume with CMake

Install into a prefix:

```sh
cmake -S . -B build \
  -DCPP_REQUEST_BUILD_TESTS=OFF \
  -DCPP_REQUEST_BUILD_EXAMPLES=OFF
cmake --build build --config Release
cmake --install build --config Release --prefix /path/to/prefix
```

Consumer project:

```cmake
find_package(cpp_request CONFIG REQUIRED)

target_link_libraries(my_app PRIVATE cpp_request::cpp_request)
```

Point CMake at a non-system prefix with `CMAKE_PREFIX_PATH` when necessary.

## Custom request

```cpp
#include <cpp_request/client.hpp>
#include <cpp_request/request.hpp>

#include <string>

cpp_request::Client client;

std::string body = R"({"name":"example"})";
cpp_request::Request request{
    cpp_request::Method::Post,
    "http://127.0.0.1:8080/items"};

request.headers().set("Content-Type", "application/json");
request.headers().set("Accept", "application/json");
request.add_query_param("source", "cpp request");
request.set_body(body);

auto result = client.request(request);
```

`Request` borrows its URL and body through `std::string_view`; the referenced storage must remain alive until `Client::request()` returns. Headers and appended query parameters own their storage.

## Client configuration

```cpp
#include <cpp_request/client.hpp>
#include <cpp_request/response_limits.hpp>

#include <chrono>

using namespace std::chrono_literals;

cpp_request::Client client;
client.set_connect_timeout(2s);
client.set_read_timeout(5s);
client.set_write_timeout(5s);
client.set_follow_redirects(true);
client.set_max_redirects(5);

cpp_request::ResponseLimits limits;
limits.max_head_bytes = 32 * 1024;
limits.max_body_bytes = 8 * 1024 * 1024;
client.set_response_limits(limits);
```

## Thread safety

A `Client` owns mutable connection-reuse state and is **not guaranteed thread-safe**. Do not concurrently call one `Client` from multiple threads without external synchronization. Separate `Client` instances may be used independently.

## v1 scope

Supported in v1:

- synchronous HTTP/1.1
- in-memory request and response bodies
- one reusable same-origin connection per `Client`
- finite redirect following
- explicit resource limits

Intentionally outside v1:

- HTTPS / TLS
- asynchronous APIs and coroutines
- HTTP/2 and HTTP/3
- WebSocket
- proxy support
- cookie jar
- multipart builder
- gzip/brotli decompression
- request/response streaming
- generalized connection pool
- automatic retries
- response cache

## Documentation

- [Getting started](docs/getting-started.md)
- [Public API](docs/api/public-api.md)
- [Error model](docs/api/error-model.md)
- [Result model](docs/api/result.md)
- [Lifetime rules](docs/api/lifetime.md)
- [Response limits](docs/api/response-limits.md)
- [Roadmap](docs/roadmap.md)
- [CMake structure](cmake/README.md)

## Development

Tests are enabled by default when `cpp_request` is the top-level project. Benchmarks are opt-in:

```sh
cmake -S . -B build-bench \
  -DBUILD_TESTING=OFF \
  -DCPP_REQUEST_BUILD_BENCHMARKS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --config Release --target cpp_request_benchmark_smoke
```

See `cmake/README.md` for the complete build-option list.
