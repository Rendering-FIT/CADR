#
# Module for finding vfont
#
# vfont::vfont target will be created
#


# try config-based find first
find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

# use regular old-style approach
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find vfont include path
	find_path(VFONT_INCLUDE_DIR VFONT/text_renderer.h
		/usr/include
		/usr/local/include
		/opt/local/include
		${THIRD_PARTY_DIR}/vfont/include
	)

	# find vfont library
	find_library(VFONT_LIBRARY_RELEASE
		NAMES
			vfont
		PATHS
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
			${THIRD_PARTY_DIR}/vfont/lib
	)
	find_library(VFONT_LIBRARY_DEBUG
		NAMES
			vfontd
		PATHS
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
			${THIRD_PARTY_DIR}/vfont/lib
	)

	# set *_FOUND flag
	if(VFONT_INCLUDE_DIR AND (VFONT_LIBRARY_RELEASE OR VFONT_LIBRARY_DEBUG))
		set(${CMAKE_FIND_PACKAGE_NAME}_FOUND True)
	endif()

	# target
	if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
		if(NOT TARGET ${CMAKE_FIND_PACKAGE_NAME}::vfont)
			add_library(${CMAKE_FIND_PACKAGE_NAME}::vfont INTERFACE IMPORTED)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME}::vfont PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${VFONT_INCLUDE_DIR}"
			)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME}::vfont PROPERTIES
				INTERFACE_LINK_LIBRARIES
					"$<$<CONFIG:Release>:${VFONT_LIBRARY_RELEASE}>$<$<CONFIG:RelWithDebInfo>:${VFONT_LIBRARY_RELEASE}>$<$<CONFIG:MinSizeRel>:${VFONT_LIBRARY_RELEASE}>$<$<CONFIG:Debug>:${VFONT_LIBRARY_DEBUG}>"
			)
		endif()
	endif()

endif()

# message
include(CADRMacros)
cadr_report_find_status(${CMAKE_FIND_PACKAGE_NAME}::vfont)
