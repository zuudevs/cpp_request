# Architecture Overview

## Status

- Project: `cpp_request`
- Target release: MVP v1.0
- Status: Proposed architecture baseline
- Language baseline: C++17
- Protocol scope: synchronous HTTP/1.1 over plaintext TCP

## Goals

The v1 architecture is designed around four priorities:

1. **Lightweight** — avoid unnecessary dependencies, allocations, copies, and indirection.
2. **Fast** — keep protocol processing and socket I/O paths simple and measurable.
3. **Portable** — isolate Windows and POSIX networking details.
4. **Predictable** — explicit ownership, explicit errors, explicit blocking behavior.

## Layered Model

```text
Application
    |
    v
Public API
    |
    v
HTTP Layer
    |
    v
Transport Layer
    |
    v
Platform Socket Layer
    |
    v
Operating System
```

### Public API

Owns user-facing abstractions such as:

- `Client`
- `Request`
- `Response`
- `Headers`
- `Url`
- `Result<T>`
- `Error`

The public API must not expose native socket handles or platform networking types.

### HTTP Layer

Owns HTTP/1.1 protocol behavior:

- request serialization
- status-line parsing
- header parsing
- response framing
- `Content-Length`
- chunked transfer decoding
- redirect semantics
- keep-alive eligibility

This layer must not depend directly on Winsock or POSIX socket types.

### Transport Layer

Owns protocol-agnostic byte transport responsibilities:

- DNS resolution orchestration
- TCP connection establishment
- send/receive loops
- connect/read/write timeouts
- connection lifetime and reuse state

The transport layer does not parse HTTP semantics.

### Platform Socket Layer

Owns platform-specific networking details:

- Winsock startup and shutdown requirements
- native socket handle representation
- socket creation
- socket options
- platform error capture
- POSIX/BSD socket calls

Platform-native errors are translated before crossing into upper layers.

## Request Flow

```text
Client::request(...)
    |
    v
Validate + parse URL
    |
    v
Resolve endpoint
    |
    v
Reuse or establish TCP connection
    |
    v
Serialize HTTP request
    |
    v
Write bytes
    |
    v
Read response bytes
    |
    v
Parse status + headers
    |
    v
Decode response framing/body
    |
    v
Determine connection reuse eligibility
    |
    v
Return Result<Response>
```

## Ownership Rules

- `Client` owns reusable connection state.
- A transport connection owns exactly one native socket handle at a time.
- `Response` owns its completed in-memory body in v1.
- `Request` may reference non-owning text input through `std::string_view` only when the referenced data remains valid for the duration of request execution.
- Native socket ownership is never duplicated by copy.
- Resource-owning internal types use RAII and move semantics.

## Non-Goals

The v1 architecture intentionally does not solve:

- TLS/HTTPS
- asynchronous I/O
- coroutines
- HTTP/2 or HTTP/3
- WebSocket
- streaming request/response bodies
- proxy support
- generalized connection pools

These may be introduced in later releases without violating the v1 layer boundaries.

## Architectural Constraint

Upper layers may depend on lower layers, but lower layers must not depend on HTTP or public API semantics.

```text
Public API -> HTTP -> Transport -> Platform
```

Reverse dependencies are not permitted in the v1 design.