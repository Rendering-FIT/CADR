set(APP_NAME HelloQtWindow)

project(${APP_NAME})

set(APP_SOURCES
	main.cpp
	VulkanWindow.cpp
	)

set(APP_INCLUDES
	VulkanWindow.h
	)

set(APP_SHADERS
	shader.vert
	shader.frag
	)

add_shaders("${APP_SHADERS}" APP_SHADER_DEPS)
add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES} ${APP_SHADER_DEPS})

# dependencies
find_package(Vulkan REQUIRED)
find_package(Qt5Gui REQUIRED)
if(UNIX)
	find_package(Qt5X11Extras REQUIRED)
	find_package(X11 REQUIRED)
	set(deps Vulkan Qt5::Gui Qt5::X11Extras X11)
elseif(WIN32)
	set(deps Vulkan Qt5::Gui)
endif()

target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${APP_NAME} ${deps})
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
