set(APP_NAME VulkanRenderingPerformance)

project(${APP_NAME})

set(APP_SOURCES
	main.cpp
	)

set(APP_INCLUDES
	)

set(APP_SHADERS
	attributelessConstantOutput.vert
	attributelessInputIndices.vert
	passThrough.vert
	singleUniformMatrix.vert
	matrixBuffer.vert
	shader.frag
	)

add_shaders("${APP_SHADERS}" APP_SHADER_DEPS)
add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES} ${APP_SHADER_DEPS})

# dependencies
find_package(Vulkan REQUIRED)
if(UNIX)
	find_package(X11 REQUIRED)
	set(deps Vulkan X11)
elseif(WIN32)
	set(deps Vulkan)
endif()

target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${APP_NAME} ${deps})
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
