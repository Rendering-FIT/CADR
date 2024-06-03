#
# Module for finding Boost
#
# targets:
#    Boost::boost
#    Boost::headers (since CMake 3.15)
#
# variables:
#    Boost_FOUND
#    Boost_INCLUDE_DIR
#    Boost_LIBRARY
#


# perform find only if the user did not specify its own include dir or library
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR AND NOT ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY)

	# make cache variables exist so user can set them easily if he wishes
	set(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR-NOTFOUND CACHE PATH "Path to ${CMAKE_FIND_PACKAGE_NAME} include directory.")
	set(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY-NOTFOUND CACHE FILEPATH "Path to ${CMAKE_FIND_PACKAGE_NAME} library.")

	# try config-based find first
	find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

	# use regular old-style approach
	# if config-based find did not succeed
	if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

		# try regular find modules
		# skipping CadR/CMakeModules
		set(CMAKE_MODULE_PATH_SAVED ${CMAKE_MODULE_PATH})
		list(REMOVE_AT CMAKE_MODULE_PATH 0)
		set(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED_SAVED ${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
		set(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED 0)
		find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} MODULE QUIET)
		set(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED ${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED_SAVED)

		# use 3rdParty dir
		if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)
			set(ENV{BOOSTROOT} "${THIRD_PARTY_DIR}/Boost/")
			find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} MODULE QUIET)
		endif()
		set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH_SAVED})

	endif()

	# mark varialbes not advanced
	mark_as_advanced(CLEAR "${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR")
	mark_as_advanced(CLEAR "${CMAKE_FIND_PACKAGE_NAME}_LIBRARY")

endif()

# set *_FOUND flag
if(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR)
	set(${CMAKE_FIND_PACKAGE_NAME}_FOUND True)
endif()

# target
if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
	if(NOT TARGET Boost::boost)
		add_library(Boost::boost INTERFACE IMPORTED)
		set_target_properties(Boost::boost PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}"
		)
		set(${CMAKE_FIND_PACKAGE_NAME}_DIR "${CMAKE_FIND_PACKAGE_NAME}_DIR-NOTFOUND" CACHE PATH "${CMAKE_FIND_PACKAGE_NAME} config directory." FORCE)
	endif()
	if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
		if(NOT TARGET Boost::headers)
			add_library(Boost::headers INTERFACE IMPORTED)
			set_target_properties(Boost::headers PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR}"
			)
		endif()
	endif()
endif()

# message
# (Boost::headers is available since CMake 3.15)
include(CADRMacros)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
	cadr_report_find_status(${CMAKE_FIND_PACKAGE_NAME}::headers)
else()
	cadr_report_find_status(${CMAKE_FIND_PACKAGE_NAME}::boost)
endif()
