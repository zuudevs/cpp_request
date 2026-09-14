# Functional Requirements

## Document Status

- Project: `cpp_request`
- Target release: MVP v1.0
- Status: Frozen baseline
- Scope: Synchronous HTTP/1.1 client over native OS sockets

## Requirement Conventions

Each requirement uses a stable identifier so it can be referenced by implementation tasks, tests, benchmarks, and future architecture decisions.

Priority meanings:

- **MUST**: required for v1.0 acceptance.
- **SHOULD**: expected unless a documented implementation constraint prevents it.

---

## 1. Protocol and Request Methods

### REQ-HTTP-001 — HTTP/1.1 support
**Priority:** MUST

The library shall implement a client for HTTP/1.1 requests and responses.

**Acceptance criteria:**
- Outbound requests use an HTTP/1.1 request line.
- Incoming HTTP/1.1 status lines and headers can be parsed.
- The implementation handles connection semantics required by the v1 scope.

### REQ-HTTP-002 — Supported request methods
**Priority:** MUST

The library shall support the following HTTP methods:

- `GET`
- `HEAD`
- `POST`
- `PUT`
- `PATCH`
- `DELETE`

**Acceptance criteria:**
- Each method can be issued through the public client API.
- The generic request path can serialize the selected method correctly.

### REQ-HTTP-003 — Custom request headers
**Priority:** MUST

The caller shall be able to attach custom HTTP request headers.

**Acceptance criteria:**
- Multiple custom headers can be sent.
- Header names and values are serialized correctly.
- Required internal headers may be generated automatically where applicable.

### REQ-HTTP-004 — Query parameters
**Priority:** MUST

The caller shall be able to append query parameters to an HTTP request target.

**Acceptance criteria:**
- Query parameters are appended to the request target.
- Reserved characters are percent-encoded where required.

### REQ-HTTP-005 — In-memory request body
**Priority:** MUST

The library shall support request bodies provided from memory.

**Acceptance criteria:**
- A request body can be provided as an owned or referenced memory-backed value exposed by the public API.
- The body is transmitted completely before the request is considered successfully written.
- Request streaming is not required in v1.0.

---

## 2. URL and Address Resolution

### REQ-URL-001 — HTTP URL handling
**Priority:** MUST

The library shall accept plaintext `http://` URLs for v1.0.

**Acceptance criteria:**
- Scheme, host, optional port, path, and query components can be identified.
- Missing explicit port resolves to port 80 for HTTP.
- Unsupported schemes are rejected through structured error handling.

### REQ-URL-002 — HTTPS excluded from v1
**Priority:** MUST

The v1.0 implementation shall not provide HTTPS/TLS transport.

**Acceptance criteria:**
- `https://` input is rejected explicitly instead of silently downgraded.
- No TLS dependency is required by the core library.

### REQ-NET-001 — DNS resolution
**Priority:** MUST

The library shall resolve hostnames using the operating system networking facilities.

**Acceptance criteria:**
- Hostnames can resolve to one or more usable socket addresses.
- Resolution failures are returned as structured errors.

### REQ-NET-002 — IPv4 support
**Priority:** MUST

The library shall support IPv4 endpoints.

**Acceptance criteria:**
- Connections can be established to IPv4 addresses returned by the resolver.

### REQ-NET-003 — IPv6 support
**Priority:** MUST

The library shall support IPv6 endpoints.

**Acceptance criteria:**
- Connections can be established to IPv6 addresses returned by the resolver where the host platform supports IPv6.

### REQ-NET-004 — Native OS sockets
**Priority:** MUST

Networking shall be implemented directly on native operating-system socket APIs.

**Acceptance criteria:**
- Windows uses Winsock-compatible APIs.
- POSIX-compatible systems use BSD/POSIX socket APIs.
- The public API does not expose native socket handles.

---

## 3. Connection Management

### REQ-CONN-001 — TCP connection establishment
**Priority:** MUST

The library shall establish TCP connections to resolved HTTP endpoints.

**Acceptance criteria:**
- The client can connect to a resolved host and port.
- Connection failures are mapped into the library error model.

### REQ-CONN-002 — Connection reuse
**Priority:** MUST

The stateful client shall support reuse of eligible HTTP/1.1 connections.

**Acceptance criteria:**
- A reusable connection can service more than one sequential request when allowed by the server response.
- A connection marked for closure is not reused.

### REQ-CONN-003 — `Connection: close`
**Priority:** MUST

The library shall honor connection-close semantics from the peer.

**Acceptance criteria:**
- A response indicating `Connection: close` causes the associated connection to be closed after the response is consumed.
- The closed connection is not retained for reuse.

### REQ-CONN-004 — RAII ownership
**Priority:** MUST

Socket resources shall use deterministic lifetime management.

**Acceptance criteria:**
- Native socket resources are released on object destruction.
- Error paths do not leak socket handles.

---

## 4. Response Processing

### REQ-RESP-001 — Status line parsing
**Priority:** MUST

The library shall parse the HTTP response status line.

**Acceptance criteria:**
- HTTP version, numeric status code, and status text can be consumed without corrupting subsequent parsing.
- Malformed status lines produce a structured protocol error.

### REQ-RESP-002 — Response headers
**Priority:** MUST

The library shall parse and expose response headers.

**Acceptance criteria:**
- Multiple headers can be read from a response.
- Header lookup is available through the response API.

### REQ-RESP-003 — Content-Length body
**Priority:** MUST

The library shall support response bodies framed by `Content-Length`.

