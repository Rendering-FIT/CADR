#
# find Wayland
#
# targets:
#    Wayland::client
#    Wayland::cursor
#
# variables:
#    Wayland_client_FOUND
#    Wayland_INCLUDE_DIR
#    Wayland_client_INCLUDE_DIR
#    Wayland_server_INCLUDE_DIR
#    Wayland_cursor_INCLUDE_DIR
#    Wayland_client_LIBRARY
#    Wayland_server_LIBRARY
#    Wayland_cursor_LIBRARY
#    Wayland_SCANNER
#    Wayland_PROTOCOLS_DIR
#


# find paths
find_path(Wayland_client_INCLUDE_DIR NAMES wayland-client.h)
find_path(Wayland_server_INCLUDE_DIR NAMES wayland-server.h)
find_path(Wayland_cursor_INCLUDE_DIR NAMES wayland-cursor.h)
find_library(Wayland_client_LIBRARY  NAMES wayland-client)
find_library(Wayland_server_LIBRARY  NAMES wayland-server)
find_library(Wayland_cursor_LIBRARY  NAMES wayland-cursor)
find_program(Wayland_SCANNER         NAMES wayland-scanner)

# Wayland include dir
set(Wayland_INCLUDE_DIR ${Wayland_client_INCLUDE_DIR} ${Wayland_server_INCLUDE_DIR})
list(REMOVE_DUPLICATES Wayland_INCLUDE_DIR)

# Wayland protocols directory
find_package(PkgConfig QUIET)
pkg_check_modules(Wayland_PROTOCOLS wayland-protocols QUIET)
if(PKG_CONFIG_FOUND AND Wayland_PROTOCOLS_FOUND)
	pkg_get_variable(Wayland_PROTOCOLS_DIR wayland-protocols pkgdatadir)
endif()
set(Wayland_PROTOCOLS_DIR "${Wayland_PROTOCOLS_DIR}" CACHE PATH "Wayland protocols directory.")

# handle required variables and set ${CMAKE_FIND_PACKAGE_NAME}_FOUND variable
include(FindPackageHandleStandardArgs)
if(Wayland_client_INCLUDE_DIR OR NOT Wayland_server_INCLUDE_DIR)
	set(Wayland_include_var Wayland_client_INCLUDE_DIR)
else()
	set(Wayland_include_var Wayland_server_INCLUDE_DIR)
endif()
if(Wayland_client_LIBRARY OR NOT Wayland_server_LIBRARY)
	set(Wayland_library_var Wayland_client_LIBRARY)
else()
	set(Wayland_library_var Wayland_server_LIBRARY)
endif()
string(CONCAT errorMessage
	"Finding of package ${CMAKE_FIND_PACKAGE_NAME} failed. Make sure Wayland development files "
	"are installed. Then, make sure relevant Wayland_* variables are set properly. "
	"Either client or server include dir must be specified, and either client or server library "
	"needs to be provided. Wayland_client_INCLUDE_DIR and Wayland_client_LIBRARY is what most "
	"people will use. Wayland_PROTOCOLS_DIR also must be set.")
find_package_handle_standard_args(
	${CMAKE_FIND_PACKAGE_NAME}
	REQUIRED_VARS ${Wayland_include_var} ${Wayland_library_var} Wayland_PROTOCOLS_DIR
	REASON_FAILURE_MESSAGE ${errorMessage}
)
unset(Wayland_library_var)

# Wayland::client target
if(Wayland_client_INCLUDE_DIR AND Wayland_client_LIBRARY)
	set(Wayland_client_FOUND TRUE)
	if(NOT TARGET Wayland::client)
		add_library(Wayland::client UNKNOWN IMPORTED)
		set_target_properties(Wayland::client PROPERTIES
			IMPORTED_LOCATION "${Wayland_client_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${Wayland_client_INCLUDE_DIR}")
	endif()
endif()

# Wayland::cursor target
if(Wayland_cursor_INCLUDE_DIR AND Wayland_cursor_LIBRARY)
	set(Wayland_cursor_FOUND TRUE)
	if(NOT TARGET Wayland::cursor)
		add_library(Wayland::cursor UNKNOWN IMPORTED)
		set_target_properties(Wayland::cursor PROPERTIES
			IMPORTED_LOCATION "${Wayland_cursor_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${Wayland_cursor_INCLUDE_DIR}")
	endif()
endif()
