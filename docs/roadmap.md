# cpp_request v1 Roadmap

## Status

- Project: `cpp_request`
- Target release: MVP v1.0
- Roadmap status: Active planning baseline
- Language baseline: C++17
- Protocol scope: synchronous HTTP/1.1 over plaintext TCP
- Last roadmap review: 2026-09-15

This document is the implementation-order source of truth for the v1 release.

The frozen functional and non-functional requirements remain authoritative for **what** v1 must provide. This roadmap defines **when** those requirements are expected to be implemented and which milestones must be complete before v1.0 can be released.

Version labels below are development milestones. A milestone does not require a public release tag unless the project explicitly decides to publish one.

---

## Status Legend

- ✅ **Complete** — merged into `main` and covered by the expected tests for that milestone.
- 🚧 **In progress** — implementation exists in an open PR or active branch.
- ⏳ **Planned** — required for v1 but implementation has not started.
- 🧪 **Release gate** — verification/hardening work required before v1.0.
- ➡️ **Post-v1** — intentionally outside the frozen v1 scope.

---

## Roadmap Overview

| Milestone | Focus | Status |
| --- | --- | --- |
| v0.1 | Requirements, architecture, API/error contracts | ✅ Complete |
| v0.2 | URL, platform sockets, DNS, TCP transport | ✅ Complete |
| v0.3 | HTTP request data model and serialization | ✅ Complete |
| v0.4 | HTTP response parsing and body framing | ✅ Complete |
| v0.5 | Public `Client` end-to-end execution path | 🚧 In progress — PR #15 |
| v0.6 | HTTP/1.1 connection reuse / keep-alive | ⏳ Planned |
| v0.7 | Redirect handling | ⏳ Planned |
| v0.8 | Protocol/API correctness hardening | ⏳ Planned |
| v0.9 | Packaging, benchmarks, examples, documentation | ⏳ Planned |
| v1.0 | Release hardening and acceptance gate | 🧪 Planned |

---

# v0.1 — Specification and Architecture Baseline ✅

Goal: freeze the v1 product boundary before implementation expands.

Completed work:

- v1 functional requirements frozen.
- v1 non-functional requirements frozen.
- architecture layers defined:
  - Public API
  - HTTP
  - Transport
  - Platform
  - OS
- public API baseline defined.
- lifetime and ownership contracts defined.
- structured `Error` taxonomy defined.
- project-owned `Result<T>` contract defined.
- C++17 minimum frozen.
- no PImpl for v1 frozen.
- HTTPS/TLS explicitly excluded from v1.
- synchronous-only v1 execution model frozen.

Historical implementation references:

- PR #2 — frozen v1 requirements
- PR #3 — architecture boundaries
- PR #4 — public API contract
- PR #5 — error and `Result<T>` contracts

Exit criteria: **complete**.

---

# v0.2 — Native Transport Foundation ✅

Goal: establish a portable, bounded, RAII-owned TCP byte stream without exposing platform sockets publicly.

Completed work:

- URL parsing and HTTP-only scheme validation.
- IPv4 and IPv6 endpoint representation.
- Winsock/POSIX native socket RAII wrapper.
- network runtime initialization.
- OS-backed DNS resolution.
- timed TCP connection establishment.
- endpoint fallback.
- timed `write_all()`.
- timed `read_some()`.
- partial I/O handling.
- connection-close/error normalization.
- SIGPIPE-safe POSIX write behavior.
- Windows/Linux/macOS CI coverage.

Historical implementation references:

- PR #6 — URL parser
- PR #7 — native socket RAII
- PR #8 — DNS resolver and endpoints
- PR #9 — timed TCP connect
- PR #10 — timed TCP read/write

Exit criteria: **complete**.

---

# v0.3 — HTTP Request Model and Serialization ✅

Goal: represent and serialize all v1 request methods without unnecessary payload copies.

Completed work:

- `Headers` owning ordered storage.
- case-insensitive header lookup.
- duplicate request-header support.
- `Request` with borrowed URL/body views.
- GET, HEAD, POST, PUT, PATCH, DELETE method model.
- HTTP/1.1 request-line serialization.
- automatic `Host` generation.
- IPv6 host formatting.
- automatic `Content-Length` generation.
- caller-provided `Content-Length` validation.
- outgoing header syntax validation.
- CR/LF request-injection rejection.
- request-body zero-copy serialization boundary.

Historical implementation references:

- PR #11 — `Headers` and `Request`
- PR #12 — request serializer

Exit criteria: **complete**.

---

# v0.4 — HTTP Response Parser and Framing ✅

Goal: incrementally parse HTTP/1.1 responses independently of TCP packet boundaries.

Completed work:

- public owning `Response`.
- incremental status-line/header parsing.
- interim `1xx` handling.
- HEAD/no-body semantics.
- `Content-Length` body framing.
- close-delimited body framing.
- chunked transfer decoding.
- chunk extensions.
- trailer consumption/validation.
- premature EOF detection.
- conflicting framing rejection.
- `Connection: close` detection.
- protocol-upgrade non-reuse semantics.
- preservation of bytes belonging to a following response.

