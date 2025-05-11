include(${CMAKE_SOURCE_DIR}/CMakeModules/CADRMacros.cmake)

# Text project
set(APP_NAME Text)
project(${APP_NAME})

set(APP_SOURCES
	main.cpp
	VulkanWindow.cpp
	)

set(APP_INCLUDES
	VulkanWindow.h
	)

# dependencies
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
if(UNIX)
	set(libs Vulkan::Headers glm stdc++fs)
elseif(WIN32)
	set(libs Vulkan::Headers glm)
endif()

# GUI dependencies
include(GuiMacros.cmake)
GuiConfigure(APP_SOURCES APP_INCLUDES libs defines vulkanWindowDefines includes)

# executable
add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES})

# target
target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${includes})
target_compile_definitions(${APP_NAME} PRIVATE ${defines})
target_link_libraries(${APP_NAME} ${libs} CadR CadText)
set_property(SOURCE VulkanWindow.cpp APPEND PROPERTY COMPILE_DEFINITIONS ${vulkanWindowDefines})
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
set_property(TARGET ${APP_NAME} PROPERTY CXX_STANDARD 20)

# copy assets
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${APP_NAME}>/assets
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/assets/roboto/Roboto-Regular.ttf" $<TARGET_FILE_DIR:${APP_NAME}>/assets
)

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
