include_guard(GLOBAL)

function(cpp_request_add_example target source)
	add_executable(${target} ${source})
	target_link_libraries(${target} PRIVATE cpp_request::cpp_request)
	target_compile_features(${target} PRIVATE cxx_std_17)
	cpp_request_configure_target(${target})
endfunction()
