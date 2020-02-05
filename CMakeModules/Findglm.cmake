#
# Module for finding GLM
#
# glm target will be created
#


# try config-based find first
# (this provides target glm (tested on GLM version 0.9.7.0))
find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

# use regular old-style approach
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find glm include path
	find_path(GLM_INCLUDE_DIR glm/glm.hpp
		/usr/include
		/usr/local/include
		/opt/local/include
		${THIRD_PARTY_DIR}/GLM
	)

	# set *_FOUND flag
	if(GLM_INCLUDE_DIR)
		set(${CMAKE_FIND_PACKAGE_NAME}_FOUND True)
	endif()

	# target
	if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
		if(NOT TARGET ${CMAKE_FIND_PACKAGE_NAME})
			add_library(${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIR}"
			)
		endif()
	endif()

endif()

# message
include(CADRMacros)
cadr_report_find_status()
