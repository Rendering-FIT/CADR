# project name
set(LIB_NAME CadPL)
set(LIB_NAME_UPPER)
string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)

# include CadR macros
include(${CMAKE_SOURCE_DIR}/CMakeModules/CADRMacros.cmake)

# linkage
if(CADR_DYNAMIC_LINKAGE)
	set(CADR_USER_DEFINED_DYNAMIC_OR_STATIC "SHARED")
	add_definitions(-D${LIB_NAME_UPPER}_LIBRARY)
else()
	set(CADR_USER_DEFINED_DYNAMIC_OR_STATIC "STATIC")
	add_definitions(-D${LIB_NAME_UPPER}_LIBRARY_STATIC)
endif()

# public headers
set(CADPL_PUBLIC_HEADERS
	PipelineSceneGraph.h
	PipelineLibrary.h
	ShaderGenerator.h
	ShaderLibrary.h
	)

# private headers
set(CADPL_PRIVATE_HEADERS
	)

# sources
set(CADPL_SOURCES
	PipelineSceneGraph.cpp
	PipelineLibrary.cpp
	ShaderGenerator.cpp
	ShaderLibrary.cpp
	)

# grouping of source files
source_group("Code" FILES ${CADPL_PUBLIC_HEADERS} ${CADPL_PRIVATE_HEADERS} ${CADPL_SOURCES})

# dependencies
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

# shaders
set(CADPL_SHADER_DEPS  UberShaderInterface.glsl UberShaderReadFuncs.glsl)
source_group("Shaders" FILES ${CADPL_SHADER_DEPS})
add_shader(UberShader.vert  ""  shaders/UberShader.vert.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.vert  "-DID_BUFFER"  shaders/UberShader-idBuffer.vert.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.geom  "-DTRIANGLES"  shaders/UberShaderTriangles.geom.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.geom  "-DLINES"      shaders/UberShaderLines.geom.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.geom  "-DID_BUFFER;-DTRIANGLES"  shaders/UberShaderTriangles-idBuffer.geom.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.geom  "-DID_BUFFER;-DLINES"      shaders/UberShaderLines-idBuffer.geom.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.frag  "-DTRIANGLES"  shaders/UberShaderTriangles.frag.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.frag  "-DLINES"      shaders/UberShaderLines.frag.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.frag  "-DID_BUFFER;-DTRIANGLES"  shaders/UberShaderTriangles-idBuffer.frag.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.frag  "-DID_BUFFER;-DLINES"      shaders/UberShaderLines-idBuffer.frag.spv  CADPL_SHADER_DEPS)
add_shader(UberShaderPoints.vert  ""  shaders/UberShaderPoints.vert.spv  CADPL_SHADER_DEPS)
add_shader(UberShaderPoints.vert  "-DID_BUFFER"  shaders/UberShaderPoints-idBuffer.vert.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.frag  "-DPOINTS"  shaders/UberShaderPoints.frag.spv  CADPL_SHADER_DEPS)
add_shader(UberShader.frag  "-DID_BUFFER;-DPOINTS"  shaders/UberShaderPoints-idBuffer.frag.spv  CADPL_SHADER_DEPS)

# CADR library
add_library(${LIB_NAME}
	${CADR_USER_DEFINED_DYNAMIC_OR_STATIC}
	${CADPL_PUBLIC_HEADERS}
	${CADPL_PRIVATE_HEADERS}
	${CADPL_SOURCES}
	${CADPL_SHADER_DEPS}
	)

# compile definitions
if(WIN32)
	if(CADR_DYNAMIC_LINKAGE)
		target_compile_definitions(${LIB_NAME}
			PRIVATE
				"CADPL_EXPORT=__declspec(dllexport)"
			INTERFACE
				"CADPL_EXPORT=__declspec(dllimport)"
		)
	else()
		target_compile_definitions(${LIB_NAME}
			PUBlIC
				"CADPL_EXPORT="
		)
	endif()
else()
	target_compile_definitions(${LIB_NAME}
		PUBLIC
			"CADPL_EXPORT=__attribute__((visibility(\"default\")))"
	)
endif()

# target includes
get_filename_component(parent_dir "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
set_target_properties(${LIB_NAME} PROPERTIES
	INCLUDE_DIRECTORIES "${parent_dir}"
	INTERFACE_INCLUDE_DIRECTORIES "${parent_dir}"
	)
target_include_directories(${LIB_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# target libraries
target_link_libraries(${LIB_NAME} Vulkan::Headers glm CadR)

# other target settings
set_property(TARGET ${LIB_NAME} PROPERTY CXX_STANDARD 17)

# headers installation
install(FILES
	${CADPL_PUBLIC_HEADERS}
	DESTINATION include/${LIB_NAME}
	)

# libraries installation
install(TARGETS ${LIB_NAME}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
	)

# cmake configuration file
# FIXME: is this working or broken?
#configure_file(
#	CADRConfig.cmake.in
#	CADRConfig.cmake
#	@ONLY
#	)

# install cmake configuration file
# FIXME: is this working or broken?
#install(FILES "${CMAKE_CURRENT_BINARY_DIR}/CADRConfig.cmake"
#        DESTINATION "${INSTALL_CMAKE_CONFIG_DIR}" # INSTALL_CMAKE_CONFIG_DIR is defined in master CMakeLists file
#        )
