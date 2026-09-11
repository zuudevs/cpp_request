find_program(CLANG_FORMAT_EXE NAMES clang-format)

function(add_clang_format_target target)
	if(CLANG_FORMAT_EXE)
		get_target_property(TARGET_SOURCES ${target} SOURCES)

		add_custom_target(format
			COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${TARGET_SOURCES}
			COMMENT "Formatting ${target} source files with clang-format"
		)

		add_custom_target(format-check
			COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror -style=file ${TARGET_SOURCES}
			COMMENT "Checking ${target} source formatting"
		)
	else()
		message(WARNING "clang format not found! Target '${target}' will not be created.")
	endif()
endfunction()