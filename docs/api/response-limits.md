# Response Resource Limits

## Purpose

`cpp_request` v1 stores completed response bodies in memory. To keep that design predictable under malformed or hostile peers, response parsing has finite resource limits.

Limits are client configuration, not HTTP syntax rules. Exceeding a configured limit returns `ErrorCode::ResponseLimitExceeded` and the connection is not retained for reuse.

## Public configuration

```cpp
cpp_request::ResponseLimits limits;
limits.max_head_bytes = 64 * 1024;
limits.max_body_bytes = 64 * 1024 * 1024;
limits.max_chunk_line_bytes = 8 * 1024;
limits.max_trailer_bytes = 64 * 1024;

cpp_request::Client client;
client.set_response_limits(limits);
```

`Client::response_limits()` returns the currently configured values.

## Default limits

| Resource | Default |
| --- | ---: |
| Response head, including terminating CRLF CRLF | 64 KiB |
| Decoded response body | 64 MiB |
| One chunk-size line, excluding CRLF | 8 KiB |
| Complete chunked trailer section, including CRLF delimiters | 64 KiB |

The defaults are finite product safeguards rather than protocol maxima. Callers that intentionally accept larger responses can replace them with larger values before issuing a request.

## Enforcement semantics

### Response head

The parser rejects a response once the current status/header block cannot fit within `max_head_bytes`. The check happens before header parsing and before unbounded header storage growth.

Interim responses are checked independently per response head.

### Content-Length body

If a valid `Content-Length` exceeds `max_body_bytes`, parsing fails before reserving the declared body capacity.

A body exactly equal to the configured limit is accepted.

### Close-delimited body

Before appending newly received bytes, the parser verifies that the accumulated body plus the new bytes remain within `max_body_bytes`.

### Chunked body

Each parsed chunk size is checked against the remaining decoded-body budget before chunk payload bytes are appended.

`max_chunk_line_bytes` prevents an unterminated or pathologically long chunk-size/extension line from growing indefinitely.

`max_trailer_bytes` bounds the full trailer section, including each CRLF and the terminal empty line.

## Zero values

Zero is a real limit, not a synonym for unlimited. For example, `max_body_bytes = 0` accepts only responses whose decoded body is empty.

A caller that wants an effectively unbounded field can explicitly set that field to a suitably large `std::size_t` value. The library does not provide a separate unlimited sentinel in v1.

## Error behavior

Resource-limit failures use:

```cpp
ErrorCode::ResponseLimitExceeded
```

They are kept separate from `MalformedResponse`, `InvalidHeader`, and other syntax/framing errors because a response can be syntactically valid while exceeding the caller's configured resource budget.

A request that fails because of a response limit does not leave its connection eligible for reuse.