**Acceptance criteria:**
- Exactly the declared number of body bytes is consumed.
- Premature connection termination is reported as an error.

### REQ-RESP-004 — Chunked transfer decoding
**Priority:** MUST

The library shall support HTTP/1.1 responses using `Transfer-Encoding: chunked`.

**Acceptance criteria:**
- Chunk sizes are parsed correctly.
- Chunk framing bytes are not exposed as response body content.
- The terminal zero-size chunk is handled correctly.
- Malformed chunk framing returns a structured protocol error.

### REQ-RESP-005 — In-memory response body
**Priority:** MUST

The complete v1.0 response body shall be stored in memory before being exposed as a completed response.

**Acceptance criteria:**
- Successful responses expose a complete body buffer.
- Response streaming is not required in v1.0.

### REQ-RESP-006 — HEAD response semantics
**Priority:** MUST

The library shall handle `HEAD` responses without expecting a message body.

**Acceptance criteria:**
- A valid `HEAD` response completes after headers even if body-related headers are present.

---

## 5. Redirects

### REQ-REDIR-001 — Automatic redirects
**Priority:** MUST

The client shall support configurable automatic redirect handling.

**Acceptance criteria:**
- Redirect following can be enabled or disabled.
- Relative redirect targets can be resolved against the current request URL when supported by HTTP semantics.

### REQ-REDIR-002 — Redirect limit
**Priority:** MUST

The client shall enforce a finite redirect limit.

**Acceptance criteria:**
- Redirect loops do not continue indefinitely.
- Exceeding the configured maximum produces a structured error.

### REQ-REDIR-003 — Plain HTTP redirect scope
**Priority:** MUST

Automatic redirects in v1.0 shall remain within supported plaintext HTTP transport.

**Acceptance criteria:**
- Redirects to supported `http://` destinations may be followed.
- Redirects requiring unsupported HTTPS/TLS are rejected explicitly.

---

## 6. Timeouts

### REQ-TIME-001 — Connect timeout
**Priority:** MUST

The client shall expose a configurable connection timeout.

**Acceptance criteria:**
- Connection establishment does not block indefinitely when a timeout is configured.
- Timeout expiration produces a distinct structured error category.

### REQ-TIME-002 — Read timeout
**Priority:** MUST

The client shall expose a configurable read timeout.

**Acceptance criteria:**
- Waiting for response data is bounded by the configured read timeout.

### REQ-TIME-003 — Write timeout
**Priority:** MUST

The client shall expose a configurable write timeout.

**Acceptance criteria:**
- Request transmission is bounded by the configured write timeout.

---

## 7. Error Handling

### REQ-ERR-001 — Structured result type
**Priority:** MUST

Fallible public operations shall use a project-defined `Result<T>`-style abstraction compatible with C++11.

**Acceptance criteria:**
- The caller can distinguish success from failure without parsing strings.
- Successful operations expose their resulting value.
- Failed operations expose structured error information.

### REQ-ERR-002 — Structured error taxonomy
**Priority:** MUST

The library shall define structured error categories for expected URL, DNS, connection, timeout, send, receive, and protocol failures.

**Acceptance criteria:**
- Common failure paths map to stable library-owned error codes or categories.
- Platform-native error values may be preserved as diagnostic information without becoming the primary public error contract.

### REQ-ERR-003 — Exceptions not required for normal failures
**Priority:** MUST

Normal networking and HTTP processing failures shall not require exception handling by the caller.

**Acceptance criteria:**
- DNS failure, connection failure, timeout, malformed response, send failure, and receive failure are reportable through `Result<T>`.

---

## 8. Public API

### REQ-API-001 — Stateful Client API
**Priority:** MUST

The library shall expose a stateful `Client` abstraction for request execution and reusable connection state.

**Acceptance criteria:**
- A client instance can issue multiple sequential requests.
- Client-level configuration can be reused across requests.

### REQ-API-002 — Convenience request helpers
**Priority:** MUST

The library shall provide convenience free-function helpers for common HTTP methods.

**Acceptance criteria:**
- At minimum, `GET` and `POST` convenience paths are available.
- Convenience helpers ultimately use the same core request implementation semantics as the stateful client.

### REQ-API-003 — Native socket encapsulation
**Priority:** MUST

Native socket handles and platform-specific socket types shall remain implementation details.

**Acceptance criteria:**
- Public headers do not require callers to manipulate Winsock or POSIX socket handles directly.

### REQ-API-004 — No PImpl requirement
**Priority:** MUST

The v1.0 design shall not require the PImpl idiom for core public types.

**Acceptance criteria:**
- The implementation does not introduce mandatory per-object PImpl allocation solely for ABI hiding.

### REQ-API-005 — Thread-safety contract
**Priority:** MUST

A single `Client` instance is not required to be safe for concurrent use from multiple threads in v1.0.

**Acceptance criteria:**
- Documentation explicitly states the v1 thread-safety contract.
- The implementation is not required to add internal synchronization solely to support concurrent access to one client instance.

---

## 9. Explicitly Out of Scope for v1.0

The following features are intentionally excluded from the MVP v1.0 requirements:

- HTTPS / TLS
- asynchronous request execution
- C++ coroutines
- HTTP/2
- HTTP/3
- WebSocket
- proxy support
- cookie jar
- multipart request construction
- transparent gzip/brotli decompression
- request-body streaming
- response-body streaming
- connection pool management beyond simple eligible connection reuse
- automatic retry policy
- response caching

These items may be considered for post-v1 releases without changing the v1.0 acceptance criteria above.
