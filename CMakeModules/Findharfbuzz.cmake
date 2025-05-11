#
# Module for finding harfbuzz
#
# harfbuzz::harfbuzz target will be created
#


# try config-based find first
find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

# use regular old-style approach
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find harfbuzz include path
	find_path(HARFBUZZ_INCLUDE_DIR harfbuzz/hb.h
		/usr/include
		/usr/local/include
		/opt/local/include
		${THIRD_PARTY_DIR}/vfont/include
	)

	# find harfbuzz library
	find_library(HARFBUZZ_LIBRARY_RELEASE
		NAMES
			harfbuzz
		PATHS
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
			${THIRD_PARTY_DIR}/vfont/lib
	)
	find_library(HARFBUZZ_LIBRARY_DEBUG
		NAMES
			harfbuzzd
		PATHS
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
			${THIRD_PARTY_DIR}/vfont/lib
	)

	# set *_FOUND flag
	if(HARFBUZZ_INCLUDE_DIR AND (HARFBUZZ_LIBRARY_RELEASE OR HARFBUZZ_LIBRARY_DEBUG))
		set(${CMAKE_FIND_PACKAGE_NAME}_FOUND True)
	endif()

	# target
	if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
		if(NOT TARGET ${CMAKE_FIND_PACKAGE_NAME}::harfbuzz)
			add_library(${CMAKE_FIND_PACKAGE_NAME}::harfbuzz INTERFACE IMPORTED)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME}::harfbuzz PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIR}/harfbuzz"
			)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME}::harfbuzz PROPERTIES
				INTERFACE_LINK_LIBRARIES
					"$<$<CONFIG:Release>:${HARFBUZZ_LIBRARY_RELEASE}>$<$<CONFIG:RelWithDebInfo>:${HARFBUZZ_LIBRARY_RELEASE}>$<$<CONFIG:MinSizeRel>:${HARFBUZZ_LIBRARY_RELEASE}>$<$<CONFIG:Debug>:${HARFBUZZ_LIBRARY_DEBUG}>"
			)
		endif()
	endif()

endif()

# message
include(CADRMacros)
cadr_report_find_status(${CMAKE_FIND_PACKAGE_NAME}::harfbuzz)
