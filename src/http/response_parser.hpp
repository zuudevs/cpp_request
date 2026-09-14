#pragma once

#include "http/chunked_decoder.hpp"

#include <cstddef>
#include <string>
#include <string_view>

#include <cpp_request/request.hpp>
#include <cpp_request/response.hpp>
#include <cpp_request/result.hpp>

namespace cpp_request::detail::http {

enum class ResponseParseProgress {
    NeedMore,
    Complete
};

class ResponseParser final {
public:
    explicit ResponseParser(Method request_method) noexcept;

    [[nodiscard]] Result<ResponseParseProgress> feed(std::string_view bytes);
    [[nodiscard]] Result<ResponseParseProgress> finish_eof();

    [[nodiscard]] bool complete() const noexcept;
    [[nodiscard]] bool connection_reusable() const noexcept;
    [[nodiscard]] const Response& response() const noexcept { return response_; }
    [[nodiscard]] std::string_view pending_bytes() const noexcept { return buffer_; }

    [[nodiscard]] Response take_response();

private:
    enum class Stage {
        Head,
        ContentLengthBody,
        CloseDelimitedBody,
        ChunkedBody,
        Complete
    };

    [[nodiscard]] Result<ResponseParseProgress> process_buffer();

    Method request_method_;
    Stage stage_{Stage::Head};
    Response response_;
    std::string buffer_;
    ChunkedDecoder chunked_decoder_;
    std::size_t content_length_remaining_{0};
    bool connection_close_requested_{false};
    bool close_delimited_{false};
};

} // namespace cpp_request::detail::http
