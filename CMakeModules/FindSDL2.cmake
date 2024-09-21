#
# Module for finding SDL2
#
# It attempts to perform config-based find first. If it fails, it attempts to find includes and libraries using standard way.
#
# Targets (used by both - config find and standard find):
#    SDL2::SDL2
#    SDL2::SDL2main
#
# Cache variables (used by both - config find and standard find):
#    SDL2_DLL (on Win32 only)
#
# Cache variables that are used if config-based find fails:
#    SDL2_INCLUDE_DIR
#    SDL2_LIBRARY
#    SDL2_MAIN_LIBRARY (optional)
#


# try config-based find first
# but only if the user did not specified its own include dir or library
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR AND NOT ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY)

	find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

	# initialize cache variables
	set(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR-NOTFOUND CACHE PATH "Path to ${CMAKE_FIND_PACKAGE_NAME} include directory.")
	set(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY-NOTFOUND CACHE FILEPATH "Path to ${CMAKE_FIND_PACKAGE_NAME} library.")
	set(${CMAKE_FIND_PACKAGE_NAME}_MAIN_LIBRARY ${CMAKE_FIND_PACKAGE_NAME}_MAIN_LIBRARY-NOTFOUND CACHE FILEPATH "Path to ${CMAKE_FIND_PACKAGE_NAME} main library.")
	if(WIN32)
		set(${CMAKE_FIND_PACKAGE_NAME}_DLL ${CMAKE_FIND_PACKAGE_NAME}_DLL-NOTFOUND CACHE FILEPATH "Path to ${CMAKE_FIND_PACKAGE_NAME}.dll that will be copied into the directory of built executable.")
	endif()

endif()

# use regular old-style approach
# if config-based find did not succeed
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find SDL2 include path
	find_path(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR SDL.h
		PATH_SUFFIXES
			SDL2
		PATHS
			/usr/include
			/usr/local/include
			/opt/local/include
	)

	# find SDL2 library
	find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY
		NAMES
			SDL2
		PATHS
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
	)
	find_library(${CMAKE_FIND_PACKAGE_NAME}_MAIN_LIBRARY
		NAMES
			SDL2main
		PATHS
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
	)

	# find SDL2_DLL library
	if(WIN32)
		find_file(${CMAKE_FIND_PACKAGE_NAME}_DLL
			NAMES
				SDL2.dll
		)
	endif()

	# set output variables
	if(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR AND ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY)
		set(SDL2_INCLUDE_DIRS "${SDL2_INCLUDE_DIR}")
		if(SDL2_MAIN_LIBRARY)
			set(SDL2_LIBRARIES    "${SDL2_LIBRARY}" "${SDL2_MAIN_LIBRARY}")
		else()
			set(SDL2_LIBRARIES    "${SDL2_LIBRARY}")
		endif()
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
	# (skip SDL2_MAIN_LIBRARY as it is optional)
	if(NOT EXISTS "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}")
		message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR does not point to existing directory.")
	endif()
	if(NOT EXISTS "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}")
		message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME}_LIBRARY does not point to existing file.")
	endif()
	if(NOT EXISTS "${${CMAKE_FIND_PACKAGE_NAME}_DLL}" AND WIN32)
		message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME}_DLL does not point to existing file.")
	endif()
	
	# SDL2::SDL2 target
	if(NOT TARGET SDL2::SDL2)
		add_library(SDL2::SDL2 SHARED IMPORTED)
		set_target_properties(SDL2::SDL2 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}"
			IMPORTED_LINK_LIBRARIES "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}"
			IMPORTED_IMPLIB "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}"
			IMPORTED_LOCATION "${${CMAKE_FIND_PACKAGE_NAME}_DLL}"
		)
	endif()

	# SDL2::SDL2main target
	if(NOT TARGET SDL2::SDL2main AND
	   NOT ${CMAKE_FIND_PACKAGE_NAME}_MAIN_LIBRARY STREQUAL "${CMAKE_FIND_PACKAGE_NAME}_MAIN_LIBRARY-NOTFOUND" AND
	   EXISTS "${${CMAKE_FIND_PACKAGE_NAME}_MAIN_LIBRARY}")

		add_library(SDL2::SDL2main STATIC IMPORTED)
		set_target_properties(SDL2::SDL2main PROPERTIES
			IMPORTED_LOCATION "${${CMAKE_FIND_PACKAGE_NAME}_MAIN_LIBRARY}"
		)
		if(WIN32)
			if(CMAKE_SIZEOF_VOID_P EQUAL 4)
				set_target_properties(SDL2::SDL2main PROPERTIES
					INTERFACE_LINK_OPTIONS "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-Wl,--undefined=_WinMain@16>"
				)
			else()
				set_target_properties(SDL2::SDL2main PROPERTIES
					INTERFACE_LINK_OPTIONS "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-Wl,--undefined=WinMain>"
				)
			endif()
		endif()
	endif()

endif()

# get SDL2_DLL
if(TARGET SDL2::SDL2 AND (NOT DEFINED CACHE{SDL2_DLL} OR SDL2_DLL STREQUAL "SDL2_DLL-NOTFOUND") AND WIN32)
	get_target_property(SDL2_DLL SDL2::SDL2 IMPORTED_LOCATION)
	cmake_path(NORMAL_PATH SDL2_DLL)
	set(SDL2_DLL "${SDL2_DLL}" CACHE FILEPATH "Path to SDL2.dll that will be copied into the directory of built executable." FORCE)
endif()

# message
include(CADRMacros)
cadr_report_find_status()
