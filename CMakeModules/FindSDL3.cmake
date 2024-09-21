#
# Module for finding SDL3
#
# It attempts to perform config-based find first. If it fails, it attempts to find includes and libraries using standard way.
#
# Targets (used by both - config find and standard find):
#    SDL3::SDL3
#
# Cache variables (used by both - config find and standard find):
#    SDL3_DLL (on Win32 only)
#
# Cache variables that are used if config-based find fails:
#    SDL3_INCLUDE_DIR
#    SDL3_LIBRARY
#


# try config-based find first
# but only if the user did not specified its own include dir or library
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR AND NOT ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY)

	find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

	# initialize cache variables
	set(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR-NOTFOUND CACHE PATH "Path to ${CMAKE_FIND_PACKAGE_NAME} include directory.")
	set(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY-NOTFOUND CACHE FILEPATH "Path to ${CMAKE_FIND_PACKAGE_NAME} library.")
	if(WIN32)
		set(${CMAKE_FIND_PACKAGE_NAME}_DLL ${CMAKE_FIND_PACKAGE_NAME}_DLL-NOTFOUND CACHE FILEPATH "Path to ${CMAKE_FIND_PACKAGE_NAME}.dll that will be copied into the directory of built executable.")
	endif()

endif()

# use regular old-style approach
# if config-based find did not succeed
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find SDL3 include path
	find_path(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR SDL.h
		PATH_SUFFIXES
			SDL3
		PATHS
			/usr/include
			/usr/local/include
			/opt/local/include
	)

	# find SDL3 library
	find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY
		NAMES
			SDL3
		PATHS
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
	)

	# find SDL3_DLL library
	if(WIN32)
		find_file(${CMAKE_FIND_PACKAGE_NAME}_DLL
			NAMES
				SDL3.dll
		)
	endif()

	# set output variables
	if(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR AND ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY)
		set(SDL3_INCLUDE_DIRS "${SDL3_INCLUDE_DIR}")
		set(SDL3_LIBRARIES    "${SDL3_LIBRARY}")
	endif()

	# handle required variables and set ${CMAKE_FIND_PACKAGE_NAME}_FOUND variable
	include(FindPackageHandleStandardArgs)
	string(CONCAT errorMessage
		"Finding of package ${CMAKE_FIND_PACKAGE_NAME} failed. Make sure it is installed "
	    "and either (1) ${CMAKE_FIND_PACKAGE_NAME}_DIR is set to the directory of "
		"${CMAKE_FIND_PACKAGE_NAME}Config.cmake and ${CMAKE_FIND_PACKAGE_NAME}-config.cmake "
		"or (2) ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR and ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY "
		"and other relevant ${CMAKE_FIND_PACKAGE_NAME}_* variables are set properly.")
	if(WIN32)
		find_package_handle_standard_args(
			${CMAKE_FIND_PACKAGE_NAME}
			REQUIRED_VARS ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY ${CMAKE_FIND_PACKAGE_NAME}_DLL ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR
			REASON_FAILURE_MESSAGE ${errorMessage}
		)
	else()
		find_package_handle_standard_args(
			${CMAKE_FIND_PACKAGE_NAME}
			REQUIRED_VARS ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR
			REASON_FAILURE_MESSAGE ${errorMessage}
		)
	endif()

	# test existence
	if(NOT EXISTS "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}")
		message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR does not point to existing directory.")
	endif()
	if(NOT EXISTS "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}")
		message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME}_LIBRARY does not point to existing file.")
	endif()
	if(NOT EXISTS "${${CMAKE_FIND_PACKAGE_NAME}_DLL}" AND WIN32)
		message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME}_DLL does not point to existing file.")
	endif()
	
	# target
	if(NOT TARGET SDL3::SDL3)
		add_library(SDL3::SDL3 SHARED IMPORTED)
		set_target_properties(SDL3::SDL3 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}"
			IMPORTED_LINK_LIBRARIES "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}"
			IMPORTED_IMPLIB "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}"
			IMPORTED_LOCATION "${${CMAKE_FIND_PACKAGE_NAME}_DLL}"
		)
	endif()

endif()

# get SDL3_DLL
if(TARGET SDL3::SDL3 AND (NOT DEFINED CACHE{SDL3_DLL} OR SDL3_DLL STREQUAL "SDL3_DLL-NOTFOUND") AND WIN32)
	get_target_property(SDL3_DLL SDL3::SDL3 IMPORTED_LOCATION)
	cmake_path(NORMAL_PATH SDL3_DLL)
	set(SDL3_DLL "${SDL3_DLL}" CACHE FILEPATH "Path to SDL3.dll that will be copied into the directory of built executable." FORCE)
endif()

# message
include(CADRMacros)
cadr_report_find_status()
