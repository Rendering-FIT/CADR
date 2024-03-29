# find package nlohmann_json
# if not found, do not include this example in build
include(${CMAKE_SOURCE_DIR}/CMakeModules/CADRMacros.cmake)
find_package_with_message(nlohmann_json nlohmann_json::nlohmann_json)


# glTFreader project
set(APP_NAME gltfReader)
project(${APP_NAME})

set(APP_SOURCES
    main.cpp
    Window.cpp
   )

set(APP_INCLUDES
    Window.h
   )

set(APP_SHADERS
    shaders/coordinates.vert
    shaders/unspecifiedMaterial.frag
   )

add_shaders("${APP_SHADERS}" APP_SHADER_DEPS)
add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES} ${APP_SHADER_DEPS})

# dependencies
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
if(UNIX)
	find_package(X11 REQUIRED)
	set(deps Vulkan::Headers glm nlohmann_json::nlohmann_json stdc++fs X11)
elseif(WIN32)
	set(deps Vulkan::Headers glm nlohmann_json::nlohmann_json)
endif()

target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${APP_NAME} ${deps} CadR)
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
