include_guard(GLOBAL)

include(GNUInstallDirs)
include(warnings)
include(sanitizers)
include(optimizations)

if(CPP_REQUEST_ENABLE_CLANG_TIDY)
	include("${PROJECT_SOURCE_DIR}/cmake/tools/clang-tidy.cmake")
endif()

function(cpp_request_setup_project)
	if(PROJECT_IS_TOP_LEVEL AND NOT DEFINED CMAKE_EXPORT_COMPILE_COMMANDS)
		set(
			CMAKE_EXPORT_COMPILE_COMMANDS
			ON
			CACHE BOOL "Export compile_commands.json" FORCE
		)
	endif()
endfunction()

function(cpp_request_configure_target target)
	if(NOT TARGET ${target})
		message(FATAL_ERROR "cpp_request_configure_target: unknown target '${target}'")
	endif()

	cpp_request_apply_warnings(${target})
	cpp_request_apply_sanitizers(${target})
	cpp_request_apply_optimizations(${target})

	if(CPP_REQUEST_ENABLE_CLANG_TIDY)
		cpp_request_apply_clang_tidy(${target})
	endif()
endfunction()
