/**
 * @file request.h
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1.0
 * @date 2026-05-30
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "error.h"
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace zd::crq {

enum class Method : uint8_t {
	Get,
	Post,
	Put,
	Patch,
	Delete
};

class Response;

class Request {
	std::expected<Response, Error> request(
		std::string_view host,
		uint16_t port,
		std::string_view request
	) noexcept;
};

class RequestBuilder {
public:
	
private:
	std::vector<char> data_{};
};

} // namespace zd::crq