# find OpenGL
include(${CMAKE_SOURCE_DIR}/CMakeModules/CADRMacros.cmake)
if(WIN32)
	find_package(OpenGL)
	set(OPENGL_TARGET OpenGL::GL)
else()
	set(OpenGL_GL_PREFERENCE LEGACY)
	find_package_with_message(OpenGL OpenGL::GLX)
	set(OPENGL_TARGET OpenGL::GLX)
endif()

# ignore this project when no OpenGL
if(NOT TARGET ${OPENGL_TARGET})
	return()
endif()


set(APP_NAME OpenGLInteroperability)

project(${APP_NAME})

set(APP_SOURCES
	main.cpp
	)

set(APP_INCLUDES
	)

set(APP_SHADERS
	twoTriangles.vert
	twoTriangles.frag
	merge.vert
	merge.frag
	)

add_shaders("${APP_SHADERS}" APP_SHADER_DEPS)
add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES} ${APP_SHADER_DEPS})

# dependencies
find_package(Vulkan REQUIRED)
if(UNIX)
	find_package(X11 REQUIRED)
	set(deps Vulkan::Vulkan OpenGL::GLX X11)
elseif(WIN32)
	set(deps Vulkan::Vulkan OpenGL::GL)
endif()

target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${APP_NAME} ${deps})
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