Historical implementation references:

- PR #13 — incremental response parser
- PR #14 — chunked transfer decoder

Exit criteria: **complete**.

---

# v0.5 — Public Client Core 🚧

Goal: connect all merged layers into one usable synchronous HTTP request path.

Current implementation target:

1. serialize `Request`,
2. resolve host,
3. connect TCP,
4. write request head,
5. write borrowed request body,
6. read response bytes,
7. feed incremental parser,
8. return owning `Response`.

Required public API for this milestone:

- `Client::request(const Request&)`
- `Client::get()`
- `Client::head()`
- `Client::post()`
- `Client::put()`
- `Client::patch()`
- `Client::del()`
- equivalent one-shot free helpers
- connect/read/write timeout setters

Verification target:

- controlled loopback GET.
- POST body transmission.
- custom headers.
- Content-Length response.
- chunked response.
- close-delimited response.
- malformed response propagation.
- HTTPS rejection before networking.

Current work:

- PR #15 — public `Client` core execution path.

Important limitation for v0.5:

- each request may still establish a fresh TCP connection.
- redirect handling is not part of this milestone.

Exit criteria:

- PR #15 merged.
- all supported CI matrices pass.
- end-to-end loopback tests pass without public internet access.

---

# v0.6 — Connection Reuse / Keep-Alive ⏳

Goal: make `Client` genuinely stateful and satisfy HTTP/1.1 connection-reuse requirements.

Requirements covered:

- REQ-CONN-002
- REQ-CONN-003
- REQ-PERF-004
- REQ-API-001 reusable state
- REQ-TEST-002 reuse integration coverage

Planned work:

- retain one eligible TCP connection inside `Client`.
- define connection identity at minimum by effective host + port.
- reuse only when the previous response is fully consumed and parser marks the connection reusable.
- close retained connection when response includes `Connection: close`.
- never reuse close-delimited connections.
- never reuse protocol-upgraded connections.
- discard retained connection after timeout, transport error, protocol error, or partial write.
- reconnect automatically when the next request targets a different endpoint.
- handle stale peer-closed keep-alive connections safely.
- preserve parser pending-byte invariant.

Required tests:

- two sequential requests over one accepted socket.
- `Connection: close` forces reconnect.
- close-delimited response forces reconnect.
- host/port change forces reconnect.
- failed connection is never retained.
- stale keep-alive socket does not corrupt a subsequent request.

Exit criteria:

- eligible sequential requests demonstrably avoid a second TCP handshake.
- no invalid/stale connection is reused.

---

# v0.7 — Redirect Handling ⏳

Goal: implement bounded automatic redirects without expanding beyond plaintext HTTP.

Requirements covered:

- REQ-REDIR-001
- REQ-REDIR-002
- REQ-REDIR-003

Planned public configuration:

- `Client::set_follow_redirects(bool)`
- `Client::set_max_redirects(std::size_t)`

Planned behavior:

- finite default redirect limit.
- follow supported HTTP redirects when enabled.
- resolve relative `Location` values against the current URL.
- reject missing/invalid `Location` where redirect following is required.
- reject redirect targets requiring unsupported HTTPS/TLS.
- return `RedirectLimitExceeded` after the configured maximum.
- preserve method/body semantics according to the selected redirect status rules.

Redirect-method policy must be explicitly documented and tested for:

- 301
- 302
- 303
- 307
- 308

Required tests:

- absolute redirect.
- relative redirect.
- redirect chain.
- redirect loop/limit.
- redirect following disabled.
- redirect to HTTPS.
- redirect across host/port with correct connection replacement.

Exit criteria:

- all frozen redirect requirements have deterministic loopback tests.

---

# v0.8 — Protocol and API Correctness Hardening ⏳

Goal: close known correctness gaps before treating the API as release-candidate quality.

## URL and query correctness

REQ-HTTP-004 requires query-parameter behavior beyond merely preserving an already-encoded raw query.

Planned work:

- define the v1 query-parameter API or explicitly refine the requirement if raw URL input is the intended contract.
- percent-encode reserved characters where required.
- add query/encoding tests.
- review percent-escape validation in URL parsing.
- strengthen bracketed IPv6 literal validation if necessary.

## `Result<T>` hardening

Known implementation review items:

- eliminate wrong-state `std::get` behavior hidden behind `noexcept`.
- avoid termination hazards in release builds when preconditions are violated.
- make representation robust for edge cases such as `T == Error` if that instantiation is intended to be supported.
- benchmark `std::variant` representation before considering manual storage.
- decide whether `Result<void>` is needed by the final v1 public/internal surface.

## HTTP hardening

Planned review:

