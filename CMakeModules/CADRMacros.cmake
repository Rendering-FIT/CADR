# returns target's path
# (used generally as info to console during configuration process)
macro(cadr_get_package_path path PACKAGE_NAME TARGET_NAME)

	# try path from the linked library
	get_target_property(found_where ${TARGET_NAME} INTERFACE_LINK_LIBRARIES)
	if(NOT EXISTS "${found_where}")
		set(found_where "")
	endif()

	# try IMPORTED_LIBNAME
	if("${found_where}" STREQUAL "" OR
	   "${found_where}" STREQUAL "found_where-NOTFOUND")
		get_target_property(found_where ${TARGET_NAME} IMPORTED_LIBNAME)
	endif()

	# try LOCATION
	get_target_property(target_type ${TARGET_NAME} TYPE)
	if(NOT ${target_type} STREQUAL INTERFACE_LIBRARY)
		if("${found_where}" STREQUAL "" OR
			"${found_where}" STREQUAL "found_where-NOTFOUND")
			get_target_property(found_where ${TARGET_NAME} LOCATION)
		endif()
	endif()

	# try IMPORTED_LOCATION
	if(NOT ${target_type} STREQUAL INTERFACE_LIBRARY)
		if("${found_where}" STREQUAL "" OR
			"${found_where}" STREQUAL "found_where-NOTFOUND")
			get_target_property(found_where ${TARGET_NAME} IMPORTED_LOCATION)
		endif()
	endif()

	# try path from include directory
	if("${found_where}" STREQUAL "" OR
	   "${found_where}" STREQUAL "found_where-NOTFOUND")
		get_target_property(found_where ${TARGET_NAME} INTERFACE_INCLUDE_DIRECTORIES)
	endif()

	# try config path
	if("${found_where}" STREQUAL "" OR
	   "${found_where}" STREQUAL "found_where-NOTFOUND")
		set(found_where "${${PACKAGE_NAME}_DIR}")
	endif()

	# use target name
	if("${found_where}" STREQUAL "" OR
	   "${found_where}" STREQUAL "found_where-NOTFOUND")
		set(found_where "${TARGET_NAME}")
	endif()

	cmake_path(NORMAL_PATH found_where OUTPUT_VARIABLE found_where)
	set(${path} ${found_where})

endmacro()


# prints find_package result into the console
# (used in various Find*.cmake files)
macro(cadr_report_find_status)

	# handle extra arguments
	# target name might be passed as extra argument
	# otherwise use ${CMAKE_FIND_PACKAGE_NAME}
	set(target_name ${ARGN})
	list(LENGTH target_name num_args)
	if(${num_args} EQUAL 0)
		set(target_name ${CMAKE_FIND_PACKAGE_NAME})
		if(TARGET ${target_name}::${target_name})
			set(target_name ${target_name}::${target_name})
		endif()
	endif()

	if(TARGET ${target_name})

		# print message on success
		if(NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
			get_property(ALREADY_REPORTED GLOBAL PROPERTY ${CMAKE_FIND_PACKAGE_NAME}_FOUND_REPORTED)
			if(NOT ALREADY_REPORTED)
				cadr_get_package_path(found_where ${CMAKE_FIND_PACKAGE_NAME} ${target_name})
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
endmacro()


# creates custom commands to convert GLSL shaders to spir-v
# and creates depsList containing name of files that should be included among the source files
macro(add_shaders nameList depsList)
	foreach(name ${nameList})
		get_filename_component(directory ${name} DIRECTORY)
		if(directory)
			file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${directory}")
		endif()
		add_custom_command(COMMENT "Converting ${name} to spir-v..."
		                   MAIN_DEPENDENCY ${name}
		                   OUTPUT ${name}.spv
		                   COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V -x ${CMAKE_CURRENT_SOURCE_DIR}/${name} -o ${name}.spv)
		source_group("Shaders" FILES ${name})
		source_group("Shaders/SPIR-V" FILES ${CMAKE_CURRENT_BINARY_DIR}/${name}.spv)
		list(APPEND ${depsList} ${name} ${CMAKE_CURRENT_BINARY_DIR}/${name}.spv)
	endforeach()
endmacro()


# creates custom commands to convert GLSL shader to spir-v using preprocessor defines and output file name
# and appends the shader and output file name to depsList; depsList contains name of files that should be included among the source files
macro(add_shader name defines outputFileName depsList)
	get_filename_component(directory ${outputFileName} DIRECTORY)
	if(directory)
		file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${directory}")
	endif()
	if("${defines}" STREQUAL "")
		set(commentText "Converting ${name} (defines: none) to spir-v...")
	else()
		set(commentText "Converting ${name} (defines: ${defines}) to spir-v...")
	endif()
	add_custom_command(COMMENT "${commentText}"
	                   MAIN_DEPENDENCY "${name}"
	                   OUTPUT "${outputFileName}"
	                   COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}" --target-env vulkan1.2 -x ${defines} "${CMAKE_CURRENT_SOURCE_DIR}/${name}" -o "${outputFileName}")
	source_group("Shaders" FILES "${name}")
	source_group("Shaders/SPIR-V" FILES "${CMAKE_CURRENT_BINARY_DIR}/${outputFileName}")
	list(APPEND ${depsList} "${name}" "${CMAKE_CURRENT_BINARY_DIR}/${outputFileName}")
endmacro()


macro(_process_single_shader name outputName outputSuffix defines dependencyList)

	# make sure target directory exists
	get_filename_component(directory ${outputName} DIRECTORY)
	if(directory)
		file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${directory}")
	endif()

	string(STRIP "${defines}" defines2)
	separate_arguments(defines2 UNIX_COMMAND "${defines2}")
	if("${defines}" STREQUAL "")
		set(commentText "Converting ${name} (defines: none) to spir-v...")
	else()
		set(commentText "Converting ${name} (defines: ${defines2}) to spir-v...")
	endif()

	add_custom_command(COMMENT "${commentText}"
	                   MAIN_DEPENDENCY "${name}"
	                   OUTPUT "${outputName}${outputSuffix}"
	                   COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}"
		                           --target-env vulkan1.2  # Vulkan 1.2 is required for VK_EXT_buffer_device_address
		                           -x  # save binary output as text-based hexadecimal numbers
		                           ${defines2}  # define pre-processor macros
		                           ${CMAKE_CURRENT_SOURCE_DIR}/${name}  # source file
		                           -o ${outputName}${outputSuffix})  # output file

	source_group("Shaders" FILES "${name}")
	source_group("Shaders/spir-v" FILES "${CMAKE_CURRENT_BINARY_DIR}/${outputName}${outputSuffix}")
	list(APPEND ${dependencyList} "${name}" "${CMAKE_CURRENT_BINARY_DIR}/${outputName}${outputSuffix}")

