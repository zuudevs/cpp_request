# Public API Contract

## Status

- Project: `cpp_request`
- Target release: MVP v1.0
- Status: Proposed API baseline
- Language baseline: C++17
- Namespace: `cpp_request`

This document defines the intended public API shape for v1.0. Internal implementation details remain free to change as long as the documented behavior is preserved.

## Design Principles

The public API should be:

1. small,
2. explicit,
3. lightweight,
4. synchronous,
5. non-exception-oriented for expected failures,
6. efficient for string-heavy HTTP processing.

`std::string_view` is preferred for non-owning textual input where the synchronous lifetime boundary makes borrowing safe and obvious.

---

## Core Public Types

The v1 public surface consists primarily of:

- `Client`
- `Request`
- `Response`
- `Headers`
- `Url`
- `Error`
- `Result<T>`

Supporting configuration types may be introduced when they materially improve clarity, but the v1 API should avoid unnecessary wrappers.

```mermaid
classDiagram
    class Client
    class Request
    class Response
    class Headers
    class Url
    class Error
    class Result~T~

    Client --> Request : executes
    Client --> Response : returns
    Request --> Headers : contains
    Request --> Url : targets
    Response --> Headers : contains
    Result~T~ --> Error : failure
```

---

## `Client`

`Client` is the primary stateful request executor.

Responsibilities:

- retain reusable connection state,
- retain client-level timeout configuration,
- retain redirect configuration,
- execute sequential HTTP requests,
- expose convenience member functions for common methods.

Conceptual interface:

```cpp
namespace cpp_request {

class Client {
public:
    Client();

    Result<Response> request(const Request& request);

    Result<Response> get(std::string_view url);
    Result<Response> head(std::string_view url);
    Result<Response> post(std::string_view url, std::string_view body = {});
    Result<Response> put(std::string_view url, std::string_view body = {});
    Result<Response> patch(std::string_view url, std::string_view body = {});
    Result<Response> del(std::string_view url);

    void set_connect_timeout(std::chrono::milliseconds timeout);
    void set_read_timeout(std::chrono::milliseconds timeout);
    void set_write_timeout(std::chrono::milliseconds timeout);

    void set_follow_redirects(bool enabled);
    void set_max_redirects(std::size_t count);
};

} // namespace cpp_request
```

Exact overload count may change before implementation, but the behavioral contract above is the v1 target.

### Thread safety

A single `Client` instance is **not** guaranteed to be safe for concurrent use from multiple threads in v1.0.

Independent `Client` instances may be used from different threads, subject to normal platform constraints.

---

## `Request`

`Request` represents a complete logical HTTP request description before execution.

Conceptual interface:

```cpp
namespace cpp_request {

enum class Method {
    Get,
    Head,
    Post,
    Put,
    Patch,
    Delete
};

class Request {
public:
    Request(Method method, std::string_view url);

    Method method() const noexcept;
    std::string_view url() const noexcept;

    Headers& headers() noexcept;
    const Headers& headers() const noexcept;

    void set_body(std::string_view body) noexcept;
    std::string_view body() const noexcept;
};

} // namespace cpp_request
```

### Borrowed request data

`Request` may hold non-owning views for URL and body data. Their lifetime requirements are defined in `lifetime.md`.

The v1 design does not require request-body ownership or request streaming.

---

## `Response`

`Response` represents a completed HTTP response.

The response owns its body because v1 only exposes completed in-memory responses.

Conceptual interface:

```cpp
namespace cpp_request {

class Response {
public:
    int status_code() const noexcept;
    std::string_view reason() const noexcept;

    const Headers& headers() const noexcept;

    std::string_view body() const noexcept;
    const std::string& body_storage() const noexcept;
};

} // namespace cpp_request
```

The exact body accessor names are not frozen by this document, but the following behavior is:

- completed response body is owned by `Response`,
- callers can access it without copying,
- destroying or moving-from the owning `Response` invalidates views into its owned storage.

---

## `Headers`

`Headers` represents HTTP header fields while preserving duplicate fields where needed.

Required behavior:

- insertion of multiple fields,
- case-insensitive lookup by field name,
- preservation of duplicate field values,
- efficient iteration,
- no requirement to canonicalize original field-name casing.

