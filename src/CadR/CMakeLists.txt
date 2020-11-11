# project name
set(LIB_NAME CadR)
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
set(CADR_PUBLIC_HEADERS
    AllocationManagers.h
    AttribSizeList.h
    CallbackList.h
    Drawable.h
    Errors.h
    Geometry.h
    ParentChildList.h
    Pipeline.h
    PrimitiveSet.h
    Renderer.h
    RenderPass.h
    StagingBuffer.h
    StateSet.h
    VertexStorage.h
    VulkanDevice.h
    VulkanInstance.h
    VulkanLibrary.h
   )

# private headers
set(CADR_PRIVATE_HEADERS
   )

# sources
set(CADR_SOURCES
    AllocationManagers.cpp
    Drawable.cpp
    Geometry.cpp
    Pipeline.cpp
    PrimitiveSet.cpp
    Renderer.cpp
    RenderPass.cpp
    StagingBuffer.cpp
    StateSet.cpp
    VertexStorage.cpp
    VulkanDevice.cpp
    VulkanInstance.cpp
    VulkanLibrary.cpp
   )

# shaders
set(CADR_SHADERS
    shaders/processDrawables.comp
   )

# CADR library
add_shaders("${CADR_SHADERS}" CADR_SHADER_DEPS)
add_library(${LIB_NAME}
    ${CADR_USER_DEFINED_DYNAMIC_OR_STATIC}
    ${CADR_PUBLIC_HEADERS}
    ${CADR_PRIVATE_HEADERS}
    ${CADR_SOURCES}
    ${CADR_SHADER_DEPS}
    )

# dependencies
find_package(Vulkan REQUIRED)
find_package(Boost REQUIRED)

# compile definitions
if(WIN32)
	if(CADR_DYNAMIC_LINKAGE)
		target_compile_definitions(${LIB_NAME}
			PRIVATE
				"CADR_EXPORT=__declspec(dllexport)"
			INTERFACE
				"CADR_EXPORT=__declspec(dllimport)"
		)
	else()
		target_compile_definitions(${LIB_NAME}
			PUBlIC
				"CADR_EXPORT="
		)
	endif()
else()
	target_compile_definitions(${LIB_NAME}
		PUBLIC
			"CADR_EXPORT=__attribute__((visibility(\"default\")))"
	)
endif()

# target libraries
target_link_libraries(${LIB_NAME} Vulkan::Headers Boost::boost)
if(UNIX)
	target_link_libraries(${LIB_NAME} dl stdc++fs)
endif()

# target includes
get_filename_component(parent_dir "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
set_target_properties(${LIB_NAME} PROPERTIES
   INCLUDE_DIRECTORIES "${parent_dir}"
   INTERFACE_INCLUDE_DIRECTORIES "${parent_dir}"
   )
target_include_directories(${LIB_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# headers installation
install(FILES
   ${CADR_PUBLIC_HEADERS}
   DESTINATION include/${LIB_NAME}
   )

# libraries installation
install(TARGETS ${LIB_NAME}
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
   RUNTIME DESTINATION bin
   )

# cmake configuration file
configure_file(
   CADRConfig.cmake.in
   CADRConfig.cmake
   @ONLY
   )

# install cmake configuration file
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/CADRConfig.cmake"
        DESTINATION "${INSTALL_CMAKE_CONFIG_DIR}" # INSTALL_CMAKE_CONFIG_DIR is defined in master CMakeLists file
        )
