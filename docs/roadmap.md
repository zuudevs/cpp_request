# cpp_request v1 Roadmap

## Status

- Project: `cpp_request`
- Target release: MVP v1.0
- Current development version: v0.9.0
- Language baseline: C++17
- Protocol scope: synchronous HTTP/1.1 over plaintext TCP
- Last roadmap review: 2026-09-15

This document is the implementation-order source of truth for the v1 release. The frozen functional and non-functional requirements remain authoritative for **what** v1 must provide; this roadmap records milestone completion and the remaining release gate.

Version labels are development milestones and do not require a public release tag unless explicitly decided.

---

## Status Legend

- ✅ **Complete** — merged implementation exists and the milestone exit criteria are satisfied.
- 🚧 **In progress** — active implementation or finalization work exists.
- ⏳ **Planned** — required work has not started.
- 🧪 **Release gate** — integrated verification required before v1.0.
- ➡️ **Post-v1** — intentionally outside the frozen v1 scope.

---

## Roadmap Overview

| Milestone | Focus | Status |
| --- | --- | --- |
| v0.1 | Requirements, architecture, API/error contracts | ✅ Complete |
| v0.2 | URL, platform sockets, DNS, TCP transport | ✅ Complete |
| v0.3 | HTTP request data model and serialization | ✅ Complete |
| v0.4 | HTTP response parsing and body framing | ✅ Complete |
| v0.5 | Public `Client` end-to-end execution path | ✅ Complete |
| v0.6 | HTTP/1.1 connection reuse / keep-alive | ✅ Complete |
| v0.7 | Redirect handling | ✅ Complete |
| v0.8 | Protocol/API correctness hardening | ✅ Complete |
| v0.9 | Packaging, benchmarks, examples, documentation | ✅ Complete on merge of PR #29 |
| v1.0 | Release hardening and acceptance gate | 🧪 Next |

---

# v0.1 — Specification and Architecture Baseline ✅

Goal: freeze the v1 product boundary before implementation expands.

Completed:

- frozen functional and non-functional requirements,
- layered Public API / HTTP / Transport / Platform architecture,
- public API, lifetime, error, and `Result<T>` contracts,
- C++17 minimum,
- synchronous-only v1,
- no PImpl requirement,
- HTTPS/TLS explicitly outside v1.

Historical references:

- PR #2 — frozen v1 requirements
- PR #3 — architecture boundaries
- PR #4 — public API contract
- PR #5 — error and `Result<T>` contracts

Exit criteria: **complete**.

---

# v0.2 — Native Transport Foundation ✅

Goal: provide a portable bounded TCP byte stream without leaking native sockets into the public API.

Completed:

- URL parsing and HTTP-only scheme validation,
- IPv4 and IPv6 endpoints,
- Winsock/POSIX native socket RAII,
- network runtime initialization,
- OS DNS resolution,
- timed connect/read/write,
- endpoint fallback,
- partial-I/O handling,
- SIGPIPE-safe POSIX behavior,
- normalized transport errors.

Historical references:

- PR #6 — URL parser
- PR #7 — native socket RAII
- PR #8 — DNS resolver and endpoints
- PR #9 — timed TCP connect
- PR #10 — timed TCP read/write

Exit criteria: **complete**.

---

# v0.3 — HTTP Request Model and Serialization ✅

Goal: represent and serialize all v1 request methods while avoiding unnecessary payload copies.

Completed:

- ordered owning `Headers`,
- case-insensitive lookup and duplicate fields,
- borrowed request URL/body,
- GET / HEAD / POST / PUT / PATCH / DELETE,
- request-line and header serialization,
- automatic `Host`,
- IPv6 Host formatting,
- Content-Length generation/validation,
- request-header syntax and injection validation.

Historical references:

- PR #11 — `Headers` and `Request`
- PR #12 — request serializer

Exit criteria: **complete**.

---

# v0.4 — HTTP Response Parser and Framing ✅

Goal: parse HTTP/1.1 incrementally and independently of TCP packet boundaries.

