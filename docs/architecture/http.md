# HTTP Layer Architecture

## Purpose

The HTTP layer implements HTTP/1.1 request serialization and response processing while remaining independent from native socket APIs.

## Request Pipeline

```text
Request data
   |
   v
Validate method/URL-derived target
   |
   v
Serialize request line
   |
   v
Serialize headers
   |
   v
Serialize header terminator
   |
   v
Append/body-write request payload
```

The layer should avoid unnecessary intermediate copies. `std::string_view` may be used for non-owning request inputs when lifetime is guaranteed by the synchronous call boundary.

## Request Serialization

The serializer is responsible for producing valid HTTP/1.1 wire format, including:

- method token
- request target
- `HTTP/1.1`
- header field lines
- terminating CRLF sequence
- request body bytes when present

Automatically generated headers such as `Host` and `Content-Length` may be added when required and not supplied explicitly, subject to the later public API contract.

## Response Processing

The response parser conceptually proceeds through states:

```text
StatusLine
   -> Headers
   -> BodyFramingDecision
   -> Body
   -> Complete
```

Malformed protocol data transitions to an error result instead of continuing with ambiguous state.

## Body Framing

The parser must support v1 framing rules for:

1. responses with no message body by HTTP semantics, including `HEAD`,
2. `Transfer-Encoding: chunked`,
3. `Content-Length`,
4. connection-close-delimited body where valid/required by HTTP/1.1 behavior.

Framing precedence must follow HTTP semantics; conflicting or invalid framing must not be guessed silently.

## Chunked Decoder

The chunked decoder owns protocol state for:

- reading hexadecimal chunk size
- consuming chunk extensions without exposing them as body content when present
- consuming each chunk payload
- validating CRLF framing
- recognizing terminal zero-size chunk
- consuming trailer section as required for correct message completion

Decoded body bytes are accumulated into the v1 in-memory response body.

## Header Handling

Headers should be parsed without requiring per-character heap allocation.

The architecture should permit:

- case-insensitive header-name lookup
- duplicate header fields where HTTP permits them
- preserving sufficient response data for user-facing access

Exact container representation is deferred to the public API and performance design phase.

## Connection Semantics

After a response is complete, the HTTP layer reports whether protocol semantics permit the underlying connection to remain reusable.

Reasons to reject reuse include:

- `Connection: close`
- incomplete or malformed response framing
- transport termination before required body completion
- any parser state that leaves unread bytes ambiguous

## Redirect Semantics

Redirect handling is orchestrated above the low-level parser but uses parsed status and `Location` metadata from this layer.

The v1 redirect policy must:

- respect the configured enable/disable option
- enforce a finite redirect count
- resolve relative locations
- reject redirects requiring unsupported HTTPS transport

Method rewriting/preservation rules for redirect status codes must be specified before implementation of redirect execution.

## Parser Design Principle

The parser should support incremental input from `read_some()` rather than assuming one receive call contains a complete status line, header block, chunk, or body.

This is required for correctness over TCP and also enables focused parser benchmarks independent of live network latency.