#include <cpp_request/client.hpp>
#include <cpp_request/error.hpp>
#include <cpp_request/headers.hpp>
#include <cpp_request/request.hpp>
#include <cpp_request/response.hpp>
#include <cpp_request/response_limits.hpp>
#include <cpp_request/result.hpp>
#include <cpp_request/url.hpp>

int main() {
    cpp_request::Request request{
        cpp_request::Method::Get,
        "http://example.com/"};
    request.headers().add("Accept", "text/plain");
    request.add_query_param("q", "cpp request");

    cpp_request::Client client;
    cpp_request::ResponseLimits limits;
    limits.max_body_bytes = 1024;
    client.set_response_limits(limits);

    const auto parsed = cpp_request::Url::parse("http://example.com/path");
    if (!parsed) {
        return 1;
    }

    auto result = cpp_request::Result<int>::success(42);
    if (!result || result.value() != 42) {
        return 1;
    }

    return cpp_request::error_message(cpp_request::ErrorCode::InvalidUrl).empty()
        ? 1
        : 0;
}
