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
		"$ENV{VULKAN_SDK}/Include"
		/usr/include
		/usr/local/include
		/opt/local/include
		${THIRD_PARTY_DIR}/Vulkan/include
	)

	# find Vulkan library
	find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY
		NAMES
			vulkan vulkan-1
		PATHS
			"$ENV{VULKAN_SDK}/Lib"
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
			${THIRD_PARTY_DIR}/Vulkan/lib
	)

	# find glslangValidator
	find_program(${CMAKE_FIND_PACKAGE_NAME}_GLSLANG_VALIDATOR_EXECUTABLE
		NAMES
			glslangValidator
		PATHS
			"$ENV{VULKAN_SDK}/bin"
			"$ENV{VULKAN_SDK}/bin32"
			/usr/bin
			${THIRD_PARTY_DIR}/Vulkan/bin
	)

	# set *_FOUND flag
	if(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR AND ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY)
		set(${CMAKE_FIND_PACKAGE_NAME}_FOUND True)
	endif()

	# Vulkan::Vulkan target
	if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
		if(NOT TARGET ${CMAKE_FIND_PACKAGE_NAME}::${CMAKE_FIND_PACKAGE_NAME})
			add_library(${CMAKE_FIND_PACKAGE_NAME}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}"
				INTERFACE_LINK_LIBRARIES "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}"
			)
		endif()
	endif()

	# Vulkan::Headers target
	if(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR)
		if(NOT TARGET ${CMAKE_FIND_PACKAGE_NAME}::Headers)
			add_library(${CMAKE_FIND_PACKAGE_NAME}::Headers INTERFACE IMPORTED)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME}::Headers PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}"
			)
		endif()
	endif()

	# Vulkan::glslangValidator target
	if(Vulkan_FOUND AND Vulkan_GLSLANG_VALIDATOR_EXECUTABLE AND NOT TARGET Vulkan::glslangValidator)
		add_executable(Vulkan::glslangValidator IMPORTED)
		set_property(TARGET Vulkan::glslangValidator PROPERTY IMPORTED_LOCATION "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")
	endif()

endif()

# message
include(CADRMacros)
cadr_report_find_status()
