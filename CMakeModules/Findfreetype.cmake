#
# Module for finding freetype
#
# freetype::freetype target will be created
#


# try config-based find first
find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

# use regular old-style approach
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find freetype include path
	find_path(FREETYPE_INCLUDE_DIR freetype2/ft2build.h
		/usr/include
		/usr/local/include
		/opt/local/include
		${THIRD_PARTY_DIR}/vfont/include
	)

	# find freetype library
	find_library(FREETYPE_LIBRARY_RELEASE
		NAMES
			freetype
		PATHS
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
			${THIRD_PARTY_DIR}/vfont/lib
	)
	find_library(FREETYPE_LIBRARY_DEBUG
		NAMES
			freetyped
		PATHS
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
			${THIRD_PARTY_DIR}/vfont/lib
	)

	# set *_FOUND flag
	if(FREETYPE_INCLUDE_DIR AND (FREETYPE_LIBRARY_RELEASE OR FREETYPE_LIBRARY_DEBUG))
		set(${CMAKE_FIND_PACKAGE_NAME}_FOUND True)
	endif()

	# target
	if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
		if(NOT TARGET ${CMAKE_FIND_PACKAGE_NAME}::freetype)
			add_library(${CMAKE_FIND_PACKAGE_NAME}::freetype INTERFACE IMPORTED)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME}::freetype PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIR}"
				INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIR}/freetype2"
			)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME}::freetype PROPERTIES
				INTERFACE_LINK_LIBRARIES
					"$<$<CONFIG:Release>:${FREETYPE_LIBRARY_RELEASE}>$<$<CONFIG:RelWithDebInfo>:${FREETYPE_LIBRARY_RELEASE}>$<$<CONFIG:MinSizeRel>:${FREETYPE_LIBRARY_RELEASE}>$<$<CONFIG:Debug>:${FREETYPE_LIBRARY_DEBUG}>"
			)
		endif()
	endif()

endif()

# message
include(CADRMacros)
cadr_report_find_status(${CMAKE_FIND_PACKAGE_NAME}::freetype)
