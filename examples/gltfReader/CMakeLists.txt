# find package nlohmann_json
# if not found, do not include this example in build
include(${CMAKE_SOURCE_DIR}/CMakeModules/CADRMacros.cmake)
find_package_with_message(nlohmann_json nlohmann_json::nlohmann_json)


# glTFreader project
set(APP_NAME gltfReader)
project(${APP_NAME})

set(APP_SOURCES
	main.cpp
	PipelineLibrary.cpp
	VulkanWindow.cpp
	)

set(APP_INCLUDES
	PipelineLibrary.h
	VulkanWindow.h
	)

set(APP_SHADERS
	"shaders/phongUberShader.vert        shaders/phong        .vert.spv"
	"shaders/phongUberShader.frag        shaders/phong        .frag.spv"
	"shaders/baseColorUberShader.vert    shaders/baseColor    .vert.spv"
	"shaders/baseColorUberShader.frag    shaders/baseColor    .frag.spv"
	)

# dependencies
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
if(UNIX)
	find_package(X11 REQUIRED)
	set(libs Vulkan::Headers glm nlohmann_json::nlohmann_json stdc++fs X11)
elseif(WIN32)
	set(libs Vulkan::Headers glm nlohmann_json::nlohmann_json)
endif()

# GUI dependencies
include(GuiMacros.cmake)
GuiConfigure(APP_SOURCES APP_INCLUDES libs defines vulkanWindowDefines includes)

# executable
process_shaders("${APP_SHADERS}" APP_SHADER_DEPS)
add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES} ${APP_SHADER_DEPS})

# target
target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${includes})
target_compile_definitions(${APP_NAME} PRIVATE ${defines})
target_link_libraries(${APP_NAME} ${libs} CadR)
set_property(SOURCE VulkanWindow.cpp APPEND PROPERTY COMPILE_DEFINITIONS ${vulkanWindowDefines})
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
set_property(TARGET ${APP_NAME} PROPERTY CXX_STANDARD 17)
