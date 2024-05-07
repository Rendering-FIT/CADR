#
# find libdecor
#
# targets:
#    libdecor::libdecor
#
# variables:
#    libdecor_FOUND
#    libdecor_INCLUDE_DIR
#    libdecor_LIBRARY
#


# try config-based find first
# but only if the user did not specified its own include dir or library
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR AND NOT ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY)

	find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

	# initialize cache variables
	set(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR-NOTFOUND CACHE PATH "Path to ${CMAKE_FIND_PACKAGE_NAME} include directory.")
	set(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY-NOTFOUND CACHE FILEPATH "Path to ${CMAKE_FIND_PACKAGE_NAME} library.")

endif()

# use regular old-style approach
# if config-based find did not succeed
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find include path
	find_path(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR
		NAMES
			libdecor-0/libdecor.h
		PATHS
			/usr/include
			/usr/local/include
			/opt/local/include
	)

	# find library path
	find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY
		NAMES
			decor-0
		PATHS
			/usr/lib64
			/usr/local/lib64
			/usr/lib
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
	)

	# handle required variables and set ${CMAKE_FIND_PACKAGE_NAME}_FOUND variable
	include(FindPackageHandleStandardArgs)
	string(CONCAT errorMessage
		"Finding of package ${CMAKE_FIND_PACKAGE_NAME} failed. Make sure it is installed "
		"and either (1) ${CMAKE_FIND_PACKAGE_NAME}_DIR is set to the directory of "
		"${CMAKE_FIND_PACKAGE_NAME}Config.cmake and ${CMAKE_FIND_PACKAGE_NAME}-config.cmake "
		"or (2) ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR and ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY "
		"and other relevant ${CMAKE_FIND_PACKAGE_NAME}_* variables are set properly.")
	find_package_handle_standard_args(
		${CMAKE_FIND_PACKAGE_NAME}
		REQUIRED_VARS ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR
		REASON_FAILURE_MESSAGE ${errorMessage}
	)

	# create target
	if(${CMAKE_FIND_PACKAGE_NAME}_FOUND AND NOT TARGET libdecor::libdecor)
		add_library(libdecor::libdecor UNKNOWN IMPORTED)
		set_target_properties(libdecor::libdecor PROPERTIES
			IMPORTED_LOCATION "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}")
	endif()

endif()
