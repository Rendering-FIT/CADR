#
# Module for finding Vulkan
#
# Vulkan and VulkanHeaders targets will be created
#


# try config-based find first
find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

# use regular old-style approach
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find Vulkan include path
	find_path(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR vulkan/vulkan.h
		/usr/include
		/usr/local/include
		/opt/local/include
		"$ENV{VULKAN_SDK}/include"
	)

	# find Vulkan library
	find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY vulkan
		/usr/lib64
		/usr/local/lib64
		/usr/lib
		/usr/lib/x86_64-linux-gnu
		/usr/local/lib
		"$ENV{VULKAN_SDK}/lib"
	)

	# set *_FOUND flag
	if(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR AND ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY)
		set(${CMAKE_FIND_PACKAGE_NAME}_FOUND True)
	endif()

	# Vulkan target
	if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
		if(NOT TARGET ${CMAKE_FIND_PACKAGE_NAME})
			add_library(${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}"
				INTERFACE_LINK_LIBRARIES "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}"
			)
		endif()
	endif()

	# VulkanHeaders target
	if(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR)
		if(NOT TARGET VulkanHeaders)
			add_library(VulkanHeaders INTERFACE IMPORTED)
			set_target_properties(VulkanHeaders PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}"
			)
		endif()
	endif()

endif()

# message
include(CADRMacros)
cadr_report_find_status()