endmacro()


macro(process_shaders shaderList dependencyList)

	# process CADR_SHADERS line by line
	foreach(line ${shaderList})

		# get shader file name
		separate_arguments(stringList UNIX_COMMAND ${line})
		list(LENGTH stringList listLen)
		list(GET stringList 0 name)

		# process shaders
		if(listLen LESS 3)
			set(outputName "${name}")
			set(outputSuffix ".spv")
			_process_single_shader("${name}" "${outputName}" "${outputSuffix}" "" ${dependencyList})
		else()
			list(GET stringList 1 outputName)
			list(GET stringList 2 outputSuffix)
			if(listLen EQUAL 3)
				set(extraDefines "")
			else()
				list(GET stringList 3 extraDefines)
			endif()
			_process_single_shader("${name}" "${outputName}-overallMaterial" "${outputSuffix}" "${extraDefines}" ${dependencyList})
			_process_single_shader("${name}" "${outputName}-overallMaterial-texturing" "${outputSuffix}" "-DTEXTURING ${extraDefines}" ${dependencyList})
			_process_single_shader("${name}" "${outputName}-perVertexColor" "${outputSuffix}" "-DPER_VERTEX_COLOR ${extraDefines}" ${dependencyList})
			_process_single_shader("${name}" "${outputName}-perVertexColor-texturing" "${outputSuffix}" "-DPER_VERTEX_COLOR -DTEXTURING ${extraDefines}" ${dependencyList})
		endif()

	endforeach()
endmacro()


# helper macro for find_package_with_message
macro(find_package_with_message2 PACKAGE_NAME TARGET_NAME)

	# find package
	find_package(${PACKAGE_NAME} QUIET)

	# print result (but only once for each package)
	get_property(ALREADY_REPORTED GLOBAL PROPERTY ${PACKAGE_NAME}_FOUND_INFO_REPORTED)
	if(NOT ALREADY_REPORTED)
		set_property(GLOBAL PROPERTY ${PACKAGE_NAME}_FOUND_INFO_REPORTED True)
		if(TARGET ${TARGET_NAME})

			# print success
			cadr_get_package_path(FOUND_WHERE ${PACKAGE_NAME} ${TARGET_NAME})
			message(STATUS "Find package ${PACKAGE_NAME}: ${FOUND_WHERE}")

		else()

			# print failure
			message(STATUS "Find package ${PACKAGE_NAME}: not found")

		endif()
	endif()

	# return on find_package() failure
	# this causes currently processed file (where the macro is used) to stop processing
	if(NOT TARGET ${TARGET_NAME})
		return()
	endif()

endmacro()


# performs find_package() and prints outcome to the console
# (It uses one or two arguments. The first argument is parameter
# to find_package() and second is target name that is expected to be
# created by find_package(). If they are the same, the second
# parameter can be omitted.)
macro(find_package_with_message PACKAGE_NAME)
	find_package_with_message2("${PACKAGE_NAME}" "${ARGN}")
endmacro()
