cmake_minimum_required(VERSION 3.5.0) # arguments for particular cmake version: Ubuntu 16.04 LTS uses version 3.5, version 3.7 supports LESS_EQUAL, Ubuntu 18.04 LTS uses version 3.10

project(CADR)

# dynamic or static library
option(CADR_DYNAMIC_LINKAGE "Build CADR as libs for dynamic linkage. Use OFF for static libs." ON)

# build examples
set(CADR_BUILD_EXAMPLES OFF CACHE BOOL "Build examples (examples directory).")

# file postfixes
set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually d on windows")
set(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_RELWITHDEBINFO_POSTFIX "rd" CACHE STRING "add a postfix, usually empty on windows")
set(CMAKE_MINSIZEREL_POSTFIX "s" CACHE STRING "add a postfix, usually empty on windows")

# project settings
set(CMAKE_MODULE_PATH "${CADR_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
set(INSTALL_CMAKE_CONFIG_DIR cmake)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# MSVC
if(MSVC)
	# no support for MSVC prior to 2017
	if(MSVC_VERSION LESS 1910)
		message(FATAL_ERROR "Microsoft Visual C++ 2015 and earlier are not supported "
		                    "for their limited support of C++11 and C++14.")
	else()

		# enable exceptions
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

	endif()
endif()

# g++
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -W -Wall -pedantic")
endif()

# src
add_subdirectory(src)

# build examples
if(CADR_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()
