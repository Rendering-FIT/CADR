#
# Module for finding Boost
#
# nlohmann_json::nlohmann_json target will be created
#


# try config-based find first
find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

# use regular old-style approach
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# try regular paths
	set(CMAKE_MODULE_PATH_SAVED ${CMAKE_MODULE_PATH})
	list(REMOVE_AT CMAKE_MODULE_PATH 0)
	set(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED_SAVED ${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
	set(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED 0)
	find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} MODULE QUIET)
	set(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED ${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED_SAVED)

	# use 3rdParty dir
	if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)
		message("Trying2")
		set(ENV{BOOST_ROOT} "${THIRD_PARTY_DIR}/Boost/")
		find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} MODULE QUIET)
	endif()
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH_SAVED})

endif()

# message
include(CADRMacros)
cadr_report_find_status(${CMAKE_FIND_PACKAGE_NAME}::boost)
