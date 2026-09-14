# Non-Functional Requirements

## Document Status

- Project: `cpp_request`
- Target release: MVP v1.0
- Status: Frozen baseline
- Design emphasis: lightweight, fast, portable, predictable

## Requirement Conventions

Each requirement has a stable identifier so it can be referenced by benchmarks, CI, implementation tasks, and architecture decisions.

Priority meanings:

- **MUST**: required for v1.0 acceptance.
- **SHOULD**: expected unless a documented implementation constraint prevents it.

---

## 1. Language and Compatibility

### REQ-COMP-001 — Minimum C++17
**Priority:** MUST

The public library interface and implementation shall compile with C++17 as the minimum language standard.

**Rationale:**
`cpp_request` relies heavily on non-owning string access for URL processing, request construction, header handling, and HTTP parsing. Using the standard `std::string_view` keeps these operations lightweight while avoiding the development and maintenance cost of a custom compatibility implementation. C++17 therefore provides a better balance between portability, performance, and implementation velocity than a C++11 baseline.

**Acceptance criteria:**
- The project can be configured with a compiler mode equivalent to C++17.
- Public headers may use `std::string_view`.
- No project-specific `string_view` compatibility layer is required.

### REQ-COMP-002 — Newer standard compatibility
**Priority:** SHOULD

The library should remain usable from applications compiled with newer C++ language standards.

**Acceptance criteria:**
- Supported CI jobs include at least one configuration newer than C++17 where practical.

---

## 2. Dependency Policy

### REQ-DEP-001 — Zero third-party runtime dependency
**Priority:** MUST

The core v1.0 library shall not depend on third-party runtime networking or HTTP libraries.

**Acceptance criteria:**
- HTTP and networking behavior does not depend on libcurl, Boost.Asio, standalone Asio, or equivalent HTTP/network backends.
- Platform networking uses native OS facilities.

### REQ-DEP-002 — Development-only dependencies allowed
**Priority:** SHOULD

Third-party dependencies may be used for testing, benchmarking, or tooling if they are not required by consumers of the core library.

**Acceptance criteria:**
- Consumer linkage does not require development-only packages.

---

## 3. Performance and Resource Efficiency

### REQ-PERF-001 — Minimize unnecessary allocation
**Priority:** MUST

The implementation shall avoid unnecessary heap allocations in request construction, response parsing, and connection management.

**Acceptance criteria:**
- No mandatory PImpl allocation exists for core public types.
- Hot-path helpers do not allocate solely for abstraction purposes.
- Allocation behavior is benchmarkable or inspectable in critical components.

### REQ-PERF-002 — Minimize unnecessary copying
**Priority:** MUST

The implementation shall avoid avoidable data copies where ownership and lifetime permit safe reuse.

**Acceptance criteria:**
- Request serialization and response processing do not duplicate full payload buffers without a functional reason.
- `std::string_view` is preferred for read-only non-owning string access where lifetime is well-defined.

### REQ-PERF-003 — Minimize abstraction overhead
**Priority:** MUST

Internal abstractions shall not introduce avoidable runtime indirection in hot paths.

**Acceptance criteria:**
- No design requires virtual dispatch for fundamental request parsing or socket I/O paths unless justified by a documented architecture decision.

### REQ-PERF-004 — Connection reuse
**Priority:** MUST

The implementation shall reuse eligible HTTP/1.1 connections to avoid unnecessary connection establishment overhead.

**Acceptance criteria:**
- Repeated sequential requests to a compatible endpoint can avoid reconnecting when reuse is valid.

### REQ-PERF-005 — Benchmarkable critical components
**Priority:** MUST

Performance-critical components shall be independently benchmarkable.

**Acceptance criteria:**
- Benchmark targets can measure at least request serialization, response/header parsing, and chunked transfer decoding.
- End-to-end network benchmarks should prefer loopback/local servers when measuring library overhead.

---

## 4. Portability

### REQ-PORT-001 — Windows support
**Priority:** MUST

The library shall support Windows using native Winsock-compatible networking facilities.

**Acceptance criteria:**
- The project builds and runs networking tests on a supported Windows CI environment.

### REQ-PORT-002 — Linux support
**Priority:** MUST

The library shall support Linux using POSIX/BSD socket facilities.

**Acceptance criteria:**
- The project builds and runs networking tests on a supported Linux CI environment.

### REQ-PORT-003 — macOS/POSIX design compatibility
**Priority:** SHOULD

The POSIX networking abstraction should remain compatible with macOS where practical.

**Acceptance criteria:**
- Platform abstractions do not intentionally rely on Linux-only behavior without isolation or documentation.

### REQ-PORT-004 — Platform details isolated
**Priority:** MUST

Platform-specific networking code shall be isolated from HTTP protocol logic.

**Acceptance criteria:**
- HTTP serialization/parsing code does not directly depend on Winsock or POSIX-specific types.

---

## 5. Reliability and Resource Safety

### REQ-REL-001 — Deterministic socket cleanup
**Priority:** MUST

Native socket resources shall be released deterministically using RAII-compatible ownership.

**Acceptance criteria:**
- Successful and failed request paths release abandoned connections correctly.
- Socket handles are not leaked when construction or connection fails.

