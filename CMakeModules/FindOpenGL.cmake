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
	${THIRD_PARTY_DIR}/GL
)

# do not process anything and do not create targets if GL/glext.h was not found
include(CADRMacros)
if(NOT OPENGL_GLEXT_INCLUDE_DIR)
	cadr_report_find_status()
	return()
endif()


# perform find_package on standard FindOpenGL.cmake
# (to call standard FindOpenGL.cmake we have to set CMAKE_MODULE_PATH to empty string
# otherwise this file would be called)
set(CMAKE_MODULE_PATH_SAVED ${CMAKE_MODULE_PATH})
list(REMOVE_AT CMAKE_MODULE_PATH 0)
if(${${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED})
	find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET REQUIRED)
else()
	find_package(${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET)
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH_SAVED})

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
