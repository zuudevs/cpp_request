include_guard(GLOBAL)

function(cpp_request_apply_optimizations target)
	if(CPP_REQUEST_ENABLE_NATIVE_OPTIMIZATION)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
			target_compile_options(${target}
				PRIVATE
					$<$<CONFIG:Release>:-march=native>
					$<$<CONFIG:Release>:-mtune=native>
			)
		else()
			message(WARNING
				"CPP_REQUEST_ENABLE_NATIVE_OPTIMIZATION currently supports Clang and GCC only"
			)
		endif()
	endif()

	if(CPP_REQUEST_ENABLE_IPO)
		include(CheckIPOSupported)
		check_ipo_supported(RESULT _cpp_request_ipo_supported OUTPUT _cpp_request_ipo_error)
		if(_cpp_request_ipo_supported)
			set_property(
				TARGET ${target}
				PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
			)
		else()
			message(WARNING "IPO is unavailable: ${_cpp_request_ipo_error}")
		endif()
	endif()
endfunction()