- response framing precedence and duplicate framing fields.
- status/no-body edge cases.
- header syntax edge cases.
- chunk extension/trailer correctness.
- request serializer Host/Content-Length policy.
- timeout boundary semantics.
- EOF classification.

## Resource-bound review

Because response bodies are memory-resident in v1, document or introduce practical limits where necessary for defensive behavior:

- response-head growth.
- pathological chunk-size lines.
- excessive trailer/header sections.
- body-size expectations.

This milestone must not add streaming; it only hardens the frozen in-memory design.

Exit criteria:

- no known correctness blocker remains for the frozen v1 scope.
- all accepted hardening decisions are reflected in docs/tests.

---

# v0.9 — Packaging, Benchmarks, Examples, and Documentation ⏳

Goal: make the library consumable and make performance claims measurable.

## Packaging

Requirements covered:

- REQ-BUILD-001
- REQ-BUILD-002
- REQ-BUILD-003

Planned work:

- install public headers.
- install/export the core library target.
- provide `cpp_requestConfig.cmake` / version config as appropriate.
- expose a stable consumer target such as `cpp_request::cpp_request`.
- add an install-tree consumer test.
- ensure test/benchmark dependencies do not leak to consumers.

## Benchmarks

Requirement covered:

- REQ-PERF-005

Required benchmark targets:

- request serialization.
- response/header parsing.
- chunked decoding.
- end-to-end loopback request overhead.
- connection reuse vs reconnect comparison.

Performance work should follow measurement; do not add complexity solely on intuition.

## Examples and user documentation

Requirements covered:

- REQ-DOC-001
- REQ-DOC-002
- REQ-DOC-003
- REQ-APIQ-004

Planned work:

- minimal GET example.
- POST body example.
- custom header example.
- timeout configuration example.
- redirect configuration example.
- reusable `Client` example.
- error-handling example.
- explicit thread-safety statement.
- explicit HTTPS/TLS exclusion.
- public API reference synchronized with implementation.

## Project presentation

Before v1.0, add a root README containing at minimum:

- project purpose.
- supported platforms.
- C++17 requirement.
- build/install instructions.
- quick-start example.
- supported v1 features.
- explicit non-goals.
- link to this roadmap.

Exit criteria:

- a clean consumer can install and use the library.
- critical components have executable benchmarks.
- user-facing documentation matches the actual API.

---

# v1.0 — Release Hardening and Acceptance Gate 🧪

Goal: verify the frozen requirements as a coherent release rather than as isolated merged features.

## Required verification

- Windows CI green.
- Linux CI green.
- macOS compatibility green where maintained.
- C++17 build validated.
- required unit tests green.
- required loopback integration tests green.
- no required test depends on the public internet.
- sanitizer configuration reviewed and exercised where supported.
- deterministic socket cleanup verified on error paths.
- connect/read/write timeout tests present.
- connection reuse test present.
- redirect tests present.
- install-tree consumer test present.
- benchmark targets build and run.
- public headers reviewed for accidental internal/platform leakage.
- documentation reviewed against frozen v1 scope.

## v1.0 Definition of Done

v1.0 is ready only when all of the following are true:

1. every **MUST** functional requirement is implemented or an explicit requirements change has been accepted first,
2. every **MUST** non-functional requirement has a concrete verification path,
3. no known correctness bug can corrupt an HTTP message boundary or reuse an invalid connection,
4. expected network/protocol failures remain representable through structured errors,
5. the installed library has zero third-party runtime dependency,
6. the public API and lifetime contracts are documented and match the implementation,
7. benchmarks exist for the performance claims the project intends to make.

---

# Dependency Order From Current State

The implementation order after the current client-core work is:

```text
Client core
    ↓
Connection reuse / keep-alive
    ↓
Redirect handling
    ↓
Protocol + API correctness hardening
    ↓
Packaging + benchmarks + examples + docs
    ↓
Release acceptance / v1.0
```

Why this order:

- redirects depend on a correct request execution path and interact with connection ownership,
- connection reuse changes `Client` state/lifetime and should stabilize before redirects build on it,
- benchmarks are more meaningful after the core behavior is complete,
- packaging and public documentation should describe the API that will actually ship,
- release hardening should verify the final integrated system rather than unfinished intermediate layers.

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

Potential future work, not ordered or committed yet:

- TLS/HTTPS transport abstraction.
- asynchronous/coroutine execution API.
- streaming request/response bodies.
- generalized multi-origin connection pool.
- proxy support.
- cookie management.
- compression/decompression.
- retry policies.
- HTTP/2 and later protocol exploration.

A post-v1 item should receive its own requirements/design work before implementation begins.

---

# Roadmap Maintenance Rule

This roadmap must be updated when any of the following occurs:

- a v1 MUST requirement changes,
- a milestone is completed,
- implementation order changes materially,
- a newly discovered correctness blocker becomes release-critical,
- a feature is moved into or out of v1 scope.

Normal implementation-detail changes do not require a roadmap edit.