### REQ-REL-002 — Predictable failure reporting
**Priority:** MUST

Expected networking and protocol failures shall be represented through structured result/error values.

**Acceptance criteria:**
- Callers do not need to parse diagnostic strings to identify major failure classes.

### REQ-REL-003 — No silent protocol downgrade
**Priority:** MUST

Unsupported transport or protocol behavior shall fail explicitly.

**Acceptance criteria:**
- HTTPS requests are rejected rather than converted to HTTP.
- Unsupported response framing or malformed protocol data produces an error rather than undefined behavior.

### REQ-REL-004 — Bounded configured waits
**Priority:** MUST

Configured connect, read, and write timeouts shall prevent indefinite blocking in their respective operations.

**Acceptance criteria:**
- Timeout tests can demonstrate bounded failure for each configured timeout class.

---

## 6. API Design

### REQ-APIQ-001 — Small public surface
**Priority:** MUST

The v1.0 public API shall expose only abstractions required to perform HTTP client operations.

**Acceptance criteria:**
- Native socket implementation types are not part of the supported public contract.
- Internal parser and transport helper types are not unnecessarily exported.

### REQ-APIQ-002 — Value-oriented lightweight types
**Priority:** SHOULD

Small public types should prefer direct value semantics over heap-backed indirection where practical.

**Acceptance criteria:**
- Small types such as errors, URL components, and result metadata do not require per-instance heap allocation solely for encapsulation.

### REQ-APIQ-003 — Move support
**Priority:** MUST

Resource-owning types shall support efficient transfer of ownership using move semantics where appropriate.

**Acceptance criteria:**
- Moving a resource-owning object does not duplicate the underlying native socket ownership.

### REQ-APIQ-004 — Thread-safety documentation
**Priority:** MUST

The library shall explicitly document that concurrent access to the same `Client` instance is not guaranteed to be thread-safe in v1.0.

**Acceptance criteria:**
- User-facing documentation includes the thread-safety contract.

---

## 7. Build and Packaging

### REQ-BUILD-001 — CMake build system
**Priority:** MUST

The project shall use CMake as its supported build-system entry point.

**Acceptance criteria:**
- The core library, tests, examples, and benchmarks can be configured through CMake targets/options.

### REQ-BUILD-002 — Installable package
**Priority:** MUST

The library shall provide an installable CMake package configuration for consumers.

**Acceptance criteria:**
- A consumer can use a documented `find_package(cpp_request ...)` workflow after installation.
- An exported target such as `cpp_request::cpp_request` is available.

### REQ-BUILD-003 — Consumer does not inherit test tooling
**Priority:** MUST

Testing and benchmarking dependencies shall not leak into the installed core library interface.

**Acceptance criteria:**
- Building a consumer against the installed library does not require test or benchmark frameworks.

---

## 8. Verification

### REQ-TEST-001 — Unit testing
**Priority:** MUST

Pure protocol and utility components shall have unit coverage.

**Acceptance criteria:**
- Request serialization, URL handling, header parsing, response framing, and chunked decoding are testable without relying on the public internet.

### REQ-TEST-002 — Integration testing
**Priority:** MUST

Networking behavior shall have integration tests against controlled endpoints.

**Acceptance criteria:**
- Tests cover successful connection, request/response exchange, timeout paths, redirects, and connection reuse where practical.

### REQ-TEST-003 — No public internet dependency for required CI
**Priority:** SHOULD

Required CI tests should avoid depending on external public services.

**Acceptance criteria:**
- Core integration tests can run against local or controlled test servers.

### REQ-TEST-004 — Cross-platform CI
**Priority:** MUST

CI shall validate the supported primary platforms.

**Acceptance criteria:**
- At minimum, Windows and Linux builds are exercised in CI.
- C++17 compatibility is represented by at least one required build configuration.

---

## 9. Documentation

### REQ-DOC-001 — Public API documentation
**Priority:** MUST

Supported public types and operations shall be documented.

**Acceptance criteria:**
- Public request execution, configuration, response access, and error handling have user-facing documentation or examples.

### REQ-DOC-002 — Scope documentation
**Priority:** MUST

The v1.0 supported and unsupported feature set shall be documented explicitly.

**Acceptance criteria:**
- HTTPS/TLS, async/coroutines, streaming, HTTP/2+, and other frozen exclusions are clearly marked as out of scope for v1.0.

### REQ-DOC-003 — Architecture boundary documentation
**Priority:** SHOULD

The repository should document the separation between public API, HTTP protocol logic, transport, and platform-specific socket code.

**Acceptance criteria:**
- Future contributors can identify which layer owns protocol logic versus operating-system networking logic.

---

## 10. v1.0 Quality Principles

The following principles guide implementation choices when multiple technically valid designs exist:

1. **Lightweight** — avoid unnecessary dependencies, allocations, copies, and hidden machinery.
2. **Fast** — optimize measurable hot paths and validate claims with benchmarks.
3. **Portable** — isolate platform-specific networking details and preserve C++17 compatibility.
4. **Predictable** — prefer explicit ownership, explicit errors, and bounded configured waits.
5. **Focused** — do not expand v1.0 with post-MVP features unless required for HTTP/1.1 correctness or the frozen acceptance criteria.
