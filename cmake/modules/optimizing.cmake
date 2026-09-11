if(ENABLE_RELEASE_OPTIMIZATION)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
endif()

function(add_release_optimizations_target target)
	if(ENABLE_RELEASE_OPTIMIZATION)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
			target_compile_options(${target} 
				PRIVATE
					$<$<CONFIG:Release>:-O3>
					$<$<CONFIG:Release>:-ffunction-sections>
					$<$<CONFIG:Release>:-fdata-sections>
			)
			if(ENABLE_NATIVE_OPTIMIZATION)
				target_compile_options(${target} 
					PRIVATE
						$<$<CONFIG:Release>:-march=native>
						$<$<CONFIG:Release>:-mtune=native>
				)
			endif()
			if(WIN32)
				target_link_options(${target} 
					PRIVATE
						$<$<CONFIG:Release>:LINKER:/OPT:REF>
						$<$<CONFIG:Release>:LINKER:/OPT:ICF>
				)
			else()
				target_link_options(${target} 
					PRIVATE
						$<$<CONFIG:Release>:LINKER:--gc-sections>
				)
			endif()
		elseif(MSVC)
			target_compile_options(${target} 
				PRIVATE
					$<$<CONFIG:Release>:/O2>
					$<$<CONFIG:Release>:/GL>
					$<$<CONFIG:Release>:/arch:AVX2>
			)
			target_link_options(${target} 
				PRIVATE
					$<$<CONFIG:Release>:/LTCG>
			)
		endif()
		if(IPO_SUPPORTED AND ENABLE_IPO)
			set_property(
				TARGET 
					${target} 
				PROPERTY 
					INTERPROCEDURAL_OPTIMIZATION_RELEASE 
				TRUE
			)
		endif()
	endif()
endfunction()