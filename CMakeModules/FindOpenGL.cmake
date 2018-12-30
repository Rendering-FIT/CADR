#
# Module for finding OpenGL
#
# It executes standard FindOpenGL.cmake
# while adding OPENGL_GLEXT_INCLUDE_DIR for including GL/glext.h
#


# find GL/glext.h include path
find_path(OPENGL_GLEXT_INCLUDE_DIR GL/glext.h
	/usr/include
	/usr/local/include
	/opt/local/include
)

# do not process anything and do not create targets if GL/glext.h was not found
include(CADRMacros)
if(NOT OPENGL_GLEXT_INCLUDE_DIR)
	cadr_report_find_status()
	return()
endif()


# handle REQUIRED and QUIET flags
if(${${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED})
	list(APPEND flags REQUIRED)
endif()
if(${${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY})
	list(APPEND flags QUIET)
endif()

# perform find_package on standard FindOpenGL.cmake
# (to call standard FindOpenGL.cmake we have to set CMAKE_MODULE_PATH to empty string
# otherwise this file would be called)
set(SAVED_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH "")
find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} ${flags})
set(CMAKE_MODULE_PATH ${SAVED_CMAKE_MODULE_PATH})

# append GL/glext.h to include directories of targets
if(TARGET OpenGL::GL)
	get_target_property(dirs OpenGL::GL INTERFACE_INCLUDE_DIRECTORIES)
	set(dirs "${dirs};${OPENGL_GLEXT_INCLUDE_DIR}")
	set_target_properties(OpenGL::GL PROPERTIES
	                      INTERFACE_INCLUDE_DIRECTORIES "${dirs}")
endif()
if(TARGET OpenGL::GLX)
	get_target_property(dirs OpenGL::GLX INTERFACE_INCLUDE_DIRECTORIES)
	set(dirs "${dirs};${OPENGL_GLEXT_INCLUDE_DIR}")
	set_target_properties(OpenGL::GLX PROPERTIES
	                      INTERFACE_INCLUDE_DIRECTORIES "${dirs}")
endif()


# message
include(CADRMacros)
cadr_report_find_status(OpenGL::GL)
