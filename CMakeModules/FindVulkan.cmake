#
# Module for finding Vulkan
#
# Vulkan target will be created
#


# try config-based find first
find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

# use regular old-style approach
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find glm include path
	find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h
		/usr/include
		/usr/local/include
		/opt/local/include
	)

	# set *_FOUND flag
	if(VULKAN_INCLUDE_DIR)
		set(${CMAKE_FIND_PACKAGE_NAME}_FOUND True)
	endif()

	# target
	if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
		if(NOT TARGET ${CMAKE_FIND_PACKAGE_NAME})
			add_library(${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_INCLUDE_DIR}"
			)
		endif()
	endif()

endif()

# message
include(CADRMacros)
cadr_report_find_status()
