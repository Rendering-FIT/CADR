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
	"shaders/phongUberShader.vert      shaders/phong           .vert.spv"
	"shaders/phongUberShader.frag      shaders/phong           .frag.spv"
	"shaders/phongUberShader.vert      shaders/phongPoint      .vert.spv  -DPOINT_SHADER"
	"shaders/baseColorUberShader.vert  shaders/baseColor       .vert.spv"
	"shaders/baseColorUberShader.frag  shaders/baseColor       .frag.spv"
	"shaders/baseColorUberShader.vert  shaders/baseColorPoint  .vert.spv  -DPOINT_SHADER"
	)

# dependencies
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
if(UNIX)
	set(libs Vulkan::Headers glm nlohmann_json::nlohmann_json stdc++fs)
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
set_property(TARGET ${APP_NAME} PROPERTY CXX_STANDARD 20)

# copy DLLs (SDL3.dll, SDL2.dll, glfw3.dll, Qt stuff,...)
if(WIN32)
	if(${GUI_TYPE} STREQUAL "SDL3" AND SDL3_DLL)
		add_custom_command(TARGET ${APP_NAME}
			POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL3_DLL}" $<TARGET_FILE_DIR:${APP_NAME}>)
	elseif(${GUI_TYPE} STREQUAL "SDL2" AND SDL2_DLL)
		add_custom_command(TARGET ${APP_NAME}
			POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_DLL}" $<TARGET_FILE_DIR:${APP_NAME}>)
	elseif(${GUI_TYPE} STREQUAL "GLFW" AND glfw3_DLL)
		add_custom_command(TARGET ${APP_NAME}
			POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${glfw3_DLL}" $<TARGET_FILE_DIR:${APP_NAME}>)
	elseif(${GUI_TYPE} STREQUAL "Qt6")
		add_custom_command(TARGET ${APP_NAME}
			POST_BUILD COMMAND Qt6::windeployqt
					--no-translations  # skip Qt translations
					--no-opengl-sw  # skip software OpenGL
					--no-system-d3d-compiler  # skip D3D stuff
					--no-svg  # skip svg support
					--no-compiler-runtime # skip VC runtime DLLs
					\"$<TARGET_FILE:${APP_NAME}>\"
					COMMENT "Deploying Qt related dependencies...")
	elseif(${GUI_TYPE} STREQUAL "Qt5")
		add_custom_command(TARGET ${APP_NAME}
			POST_BUILD COMMAND "${QT5_WINDEPLOYQT_EXECUTABLE}"
					--no-translations  # skip Qt translations
					--no-widgets  # skip Qt widgets
					--no-opengl-sw  # skip software OpenGL
					--no-angle  # skip software OpenGL (ANGLE)
					--no-compiler-runtime # skip VC runtime DLLs
					\"$<TARGET_FILE:${APP_NAME}>\"
					COMMENT "Deploying Qt related dependencies...")
	endif()
endif()
