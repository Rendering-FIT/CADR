cmake_minimum_required(VERSION 3.1.0)

project(CADR)

# dynamic or static library
option(CADR_DYNAMIC_LINKAGE "Build CADR as libs for dynamic linkage. Use OFF for static libs." ON)

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
   # no support for MSVC 2013 and earlier
   if(MSVC_VERSION LESS 1900)
      message(FATAL_ERROR "Microsoft Visual C++ 2013 and earlier are not supported "
              "as their support of C++11 is too limited.")
   else()
      # MSVC 2015 workarounds to be placed here (currently none)
   endif()
endif()

# g++
if(CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -W -Wall -pedantic")
endif()

# src
add_subdirectory(src)
