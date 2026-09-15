include_guard(GLOBAL)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)

function(cpp_request_apply_clang_tidy target)
	if(CLANG_TIDY_EXE)
		set_target_properties(
			${target}
			PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
		)
	else()
		message(WARNING
			"CPP_REQUEST_ENABLE_CLANG_TIDY is ON, but clang-tidy was not found"
		)
	endif()
endfunction()
