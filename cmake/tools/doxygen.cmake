include_guard(GLOBAL)

find_package(Doxygen QUIET)

function(cpp_request_add_doxygen_target target)
	if(NOT Doxygen_FOUND)
		message(WARNING
			"CPP_REQUEST_ENABLE_DOXYGEN is ON, but Doxygen was not found"
		)
		return()
	endif()

	set(_cpp_request_doxyfile "${PROJECT_BINARY_DIR}/Doxyfile")
	configure_file(
		"${PROJECT_SOURCE_DIR}/cmake/templates/Doxyfile.in"
		"${_cpp_request_doxyfile}"
		@ONLY
	)

	add_custom_target(${target}
		COMMAND Doxygen::doxygen "${_cpp_request_doxyfile}"
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
		COMMENT "Generating cpp_request API documentation"
		VERBATIM
	)
endfunction()
