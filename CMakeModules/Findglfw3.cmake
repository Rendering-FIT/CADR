#
# Module for finding GLFW3
#
# It attempts to perform config-based find first. If it fails, it attempts to find includes and libraries using standard way.
#
# Targets (used by both - config find and standard find):
#    glfw (glfw is used instead of glfw3 because of glfw uses it like this)
#
# Cache variables (used by both - config find and standard find):
#    glfw_DLL (on Win32 only)
#
# Cache variables that are used if config-based find fails:
#    glfw_INCLUDE_DIR
#    glfw_LIBRARY
#


# try config-based find first
# but only if the user did not specified its own include dir or library
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR AND NOT ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY)

	find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

	# initialize cache variables
	set(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR-NOTFOUND CACHE PATH "Path to ${CMAKE_FIND_PACKAGE_NAME} include directory.")
	set(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY-NOTFOUND CACHE FILEPATH "Path to ${CMAKE_FIND_PACKAGE_NAME} library.")

	# find GLFW DLL
	if(WIN32)
		set(${CMAKE_FIND_PACKAGE_NAME}_DLL ${CMAKE_FIND_PACKAGE_NAME}_DLL-NOTFOUND CACHE FILEPATH "Path to ${CMAKE_FIND_PACKAGE_NAME}.dll that will be copied into the directory of built executable.")
	endif()

endif()

# use regular old-style approach
# if config-based find did not succeed
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find GLFW include path
	find_path(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR GLFW/glfw3.h
		/usr/include
		/usr/local/include
		/opt/local/include
	)

	# find GLFW library
	if(WIN32)
		find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY
			NAMES
				glfw3.lib glfw3_mt.lib glfw3dll.lib
		)
	else()
		find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY
			NAMES
				libglfw.so libglfw.so.3
			PATHS
				/usr/lib64
				/usr/local/lib64
				/usr/lib
				/usr/lib/x86_64-linux-gnu
				/usr/local/lib
		)
	endif()

	# find GLFW DLL
	if(WIN32)
		find_file(${CMAKE_FIND_PACKAGE_NAME}_DLL
			NAMES
				glfw3.dll
		)
	endif()

	# check GLFW version
	if(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR AND EXISTS "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}/GLFW/glfw3.h")
		file(READ "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}/GLFW/glfw3.h" fileContent)
		string(REGEX MATCH "GLFW_VERSION_MAJOR[ ]+([0-9]*)" _ "${fileContent}")
		set(major "${CMAKE_MATCH_1}")
		string(REGEX MATCH "GLFW_VERSION_MINOR[ ]+([0-9]*)" _ "${fileContent}")
		set(minor "${CMAKE_MATCH_1}")
		string(REGEX MATCH "GLFW_VERSION_REVISION[ ]+([0-9]*)" _ "${fileContent}")
		set(revision "${CMAKE_MATCH_1}")
		set(version "${major}.${minor}.${revision}")
	endif()

	# handle required variables and set ${CMAKE_FIND_PACKAGE_NAME}_FOUND variable
	include(FindPackageHandleStandardArgs)
	string(CONCAT errorMessage
		"Finding of package ${CMAKE_FIND_PACKAGE_NAME} failed. Make sure it is installed, "
		"it is of the version ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} or newer, "
		"and either (1) ${CMAKE_FIND_PACKAGE_NAME}_DIR is set to the directory of "
		"${CMAKE_FIND_PACKAGE_NAME}Config.cmake and ${CMAKE_FIND_PACKAGE_NAME}-config.cmake "
		"or (2) ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR and ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY "
		"and other relevant ${CMAKE_FIND_PACKAGE_NAME}_* variables are set properly.")
	find_package_handle_standard_args(
		${CMAKE_FIND_PACKAGE_NAME}
		REQUIRED_VARS ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR
		VERSION_VAR version
		REASON_FAILURE_MESSAGE ${errorMessage}
	)

	# test existence
	if(NOT EXISTS "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}")
		message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR does not point to existing directory.")
	endif()
	if(NOT EXISTS "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}")
		message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME}_LIBRARY does not point to existing file.")
	endif()

	# target
	if(NOT TARGET glfw)
		add_library(glfw SHARED IMPORTED)
		set_target_properties(glfw PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}"
			INTERFACE_LINK_LIBRARIES "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}"
			IMPORTED_IMPLIB "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}"
			IMPORTED_LOCATION "${${CMAKE_FIND_PACKAGE_NAME}_DLL}"
		)
	endif()

endif()

# get glfw3_DLL
if(TARGET glfw AND (NOT DEFINED CACHE{${CMAKE_FIND_PACKAGE_NAME}_DLL} OR ${CMAKE_FIND_PACKAGE_NAME}_DLL STREQUAL "${CMAKE_FIND_PACKAGE_NAME}_DLL-NOTFOUND") AND WIN32)
	get_target_property(glfw3_DLL glfw IMPORTED_LOCATION)
	cmake_path(NORMAL_PATH glfw3_DLL)
	set(glfw3_DLL "${glfw3_DLL}" CACHE FILEPATH "Path to glfw3.dll that will be copied into the directory of built executable." FORCE)
endif()

# make alias target glfw3
if(NOT TARGET glfw3 AND TARGET glfw)
	add_library(glfw3 ALIAS glfw)
endif()

# message
include(CADRMacros)
cadr_report_find_status()
