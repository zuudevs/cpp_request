#pragma once

#include <cstddef>
#include <string>

#include <cpp_request/result.hpp>

namespace cpp_request::detail::http {

enum class ChunkDecodeProgress {
    NeedMore,
    Complete
};

class ChunkedDecoder final {
public:
    [[nodiscard]] Result<ChunkDecodeProgress> process(
        std::string& input,
        std::string& output);

private:
    enum class Stage {
        SizeLine,
        Data,
        DataCrlf,
        Trailers,
        Complete
    };

    Stage stage_{Stage::SizeLine};
    std::size_t chunk_remaining_{0};
};

} // namespace cpp_request::detail::http