Completed:

- owning `Response`,
- status-line and header parsing,
- interim 1xx handling,
- HEAD/no-body semantics,
- Content-Length framing,
- close-delimited framing,
- chunked transfer decoding,
- chunk extension/trailer validation,
- EOF and framing-conflict detection,
- connection-reuse eligibility signals,
- preservation of pending bytes belonging to the following response.

Historical references:

- PR #13 — incremental response parser
- PR #14 — chunked transfer decoder

Exit criteria: **complete**.

---

# v0.5 — Public Client Core ✅

Goal: connect serialization, DNS, TCP, parser, and public result handling into one synchronous request path.

Completed:

- `Client::request(const Request&)`,
- member GET / HEAD / POST / PUT / PATCH / DELETE helpers,
- equivalent one-shot free helpers,
- connect/read/write timeout configuration,
- loopback integration coverage,
- unsupported HTTPS rejection before networking.

Historical reference:

- PR #15 — public `Client` execution path

Exit criteria: **complete**.

---

# v0.6 — Connection Reuse / Keep-Alive ✅

Goal: make `Client` genuinely stateful for sequential same-origin HTTP/1.1 traffic.

Completed:

- one retained eligible connection,
- effective host+port origin identity,
- reuse only after complete reusable responses,
- forced close for `Connection: close`, close-delimited, upgrade, and failed states,
- origin-change reconnect,
- no hidden automatic retry after stale keep-alive failure,
- move-only client ownership.

Historical reference:

- PR #20 — HTTP/1.1 connection reuse

Exit criteria: **complete**.

---

# v0.7 — Redirect Handling ✅

Goal: implement bounded redirects without expanding beyond plaintext HTTP.

Completed:

- configurable finite redirect following,
- 301 / 302 / 303 / 307 / 308 handling,
- method/body rewrite policy,
- absolute and relative Location resolution,
- same-origin reuse and cross-origin reconnect,
- sensitive header stripping across origins,
- unsupported redirect-scheme rejection,
- redirect-specific structured errors.

Historical reference:

- PR #21 — bounded HTTP redirect handling

Exit criteria: **complete**.

---

# v0.8 — Protocol and API Correctness Hardening ✅

Goal: close correctness and bounded-resource gaps before presenting the API as release-candidate quality.

## Result hardening

Completed:

- explicit variant-index state access,
- no hidden `bad_variant_access` from noexcept accessors,
- explicit success/failure factories,
- unambiguous `Result<Error>`,
- move-only payload support.

Reference: PR #22.

## URL and query correctness

Completed:

- owned appended query parameters,
- insertion-order/repeated-key preservation,
- percent encoding,
- raw-query preservation,
- percent-escape validation,
- ASCII-only scheme handling,
- stronger IPv6 literal validation.

Reference: PR #23.

## HTTP framing correctness

Completed:

- safe framing precedence,
- Transfer-Encoding + Content-Length conflict rejection,
- 1xx/204/205/304/HEAD framing review,
- strict chunk-extension and trailer validation,
- correct empty-body request Content-Length behavior,
- explicit timeout/EOF semantics,
- CI-exposed lifetime bug fixes.

Reference: PR #24.

## Response resource bounds

Completed:

- public `ResponseLimits`,
- default 64 KiB head limit,
- default 64 MiB decoded body limit,
- default 8 KiB chunk-line limit,
- default 64 KiB trailer limit,
- early Content-Length rejection,
- bounded close-delimited and chunked accumulation,
- `ResponseLimitExceeded`,
- zero treated as a real limit,
- deterministic boundary tests,
- failed limited responses never leave the connection reusable.

Reference: PR #25.

v0.8 exit criteria: **complete**.

---

# v0.9 — Packaging, Benchmarks, Examples, and Documentation ✅

Goal: make the library consumable, measurable, and understandable before the release gate.

## Packaging ✅

Completed:

- public header installation,
- library installation/export,
- `cpp_requestConfig.cmake` and version config,
- stable installed target `cpp_request::cpp_request`,
- install-tree consumer test,
- no GTest/Google Benchmark leakage into consumer package metadata.

Reference: PR #26.

## Benchmarks ✅

Executable benchmarks now cover:

- request serialization,
- response/header parsing,
- chunked decoding,
- end-to-end local loopback request overhead,
- connection reuse vs reconnect.

The benchmark suite is opt-in and has a dedicated Release smoke matrix across Windows, Linux, and macOS.

Reference: PR #27.

## Build-system integration ✅

Completed:

- modular target-scoped CMake configuration,
- project version aligned to v0.9.0,
- namespaced build helpers,
- centralized test/benchmark/example/install modules,
- clean install/export ownership,
- source-tree-safe configure behavior,
- development tooling cleanup.

Reference: PR #28.

## Examples and user documentation ✅

Completed by PR #29:

- root README,
- minimal GET example,
- POST/custom-header/query example,
- configured reusable Client example,
- timeout configuration,
- redirect configuration,
- response-limit configuration,
- structured error handling,
- explicit thread-safety statement,
- explicit HTTP-only / HTTPS-TLS exclusion,
- install/consumer instructions,
- getting-started guide aligned with the implemented public API.

Existing API-specific documentation remains authoritative for detailed contracts:

- `docs/api/public-api.md`,
- `docs/api/error-model.md`,
- `docs/api/result.md`,
- `docs/api/lifetime.md`,
- `docs/api/response-limits.md`.

v0.9 exit criteria: **complete when PR #29 merges**.

---

# v1.0 — Release Hardening and Acceptance Gate 🧪

Goal: verify the frozen requirements as one coherent release candidate.

Required verification:

- Windows Debug/Release CI green,
- Linux Debug/Release CI green,
- macOS Debug/Release CI green,
- C++17 build validated,
- all unit and loopback integration tests green,
- no required test depends on public internet access,
- sanitizer configuration reviewed and exercised where supported,
- deterministic socket cleanup verified on failure paths,
- connect/read/write timeout coverage present,
- connection reuse and redirect coverage present,
- install-tree consumer test green,
- benchmark targets build and run,
- examples build,
- public headers reviewed for internal/platform leakage,
- documentation checked against the frozen v1 scope,
- release versioning/tagging decision finalized.

## v1.0 Definition of Done

v1.0 is ready only when:

1. every **MUST** functional requirement is implemented or explicitly revised first,
2. every **MUST** non-functional requirement has a concrete verification path,
3. no known correctness bug can corrupt an HTTP message boundary or reuse an invalid connection,
4. expected URL/network/protocol/resource failures remain representable through structured errors,
5. the installed library has zero third-party runtime dependency,
6. public API and lifetime contracts match implementation,
7. benchmarks exist for any performance claims the project intends to make,
8. all release-gate CI and documentation checks pass.

---

# Next Step

```text
v0.9 examples + docs merge
    ↓
v1.0 integrated release hardening
    ↓
v1.0 release candidate
```

No new feature should enter the v1 release path unless a frozen requirement is explicitly changed.

---

# Frozen v1 Scope Boundary

The following remain intentionally outside v1.0:

- HTTPS / TLS
- asynchronous API
- coroutines
- HTTP/2
- HTTP/3
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

These features must not delay v1.0 unless the frozen requirements are explicitly revised.

---

# Post-v1 Direction ➡️

Potential future work includes:

- TLS/HTTPS transport abstraction,
- asynchronous/coroutine execution,
- streaming request/response bodies,
- generalized multi-origin connection pooling,
- proxy support,
- cookie management,
- compression/decompression,
- retry policies,
- HTTP/2 and later protocol exploration.

Every post-v1 item should receive its own requirements/design work before implementation.

---

# Roadmap Maintenance Rule

Update this roadmap whenever:

- a v1 MUST requirement changes,
- a milestone completes,
- implementation order materially changes,
- a newly discovered correctness blocker becomes release-critical,
- a feature moves into or out of v1 scope.
