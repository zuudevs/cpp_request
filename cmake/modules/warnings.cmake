include_guard(GLOBAL)

function(cpp_request_apply_warnings target)
	if(NOT CPP_REQUEST_ENABLE_WARNINGS)
		return()
	endif()

	if(MSVC)
		target_compile_options(${target}
			PRIVATE
				/W4
				/permissive-
		)
		if(CPP_REQUEST_WARNINGS_AS_ERRORS)
			target_compile_options(${target} PRIVATE /WX)
		endif()
	else()
		target_compile_options(${target}
			PRIVATE
				-Wall
				-Wextra
				-Wpedantic
				-Wconversion
				-Wshadow
		)
		if(CPP_REQUEST_WARNINGS_AS_ERRORS)
			target_compile_options(${target} PRIVATE -Werror)
		endif()
	endif()
endfunction()