Conceptual API:

```cpp
namespace cpp_request {

class Headers {
public:
    void add(std::string_view name, std::string_view value);
    void set(std::string_view name, std::string_view value);
    bool contains(std::string_view name) const noexcept;

    std::string_view get(std::string_view name) const noexcept;
};

} // namespace cpp_request
```

### Lookup semantics

For v1:

- `get(name)` returns the first matching field value,
- absence is represented by an empty view,
- APIs for enumerating all duplicate values may be added if required by implementation/tests without breaking this base contract.

---

## `Url`

`Url` represents a parsed HTTP URL.

Required observable components:

- scheme,
- host,
- optional explicit port,
- effective port,
- path,
- query,
- request target.

Conceptual interface:

```cpp
namespace cpp_request {

class Url {
public:
    static Result<Url> parse(std::string_view input);

    std::string_view scheme() const noexcept;
    std::string_view host() const noexcept;
    std::string_view path() const noexcept;
    std::string_view query() const noexcept;

    std::uint16_t port() const noexcept;
    std::string_view target() const noexcept;
};

} // namespace cpp_request
```

For v1, only `http://` is supported. `https://` must fail explicitly.

Implementation may own normalized URL storage internally when necessary; callers must not depend on the exact storage representation.

---

## `Result<T>`

Expected runtime failures are returned through `Result<T>` rather than requiring exceptions.

Required semantic states:

- success with `T`,
- failure with `Error`.

Conceptual interface:

```cpp
namespace cpp_request {

template <class T>
class Result {
public:
    bool has_value() const noexcept;
    explicit operator bool() const noexcept;

    T& value();
    const T& value() const;

    Error error() const noexcept;
};

} // namespace cpp_request
```

The internal representation is intentionally not frozen here.

`Result<T>` must not require heap allocation solely to represent success/failure state when avoidable.

---

## `Error`

`Error` is a library-owned structured error value.

The exact taxonomy is intentionally deferred to the dedicated error-model document.

The public contract requires that callers can distinguish major failure classes without parsing diagnostic strings.

Expected categories include:

- invalid URL,
- unsupported scheme,
- name resolution failure,
- connection failure,
- timeout,
- send failure,
- receive failure,
- malformed HTTP response,
- redirect failure.

---

## Free Convenience Functions

The library should expose stateless convenience helpers for simple one-shot requests.

Conceptually:

```cpp
namespace cpp_request {

Result<Response> get(std::string_view url);
Result<Response> head(std::string_view url);
Result<Response> post(std::string_view url, std::string_view body = {});
Result<Response> put(std::string_view url, std::string_view body = {});
Result<Response> patch(std::string_view url, std::string_view body = {});
Result<Response> del(std::string_view url);

} // namespace cpp_request
```

These helpers may internally create a temporary `Client` and therefore do not promise connection reuse across separate calls.

---

## Request Execution Flow

```mermaid
sequenceDiagram
    participant App as Application
    participant C as Client
    participant U as URL Parser
    participant H as HTTP Layer
    participant T as Transport

    App->>C: get(url)
    C->>U: parse(url)
    U-->>C: Result<Url>
    C->>H: build request
    H-->>C: serialized request
    C->>T: execute byte I/O
    T-->>C: response bytes
    C->>H: parse response
    H-->>C: Response metadata/body
    C-->>App: Result<Response>
```

---

## API Naming Rules

For v1 public API:

- use `snake_case` for functions,
- use PascalCase for public type names,
- avoid abbreviations unless standard in HTTP/networking vocabulary,
- avoid platform-specific terminology,
- avoid exposing internal parser/transport types.

`delete` is a C++ keyword, so the convenience member/free function uses `del()` unless a better non-keyword name is selected before freeze.

---

## Explicit v1 API Non-Goals

The public API will not include dedicated abstractions for:

- TLS configuration,
- async handles/futures,
- coroutines,
- streaming bodies,
- HTTP/2 streams,
- proxy configuration,
- cookie jars,
- retry policies,
- cache policies,
- multipart builders.

These features belong to post-v1 design work.
