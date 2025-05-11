# project name
set(LIB_NAME CadText)
set(LIB_NAME_UPPER)
string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)

# linkage
if(CADTEXT_DYNAMIC_LINKAGE)
	set(CADTEXT_USER_DEFINED_DYNAMIC_OR_STATIC "SHARED")
	add_definitions(-D${LIB_NAME_UPPER}_LIBRARY)
else()
	set(CADTEXT_USER_DEFINED_DYNAMIC_OR_STATIC "STATIC")
	add_definitions(-D${LIB_NAME_UPPER}_LIBRARY_STATIC)
endif()

# public headers
set(CADTEXT_PUBLIC_HEADERS
	TextRenderer.h
	TriangulationTextRenderer.h
	TessellationShadersTextRenderer.h
	WindingNumberTextRenderer.h
	SdfTextRenderer.h
	)

# private headers
set(CADTEXT_PRIVATE_HEADERS
	)

# sources
set(CADTEXT_SOURCES
	TextRenderer.cpp
	TriangulationTextRenderer.cpp
	TessellationShadersTextRenderer.cpp
	WindingNumberTextRenderer.cpp
	SdfTextRenderer.cpp
	)

# shaders
set(CADTEXT_SHADERS
	shaders/triangle.vert
	shaders/triangle.frag
	shaders/curve.vert
	shaders/curve.tesc
	shaders/curve.tese
	shaders/curve.frag
	shaders/windingNumber.vert
	shaders/windingNumber.frag
	shaders/sdf.vert
	shaders/sdf.frag
	)

# grouping of source files
source_group("Code" FILES ${CADTEXT_PUBLIC_HEADERS} ${CADTEXT_PRIVATE_HEADERS} ${CADTEXT_SOURCES})

# dependencies
find_package(vfont REQUIRED)
find_package(glm REQUIRED)
find_package(freetype REQUIRED)
find_package(harfbuzz REQUIRED)

# compile shaders
add_shaders("${CADTEXT_SHADERS}" CADTEXT_SHADER_DEPS)

# CadText library
add_library(${LIB_NAME}
	${CADTEXT_USER_DEFINED_DYNAMIC_OR_STATIC}
	${CADTEXT_PUBLIC_HEADERS}
	${CADTEXT_PRIVATE_HEADERS}
	${CADTEXT_SOURCES}
	${CADTEXT_SHADER_DEPS}
	)

# compile definitions
if(WIN32)
	if(CADTEXT_DYNAMIC_LINKAGE)
		target_compile_definitions(${LIB_NAME}
			PRIVATE
				"CADTEXT_EXPORT=__declspec(dllexport)"
			INTERFACE
				"CADTEXT_EXPORT=__declspec(dllimport)"
		)
	else()
		target_compile_definitions(${LIB_NAME}
			PUBLIC
				"CADTEXT_EXPORT="
		)
	endif()
else()
	target_compile_definitions(${LIB_NAME}
		PUBLIC
			"CADTEXT_EXPORT=__attribute__((visibility(\"default\")))"
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
target_link_libraries(${LIB_NAME} CadR vfont::vfont glm freetype::freetype harfbuzz::harfbuzz)
if(UNIX)
	target_link_libraries(${LIB_NAME} dl stdc++fs)
endif()

# other target settings
set_property(TARGET ${LIB_NAME} PROPERTY CXX_STANDARD 20)

# headers installation
install(FILES
	${CADTEXT_PUBLIC_HEADERS}
	DESTINATION include/${LIB_NAME}
	)

# libraries installation
install(TARGETS ${LIB_NAME}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
	)

# cmake configuration file
# configure_file(
# 	CADRConfig.cmake.in
# 	CADRConfig.cmake
# 	@ONLY
# 	)

# install cmake configuration file
# install(FILES "${CMAKE_CURRENT_BINARY_DIR}/CADRConfig.cmake"
#         DESTINATION "${INSTALL_CMAKE_CONFIG_DIR}" # INSTALL_CMAKE_CONFIG_DIR is defined in master CMakeLists file
#         )
