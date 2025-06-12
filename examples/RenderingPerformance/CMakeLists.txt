set(APP_NAME RenderingPerformance)

project(${APP_NAME})

set(APP_SOURCES
    main.cpp
    Tests.cpp
    VulkanWindow.cpp
   )

set(APP_INCLUDES
    Tests.h
    VulkanWindow.h
   )

# dependencies
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
set(libs Vulkan::Headers glm)

# shaders
include(${CMAKE_SOURCE_DIR}/CMakeModules/CADRMacros.cmake)
add_shader(shader.vert "" shader.vert.spv APP_SHADER_DEPS)
add_shader(shader.vert -DID_BUFFER shader-idBuffer.vert.spv APP_SHADER_DEPS)
add_shader(shader.frag "" shader.frag.spv APP_SHADER_DEPS)
add_shader(shader.frag -DID_BUFFER shader-idBuffer.frag.spv APP_SHADER_DEPS)

# GUI dependencies
include(VulkanWindow.cmake)
VulkanWindowConfigure(APP_SOURCES APP_INCLUDES libs defines includes)

# target
add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES} ${APP_SHADER_DEPS})
target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${includes})
target_compile_definitions(${APP_NAME} PRIVATE ${defines})
target_link_libraries(${APP_NAME} ${libs} CadR)
set_property(TARGET ${APP_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
