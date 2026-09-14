#pragma once

#include <string_view>

namespace cpp_request {

enum class ErrorCode {
    InvalidUrl,
    UnsupportedScheme,
    InvalidPort,

    ResolveFailed,
    SocketCreateFailed,
    ConnectFailed,
    ConnectTimeout,
    WriteFailed,
    WriteTimeout,
    ReadFailed,
    ReadTimeout,
    ConnectionClosed,

    MalformedResponse,
    InvalidStatusLine,
    InvalidHeader,
    InvalidContentLength,
    InvalidChunkSize,
    InvalidChunkFraming,
    ConflictingMessageFraming,
    UnexpectedEof,

    RedirectLimitExceeded,
    MissingRedirectLocation,
    UnsupportedRedirectScheme,

    Unknown
};

struct Error {
    ErrorCode code;
    int native_code{0};
};

[[nodiscard]] constexpr std::string_view error_message(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::InvalidUrl: return "invalid URL";
    case ErrorCode::UnsupportedScheme: return "unsupported URL scheme";
    case ErrorCode::InvalidPort: return "invalid port";
    case ErrorCode::ResolveFailed: return "name resolution failed";
    case ErrorCode::SocketCreateFailed: return "socket creation failed";
    case ErrorCode::ConnectFailed: return "connection failed";
    case ErrorCode::ConnectTimeout: return "connection timed out";
    case ErrorCode::WriteFailed: return "write failed";
    case ErrorCode::WriteTimeout: return "write timed out";
    case ErrorCode::ReadFailed: return "read failed";
    case ErrorCode::ReadTimeout: return "read timed out";
    case ErrorCode::ConnectionClosed: return "connection closed";
    case ErrorCode::MalformedResponse: return "malformed HTTP response";
    case ErrorCode::InvalidStatusLine: return "invalid HTTP status line";
    case ErrorCode::InvalidHeader: return "invalid HTTP header";
    case ErrorCode::InvalidContentLength: return "invalid Content-Length";
    case ErrorCode::InvalidChunkSize: return "invalid chunk size";
    case ErrorCode::InvalidChunkFraming: return "invalid chunk framing";
    case ErrorCode::ConflictingMessageFraming: return "conflicting HTTP message framing";
    case ErrorCode::UnexpectedEof: return "unexpected end of stream";
    case ErrorCode::RedirectLimitExceeded: return "redirect limit exceeded";
    case ErrorCode::MissingRedirectLocation: return "redirect location missing";
    case ErrorCode::UnsupportedRedirectScheme: return "unsupported redirect scheme";
    case ErrorCode::Unknown: return "unknown error";
    }
    return "unknown error";
}

} // namespace cpp_request
