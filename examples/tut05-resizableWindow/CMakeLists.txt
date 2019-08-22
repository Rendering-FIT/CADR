set(APP_NAME tut05-resizableWindow)

project(${APP_NAME})

set(APP_SOURCES
	main.cpp
	)

set(APP_INCLUDES
	)

add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES})

# dependencies
find_package(Vulkan REQUIRED)
if(UNIX)
	find_package(X11 REQUIRED)
	set(deps Vulkan::Vulkan X11)
elseif(WIN32)
	set(deps Vulkan::Vulkan)
endif()

target_link_libraries(${APP_NAME} ${deps})
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
