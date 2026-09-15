include_guard(GLOBAL)

find_program(CLANG_FORMAT_EXE NAMES clang-format)

function(cpp_request_add_clang_format_targets)
	if(NOT CLANG_FORMAT_EXE)
		message(WARNING
			"CPP_REQUEST_ENABLE_CLANG_FORMAT is ON, but clang-format was not found"
		)
		return()
	endif()

	file(GLOB_RECURSE _cpp_request_format_sources
		CONFIGURE_DEPENDS
		LIST_DIRECTORIES FALSE
		"${PROJECT_SOURCE_DIR}/include/*.h"
		"${PROJECT_SOURCE_DIR}/include/*.hpp"
		"${PROJECT_SOURCE_DIR}/src/*.c"
		"${PROJECT_SOURCE_DIR}/src/*.cc"
		"${PROJECT_SOURCE_DIR}/src/*.cpp"
		"${PROJECT_SOURCE_DIR}/src/*.h"
		"${PROJECT_SOURCE_DIR}/src/*.hpp"
		"${PROJECT_SOURCE_DIR}/tests/*.cpp"
		"${PROJECT_SOURCE_DIR}/tests/*.hpp"
		"${PROJECT_SOURCE_DIR}/benchmarks/*.cpp"
		"${PROJECT_SOURCE_DIR}/benchmarks/*.hpp"
		"${PROJECT_SOURCE_DIR}/examples/*.cpp"
		"${PROJECT_SOURCE_DIR}/examples/*.hpp"
	)

	if(NOT _cpp_request_format_sources)
		return()
	endif()

	add_custom_target(cpp_request_format
		COMMAND "${CLANG_FORMAT_EXE}" -i -style=file ${_cpp_request_format_sources}
		COMMENT "Formatting cpp_request C++ sources"
		VERBATIM
	)

	add_custom_target(cpp_request_format_check
		COMMAND "${CLANG_FORMAT_EXE}" --dry-run --Werror -style=file ${_cpp_request_format_sources}
		COMMENT "Checking cpp_request C++ source formatting"
		VERBATIM
	)
endfunction()
