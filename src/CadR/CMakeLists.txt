# project name
set(LIB_NAME CadR)
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
    AttribSizeList.h
    AttribStorage.h
    BufferData.h
    CadR.h
    CallbackList.h
    Export.h
    Mesh.h
    PrimitiveSet.h
    Renderer.h
    StagingBuffer.h
    VulkanDevice.h
    VulkanInstance.h
    VulkanLibrary.h
   )

# private headers
set(CADR_PRIVATE_HEADERS
   )

# sources
set(CADR_SOURCES
    AttribStorage.cpp
    CadR.cpp
    PrimitiveSet.cpp
    Renderer.cpp
    StagingBuffer.cpp
    VulkanDevice.cpp
    VulkanInstance.cpp
    VulkanLibrary.cpp
   )

# CADR library
add_library(${LIB_NAME}
    ${CADR_USER_DEFINED_DYNAMIC_OR_STATIC}
    ${CADR_PUBLIC_HEADERS}
    ${CADR_PRIVATE_HEADERS}
    ${CADR_SOURCES}
    )

# dependencies
find_package(Vulkan REQUIRED)
find_package(Boost REQUIRED)

# target libraries
target_link_libraries(${LIB_NAME} Vulkan::Headers Boost::boost)
if(UNIX)
	target_link_libraries(${LIB_NAME} dl)
endif()

# target includes
get_filename_component(parent_dir "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
set_target_properties(${LIB_NAME} PROPERTIES
   INCLUDE_DIRECTORIES "${parent_dir}"
   INTERFACE_INCLUDE_DIRECTORIES "${parent_dir}"
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
