include_guard(GLOBAL)

include(CMakePackageConfigHelpers)

function(cpp_request_configure_install)
	set(
		CPP_REQUEST_INSTALL_CMAKEDIR
		"${CMAKE_INSTALL_LIBDIR}/cmake/cpp_request"
	)

	configure_package_config_file(
		"${PROJECT_SOURCE_DIR}/cmake/templates/cpp_requestConfig.cmake.in"
		"${PROJECT_BINARY_DIR}/cpp_requestConfig.cmake"
		INSTALL_DESTINATION "${CPP_REQUEST_INSTALL_CMAKEDIR}"
	)

	write_basic_package_version_file(
		"${PROJECT_BINARY_DIR}/cpp_requestConfigVersion.cmake"
		VERSION "${PROJECT_VERSION}"
		COMPATIBILITY SameMajorVersion
	)

	install(
		TARGETS cpp_request
		EXPORT cpp_requestTargets
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	)

	install(
		DIRECTORY "${PROJECT_SOURCE_DIR}/include/cpp_request"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
		FILES_MATCHING PATTERN "*.hpp"
	)

	install(
		EXPORT cpp_requestTargets
		FILE cpp_requestTargets.cmake
		NAMESPACE cpp_request::
		DESTINATION "${CPP_REQUEST_INSTALL_CMAKEDIR}"
	)

	install(
		FILES
			"${PROJECT_BINARY_DIR}/cpp_requestConfig.cmake"
			"${PROJECT_BINARY_DIR}/cpp_requestConfigVersion.cmake"
		DESTINATION "${CPP_REQUEST_INSTALL_CMAKEDIR}"
	)
endfunction()
