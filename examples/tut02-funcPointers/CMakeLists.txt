set(APP_NAME tut02-funcPointers)

project(${APP_NAME})

set(APP_SOURCES
    main.cpp
   )

set(APP_INCLUDES
   )

add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES})

# dependencies
find_package(Vulkan REQUIRED)
target_link_libraries(${APP_NAME} Vulkan::Vulkan)
if(NOT WIN32)
	target_link_libraries(${APP_NAME} dl)
endif()

set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
