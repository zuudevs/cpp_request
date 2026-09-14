# Architecture Layers

## Purpose

This document defines the allowed responsibilities and dependencies between `cpp_request` v1 layers.

## 1. Public API Layer

### Responsibilities

- expose request execution to users
- expose configuration and timeout values
- expose request/response data structures
- expose structured `Result<T>` and `Error`
- provide convenience helpers such as `get()` and `post()`

### Must not

- expose native socket handles
- include Winsock/POSIX implementation details in the supported API contract
- parse platform-native error codes directly
- implement HTTP wire parsing inline in user-facing types when separation is practical

## 2. HTTP Layer

### Responsibilities

- serialize HTTP/1.1 request lines and headers
- parse HTTP/1.1 status line and headers
- determine message framing
- decode chunked transfer encoding
- enforce HEAD response semantics
- determine connection-close and keep-alive semantics
- support redirect-related HTTP decisions

### Inputs

- method
- parsed URL/request target
- headers
- request body view
- byte sequences received from transport

### Outputs

- serialized request bytes or write segments
- parsed response metadata
- completed response body
- protocol errors
- connection reuse decision

### Must not

- call `socket()`, `connect()`, `send()`, `recv()`, or equivalent OS APIs directly
- own Winsock initialization
- perform DNS resolution

## 3. Transport Layer

### Responsibilities

- resolve endpoints through the platform abstraction
- establish TCP connections
- perform complete/partial write handling
- perform receive operations
- enforce connect/read/write timeout configuration
- own reusable connection state
- expose byte-oriented operations to the HTTP layer

### Must not

- interpret HTTP headers
- decode chunked transfer encoding
- implement redirect semantics
- depend on request methods such as GET or POST

## 4. Platform Socket Layer

### Responsibilities

- encapsulate native socket types
- initialize platform networking facilities where required
- create and close sockets
- configure blocking/non-blocking state or timeout-related options as required by the selected implementation strategy
- perform native address resolution calls
- expose normalized low-level results to transport
- capture native diagnostic information

### Windows

Expected backend: Winsock2-compatible APIs.

### POSIX

Expected backend: BSD/POSIX socket APIs suitable for Linux and compatible POSIX systems.

## Dependency Rules

Allowed dependencies:

```text
public -> http
public -> transport      (only through internal orchestration where required)
http   -> transport interface
transport -> platform
```

Forbidden dependencies:

```text
platform -> transport policy
platform -> HTTP
platform -> public API
transport -> HTTP semantics
HTTP -> native OS socket APIs
```

## Data Movement Principle

Data should cross layer boundaries by value only when ownership is required. Non-owning access should prefer lightweight views such as `std::string_view` when lifetime is explicit and safe.

This principle does not override correctness: response bodies remain owned because v1 exposes only completed in-memory responses.