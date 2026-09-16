# cpp_request v1 Roadmap

## Status

- Project: `cpp_request`
- Target release: MVP v1.0
- Current development version: v0.9.0
- Language baseline: C++17
- Protocol scope: synchronous HTTP/1.1 over plaintext TCP
- Current milestone: v1.0 release hardening
- Last roadmap review: 2026-09-16

The frozen functional and non-functional requirements remain authoritative for **what** v1 must provide. This roadmap records implementation milestones and the remaining release work.

The detailed v1 acceptance evidence is maintained in [`docs/release/v1.0-acceptance.md`](release/v1.0-acceptance.md).

---

## Roadmap Overview

| Milestone | Focus | Status |
| --- | --- | --- |
| v0.1 | Requirements, architecture, API/error contracts | ✅ Complete |
| v0.2 | URL, platform sockets, DNS, TCP transport | ✅ Complete |
| v0.3 | HTTP request model and serialization | ✅ Complete |
| v0.4 | HTTP response parsing and body framing | ✅ Complete |
| v0.5 | Public `Client` execution path | ✅ Complete |
| v0.6 | HTTP/1.1 connection reuse / keep-alive | ✅ Complete |
| v0.7 | Redirect handling | ✅ Complete |
| v0.8 | Protocol/API correctness hardening | ✅ Complete |
| v0.9 | Packaging, benchmarks, examples, documentation | ✅ Complete |
| v1.0 | Release hardening and acceptance gate | 🚧 In progress |

---

## Completed implementation milestones

### v0.1 — Specification and architecture

Frozen v1 requirements, architecture boundaries, public API/error/lifetime contracts, C++17 baseline, synchronous-only execution, no mandatory PImpl, and explicit HTTPS/TLS exclusion.

References: PR #2, #3, #4, #5.

### v0.2 — Native transport foundation

URL parsing, native socket RAII, DNS, IPv4/IPv6 endpoints, timed TCP connect/read/write, endpoint fallback, partial I/O, and platform error normalization.

References: PR #6–#10.

### v0.3 — Request model and serializer

Ordered headers, request methods, borrowed request URL/body, Host generation, Content-Length behavior, custom headers, query parameters, and request injection validation.

References: PR #11, #12, #23.

### v0.4 — Response parser and framing

Incremental status/header parsing, Content-Length, close-delimited and chunked bodies, HEAD/1xx/204/205/304 semantics, trailer/chunk validation, EOF handling, and framing-conflict rejection.

References: PR #13, #14, #24.

### v0.5 — Public Client

Stateful synchronous `Client`, one-shot helpers, configurable connect/read/write timeouts, structured errors, and loopback integration coverage.

Reference: PR #15.

### v0.6 — Connection reuse

Single eligible retained connection, same-origin reuse, forced close rules, origin-change reconnect, stale keep-alive handling without hidden retry, and move-only resource ownership.

Reference: PR #20.

### v0.7 — Redirects

Bounded configurable redirects, HTTP redirect method/body policy, relative target resolution, sensitive-header stripping across origins, and explicit unsupported-scheme handling.

Reference: PR #21.

### v0.8 — Correctness and resource hardening

`Result<T>` hardening, URL/query correctness, HTTP framing review, explicit timeout/EOF semantics, response resource limits, deterministic boundary tests, and connection invalidation after parsing/resource failures.

References: PR #22–#25.

### v0.9 — Consumability and measurement

Installable CMake package, stable `cpp_request::cpp_request` target, install-tree consumer test, benchmark suite and smoke CI, modular CMake architecture, examples, README, and getting-started documentation.

References: PR #26–#29.

---

# v1.0 — Release Hardening and Acceptance Gate 🚧

Goal: verify the frozen requirements as one coherent release candidate without adding new feature scope.

## Automated verification

The release candidate must have:

- Windows Debug/Release CI green,
- Linux Debug/Release CI green,
- macOS Debug/Release CI green,
- C++17 build/test coverage,
- C++20 compatibility coverage,
- ASan + UBSan test execution where supported,
- warnings-as-errors release-gate builds,
- unit and loopback integration tests green,
- no required HTTP test depending on a public service,
- install-tree consumer test green,
- C++20 installed-consumer test green,
- benchmark smoke jobs green,
- examples compiling,
- every public header compiling independently without `src/` includes.

## Required review

Before v1.0.0 promotion:

- every MUST functional requirement must have implementation/test evidence,
- every MUST non-functional requirement must have a verification path,
- no known bug may corrupt HTTP message framing or reuse an invalid connection,
- expected URL/network/protocol/resource failures must remain structured,
- the installed library must have zero third-party runtime dependency,
- public API/lifetime/error documentation must match implementation,
- performance claims must be grounded in the benchmark suite,
- release version/tag metadata must be finalized.

The requirement-to-evidence matrix lives in [`docs/release/v1.0-acceptance.md`](release/v1.0-acceptance.md).

## Promotion sequence

```text
v1.0 hardening gates green
    ↓
manual acceptance review
    ↓
focused 0.9.0 → 1.0.0 release PR
    ↓
merge
    ↓
v1.0 tag / release
```

The current hardening branch intentionally keeps the project version at `0.9.0`. The version becomes `1.0.0` only after the acceptance gate is satisfied.

---

# Frozen v1 Scope Boundary

The following remain intentionally outside v1.0:

- HTTPS / TLS
- asynchronous API and coroutines
- HTTP/2 and HTTP/3
- WebSocket
- proxy support
- cookie jar
- multipart builder
- gzip/brotli decompression
- request streaming
- response streaming
- generalized connection pool
- automatic retries
- response cache

No item above should enter the v1 release path unless the frozen requirements are explicitly revised first.

---

# Post-v1 Direction

Potential future work includes TLS/HTTPS, async/coroutine execution, streaming bodies, multi-origin connection pooling, proxy/cookie support, compression, retry policies, and later HTTP versions. Each post-v1 feature should receive its own requirements/design work before implementation.
