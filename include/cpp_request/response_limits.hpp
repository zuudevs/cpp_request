#pragma once

#include <cstddef>

namespace cpp_request {

struct ResponseLimits final {
    static constexpr std::size_t default_max_head_bytes = 64u * 1024u;
    static constexpr std::size_t default_max_body_bytes = 64u * 1024u * 1024u;
    static constexpr std::size_t default_max_chunk_line_bytes = 8u * 1024u;
    static constexpr std::size_t default_max_trailer_bytes = 64u * 1024u;

    std::size_t max_head_bytes{default_max_head_bytes};
    std::size_t max_body_bytes{default_max_body_bytes};
    std::size_t max_chunk_line_bytes{default_max_chunk_line_bytes};
    std::size_t max_trailer_bytes{default_max_trailer_bytes};
};

} // namespace cpp_request
