# project name
set(LIB_NAME CADR)
set(LIB_NAME_UPPER)
string(TOUPPER ${LIB_NAME} LIB_NAME_UPPER)

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
    AttribConfig.h
    AttribStorage.h
    Export.h
    Buffer.h
    Drawable.h
    Factory.h
    Geode.h
    )

# private headers
set(CADR_PRIVATE_HEADERS
    )

# sources
set(CADR_SOURCES
    AttribStorage.cpp
    DrawCommand.cpp
    Factory.cpp
    Renderer.cpp
    )

# CADR library
add_library(${LIB_NAME}
    ${GPUENGINE_USER_DEFINED_DYNAMIC_OR_STATIC}
    ${CADR_PUBLIC_HEADERS}
    ${CADR_PRIVATE_HEADERS}
    ${CADR_SOURCES}
    )

# dependencies
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

# target dependencies
#
# Internal_deps - only 'ge' targets goes here (e.g. geCore), it configures this package intra project dependencies and also configures the config file
# External_deps_Export - other than 'ge' packages goes here (e.g. glew), it configures this package extra project dependencies and also configures the config file
# External_libs - external libs or targets to link with
# Internal_inc - additional include directories
# Includes_to_export - includes to be exported from this target to application
get_filename_component(parent_dir "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
set(internal_deps)
set(external_deps_export glm)
set(external_libs glm)
set(internal_includes "${parent_dir}")
set(includes_to_export "${parent_dir}")

# target libraries
target_link_libraries(${LIB_NAME} ${internal_deps} ${external_libs})

# target includes
set_target_properties(${LIB_NAME} PROPERTIES
   INCLUDE_DIRECTORIES "${internal_includes}"
   INTERFACE_INCLUDE_DIRECTORIES "${includes_to_export}"
   )

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
