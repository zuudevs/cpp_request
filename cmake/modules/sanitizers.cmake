include_guard(GLOBAL)

function(cpp_request_apply_sanitizers target)
	if(NOT CPP_REQUEST_ENABLE_SANITIZERS)
		return()
	endif()

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
		target_compile_options(${target}
			PRIVATE
				-fsanitize=address
				-fsanitize=undefined
				-fno-omit-frame-pointer
		)
		target_link_options(${target}
			PRIVATE
				-fsanitize=address
				-fsanitize=undefined
		)
	else()
		message(WARNING
			"CPP_REQUEST_ENABLE_SANITIZERS is enabled, but the current compiler is unsupported"
		)
	endif()
endfunction()
