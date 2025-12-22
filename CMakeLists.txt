cmake_minimum_required(VERSION 3.21.0) # arguments for discussion about minimal cmake version:
                                       # CXX_STANDARD 20 is available since 3.12,
                                       # INTERFACE_LINK_OPTIONS needs at least 3.13,
                                       # Ubuntu 20.04 LTS uses 3.16.3,
                                       # cmake_path was introduced in version 3.20,
                                       # Vulkan::Headers and Vulkan::glslangValidator targets are available since 3.21,
                                       # Ubuntu 22.04 LTS uses 3.22.1
cmake_policy(SET CMP0167 OLD) # for CMake 3.30 and newer, keep FindBoost.cmake working instead of new BoostConfig.cmake

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
set(CMAKE_MODULE_PATH "${CADR_SOURCE_DIR}/CMakeModules/;${CMAKE_MODULE_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/")
set(THIRD_PARTY_DIR "${CADR_SOURCE_DIR}/3rdParty/" CACHE STRING "3rdParty directory.")
set(INSTALL_CMAKE_CONFIG_DIR cmake)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# MSVC
if(MSVC)

	# enable exceptions
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

	# tell MSVC that source files are utf-8 encoded
	# (it sets source charset and execution charset to utf-8)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

	# avoid min and max macros
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DNOMINMAX")

	# disable warning C4251: class T1 (usually std::vector or so) needs to have dll-interface to be used by clients of class T2 (class containing member T1)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251")

	# no support for MSVC prior to 2015
	if(MSVC_VERSION LESS 1900)
		message(FATAL_ERROR "Microsoft Visual C++ 2013 and earlier are not supported "
		                    "because of their limited support of C++11 and C++14.")
	elseif(MSVC_VERSION LESS 1910)
		# MSVC 2015 is exactly 1900
	elseif(MSVC_VERSION LESS 1920)
		# MSVC 2017 is range 1910 to 1916
	elseif(MSVC_VERSION LESS 1999)
		# MSVC 2019 is range 1920 to 19??
	endif()

endif()

# g++
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wpedantic")
endif()

# src
add_subdirectory(src)

# src
add_subdirectory(tests)

# build examples
if(CADR_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()
