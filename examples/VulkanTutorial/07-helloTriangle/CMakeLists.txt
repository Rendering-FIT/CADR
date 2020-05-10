set(APP_NAME tut05-helloTriangle)

project(${APP_NAME})

set(APP_SOURCES
    main.cpp
   )

set(APP_INCLUDES
   )

set(APP_SHADERS
    shader.vert
    shader.frag
   )

add_shaders("${APP_SHADERS}" APP_SHADER_DEPS)
add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES} ${APP_SHADER_DEPS})

# dependencies
find_package(Vulkan REQUIRED)

target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${APP_NAME} Vulkan::Vulkan)
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
