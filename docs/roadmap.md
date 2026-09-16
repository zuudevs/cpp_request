# cpp_request v1 Roadmap

## Status

- Project: `cpp_request`
- Target release: v1.0.0
- Current project version: v1.0.0
- Language baseline: C++17
- Protocol scope: synchronous HTTP/1.1 over plaintext TCP
- Current milestone: v1.0 implementation and acceptance complete; release tag pending
- Last roadmap review: 2026-09-16

The frozen functional and non-functional requirements remain authoritative for **what** v1 provides. Detailed acceptance evidence is maintained in [`docs/release/v1.0-acceptance.md`](release/v1.0-acceptance.md), with the manual review in [`docs/release/v1.0-manual-review.md`](release/v1.0-manual-review.md).

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
| v1.0 | Release hardening, acceptance, stable API promotion | ✅ Complete |

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

### v1.0 — Release hardening and stable promotion

Completed work includes:

- Windows/Linux/macOS Debug and Release CI,
- C++17 baseline and C++20 compatibility coverage,
- ASan + UBSan release-gate execution,
- warnings-as-errors hardening builds,
- unit and local-loopback integration tests,
- install-tree and C++20 consumer verification,
- benchmark smoke execution,
- public-header isolation,
- HTTP framing/connection-reuse manual review,
- public API/lifetime/error/result contract freeze,
- synchronized CMake and manifest version metadata at `1.0.0`,
- final v1.0 release notes.

Release-hardening references: PR #30 and #31. The final promotion PR completes the `1.0.0` version transition.

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

These are post-v1 features and require separate requirements/design work before implementation.

---

# Release sequence

```text
v1.0 hardening gates green ✅
    ↓
manual acceptance review ✅
    ↓
1.0.0 promotion PR + final gates  ← current
    ↓
merge
    ↓
tag v1.0.0 / GitHub Release
```

The tag and GitHub Release are intentionally separate from the promotion PR and must only be created after the promotion PR is green and merged.

---

# Post-v1 Direction

Potential future work includes TLS/HTTPS, async/coroutine execution, streaming bodies, multi-origin connection pooling, proxy/cookie support, compression, retry policies, and later HTTP versions. Each post-v1 feature should receive its own requirements/design work before implementation.
