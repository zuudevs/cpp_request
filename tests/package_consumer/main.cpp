#include <cpp_request/client.hpp>
#include <cpp_request/error.hpp>
#include <cpp_request/request.hpp>
#include <cpp_request/response_limits.hpp>

int main() {
    cpp_request::Request request{
        cpp_request::Method::Get,
        "http://example.com/"};
    request.add_query_param("q", "cpp request");

    cpp_request::Client client;
    cpp_request::ResponseLimits limits;
    limits.max_body_bytes = 1024;
    client.set_response_limits(limits);

    return cpp_request::error_message(cpp_request::ErrorCode::InvalidUrl).empty()
        ? 1
        : 0;
}
