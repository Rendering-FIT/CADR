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
if(TARGET ${CMAKE_FIND_PACKAGE_NAME})

	# print message on success
	if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
		get_property(ALREADY_REPORTED GLOBAL PROPERTY ${CMAKE_FIND_PACKAGE_NAME}_FOUND_REPORTED)
		if(NOT ALREADY_REPORTED)

			# try path from the linked library
			get_target_property(found_where ${CMAKE_FIND_PACKAGE_NAME} INTERFACE_LINK_LIBRARIES)
			if(NOT EXISTS ${found_where})
				set(found_where "")
			endif()

			# try IMPORTED_LIBNAME
			if("${found_where}" STREQUAL "" OR
				"${found_where}" STREQUAL "found_where-NOTFOUND")
				get_target_property(found_where ${CMAKE_FIND_PACKAGE_NAME} IMPORTED_LIBNAME)
			endif()

			# try LOCATION
			get_target_property(target_type ${CMAKE_FIND_PACKAGE_NAME} TYPE)
			if(NOT ${target_type} STREQUAL INTERFACE_LIBRARY)
				if("${found_where}" STREQUAL "" OR
					"${found_where}" STREQUAL "found_where-NOTFOUND")
					get_target_property(found_where ${CMAKE_FIND_PACKAGE_NAME} LOCATION)
				endif()
			endif()

			# try IMPORTED_LOCATION
			if(NOT ${target_type} STREQUAL INTERFACE_LIBRARY)
				if("${found_where}" STREQUAL "" OR
					"${found_where}" STREQUAL "found_where-NOTFOUND")
					get_target_property(found_where ${CMAKE_FIND_PACKAGE_NAME} IMPORTED_LOCATION)
				endif()
			endif()

			# try path from include directory
			if("${found_where}" STREQUAL "" OR
				"${found_where}" STREQUAL "found_where-NOTFOUND")
				get_target_property(found_where ${CMAKE_FIND_PACKAGE_NAME} INTERFACE_INCLUDE_DIRECTORIES)
			endif()

			# try config path
			if("${found_where}" STREQUAL "" OR
				"${found_where}" STREQUAL "found_where-NOTFOUND")
				set(found_where "${${PACKAGE_NAME}_DIR}")
			endif()

			# use target name
			if("${found_where}" STREQUAL "" OR
				"${found_where}" STREQUAL "found_where-NOTFOUND")
				set(found_where "${CMAKE_FIND_PACKAGE_NAME}")
			endif()

			message(STATUS "Find package ${CMAKE_FIND_PACKAGE_NAME}: ${found_where}")
			set_property(GLOBAL PROPERTY ${CMAKE_FIND_PACKAGE_NAME}_FOUND_REPORTED True)
		endif()
	endif()

else()

	# print message on failure (REQUIRED option)
	if(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
		get_property(ALREADY_REPORTED GLOBAL PROPERTY ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_ERROR_REPORTED)
		if(NOT ALREADY_REPORTED)
			message(FATAL_ERROR "Find package ${CMAKE_FIND_PACKAGE_NAME}: not found")  # FATAL_ERROR will stop CMake processing
			set_property(GLOBAL PROPERTY ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_ERROR_REPORTED True)
		endif()

	# print message on failure (QUIET option)
	else()
		if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
			get_property(ALREADY_REPORTED GLOBAL PROPERTY ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_INFO_REPORTED)
			if(NOT ALREADY_REPORTED)
				message(STATUS "Find package ${CMAKE_FIND_PACKAGE_NAME}: not found")
				set_property(GLOBAL PROPERTY ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_INFO_REPORTED True)
			endif()
		endif()
	endif()

endif()
