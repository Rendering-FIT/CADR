#
# Module for finding nlohmann_json
#
# nlohmann_json::nlohmann_json target will be created
#


# try config-based find first
find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CONFIG QUIET)

# use regular old-style approach
if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND)

	# find glm include path
	find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
		/usr/include
		/usr/local/include
		/opt/local/include
		${THIRD_PARTY_DIR}/json-nlohmann/include
	)

	# set *_FOUND flag
	if(NLOHMANN_JSON_INCLUDE_DIR)
		set(${CMAKE_FIND_PACKAGE_NAME}_FOUND True)
	endif()

	# target
	if(${CMAKE_FIND_PACKAGE_NAME}_FOUND)
		if(NOT TARGET ${CMAKE_FIND_PACKAGE_NAME}::nlohmann_json)
			add_library(${CMAKE_FIND_PACKAGE_NAME}::nlohmann_json INTERFACE IMPORTED)
			set_target_properties(${CMAKE_FIND_PACKAGE_NAME}::nlohmann_json PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${NLOHMANN_JSON_INCLUDE_DIR}"
			)
		endif()
	endif()

endif()

# message
include(CADRMacros)
cadr_report_find_status(${CMAKE_FIND_PACKAGE_NAME}::nlohmann_json)
