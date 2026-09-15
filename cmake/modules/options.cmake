include_guard(GLOBAL)

function(cpp_request_setup_options)
	set(_cpp_request_top_level_default OFF)
	if(PROJECT_IS_TOP_LEVEL)
		set(_cpp_request_top_level_default ON)
	endif()

	option(
		CPP_REQUEST_BUILD_TESTS
		"Build cpp_request tests"
		${_cpp_request_top_level_default}
	)
	option(
		CPP_REQUEST_BUILD_BENCHMARKS
		"Build cpp_request benchmarks"
		OFF
	)
	option(
		CPP_REQUEST_BUILD_EXAMPLES
		"Build cpp_request examples"
		${_cpp_request_top_level_default}
	)
	option(
		CPP_REQUEST_ENABLE_INSTALL
		"Enable cpp_request install and package export rules"
		${_cpp_request_top_level_default}
	)

	option(CPP_REQUEST_ENABLE_WARNINGS "Enable compiler warnings" ON)
	option(CPP_REQUEST_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
	option(CPP_REQUEST_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
	option(CPP_REQUEST_ENABLE_IPO "Enable interprocedural optimization for Release builds" OFF)
	option(CPP_REQUEST_ENABLE_NATIVE_OPTIMIZATION "Enable host-specific optimization for local Release builds" OFF)

	option(CPP_REQUEST_ENABLE_CLANG_TIDY "Enable clang-tidy for cpp_request targets" OFF)
	option(CPP_REQUEST_ENABLE_CLANG_FORMAT "Add clang-format helper targets" OFF)
	option(CPP_REQUEST_ENABLE_DOXYGEN "Add the Doxygen documentation target" OFF)
endfunction()
