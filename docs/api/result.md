# `Result<T>` Contract

## Status

- Project: `cpp_request`
- Target release: MVP v1.0
- Status: Frozen v1.0 contract
- Language baseline: C++17

## Purpose

`Result<T>` is the standard return type for fallible public operations in `cpp_request`.

It represents exactly one of two states:

- success containing `T`, or
- failure containing `Error`.

```mermaid
stateDiagram-v2
    [*] --> Success
    [*] --> Failure
    Success --> [*]
    Failure --> [*]
```

A `Result<T>` never represents both states simultaneously and does not require exception handling for expected network or protocol failures.

## Stable Public Interface

The stable v1 semantics are equivalent to:

```cpp
template <class T>
class [[nodiscard]] Result {
public:
    static Result success(T value);
    static Result failure(Error error);

    bool has_value() const noexcept;
    explicit operator bool() const noexcept;

    T& value() & noexcept;
    const T& value() const & noexcept;
    T&& value() && noexcept;

    Error& error() & noexcept;
    const Error& error() const & noexcept;
};
```

Construction details are implementation-specific, but the success/failure semantics above are frozen for v1.0.

For normal `T != Error` usage, direct construction from a success value or from `Error` is available. Explicit factories remain available when the desired state should be unambiguous.

## Explicit Factories

`success()` and `failure()` provide an unambiguous way to construct either state.

```cpp
auto ok = Result<int>::success(42);
auto failed = Result<int>::failure(Error{ErrorCode::ReadTimeout});
```

This is especially important for `Result<Error>`, where the success payload type and failure type are otherwise identical.

For `Result<Error>`:

- `Result<Error>::success(error_value)` creates a successful result whose value is an `Error` object,
- `Result<Error>::failure(error_value)` creates a failed result,
- direct construction from `Error` is treated as the failure form.

This keeps both states representable without introducing `ErrorCode::None` or another sentinel value.

## `[[nodiscard]]`

`Result<T>` is declared `[[nodiscard]]` so silently ignoring a potentially failed network operation produces a compiler diagnostic where supported.

Example:

```cpp
cpp_request::Client client;
client.get("http://example.com"); // should warn when result is discarded
```

## State Inspection

Two equivalent checks are supported:

```cpp
if (result.has_value()) {
    // success
}

if (result) {
    // success
}
```

`operator bool()` is `explicit` to avoid unintended implicit conversions.

## Value Access

`value()` is valid only when `has_value() == true`.

Calling the wrong-state accessor is a programmer error and violates a documented precondition.

The implementation does not rely on a hidden `std::bad_variant_access` path while declaring the accessor `noexcept`. A violated accessor precondition is outside normal runtime error handling and is not modeled as a network/protocol failure.

Recommended usage:

```cpp
auto result = client.get("http://example.com");
if (!result) {
    const auto& error = result.error();
    // handle error
    return;
}

const auto& response = result.value();
```

## Error Access

`error()` is valid only when `has_value() == false`.

Like `value()`, calling it in the opposite state violates the accessor precondition.

## Ownership

`Result<T>` owns whichever active state it contains.

Therefore:

- successful `Result<Response>` owns its `Response`,
- failed `Result<Response>` owns its `Error`,
- moving a result transfers its active state according to `T` / `Error` move semantics,
- copying is available only when the contained type permits it.

Move-only payloads remain usable as successful result values.

## Storage Representation

The physical storage is intentionally not part of the public API contract.

The v1 implementation uses a discriminated value/error representation without a separate heap allocation solely for the result state. Internal representation may change compatibly in later releases.

If the physical alternatives have identical types, state inspection and access use the result discriminator rather than type-based lookup.

## Allocation Policy

`Result<T>` itself does not require a separate heap allocation solely to store its success/error discriminator.

Any allocation performed by `T` remains a property of `T`, not of the result abstraction.

## `Result<void>`

`Result<void>` is not part of the v1.0 public surface. The primary template rejects `void` explicitly.

A future specialization may be introduced if a concrete API needs it, provided that addition preserves the stable v1 contract.

## No `ErrorCode::None` Requirement

Success is represented by the `Result` state itself, not by an `ErrorCode::None` sentinel.

```mermaid
flowchart LR
    R[Result<T>] -->|success| V[T]
    R -->|failure| E[Error]
```

There is no valid state where a failure contains a "no error" code.

## Error Propagation

Internal functions propagate failures without converting them to text and reparsing them later.

```mermaid
sequenceDiagram
    participant C as Client
    participant H as HTTP
    participant T as Transport
    participant P as Platform

    C->>H: execute request
    H->>T: write/read bytes
    T->>P: socket operation
    P-->>T: native failure
    T-->>H: Result<T> / Error
    H-->>C: propagate portable Error
    C-->>C: return Result<Response>
```

## Exception Boundary

`Result<T>` is responsible for expected operation failures, not catastrophic runtime conditions such as allocation failure.

The contract distinguishes:

- expected network/protocol failures → `Result<T>` failure,
- programmer contract violations → accessor precondition violation,
- unrelated standard-library/system failures → not redefined by this result model.

## Non-Goals

For v1, `Result<T>` does not provide a large functional-combinator API such as:

- `and_then`
- `transform`
- `or_else`
- monadic pipelines

These may be added later if real usage demonstrates value. The v1 API stays small and focused.
