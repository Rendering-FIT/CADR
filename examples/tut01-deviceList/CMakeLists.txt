set(APP_NAME tut01-deviceList)

project(${APP_NAME})

set(APP_SOURCES
    main.cpp
   )

set(APP_INCLUDES
   )

add_executable(${APP_NAME} WIN32 ${APP_SOURCES} ${APP_INCLUDES})

# dependencies
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

target_link_libraries(${APP_NAME} Vulkan glm)
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
