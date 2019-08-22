set(APP_NAME tut03-helloWindowX11)

project(${APP_NAME})

set(APP_SOURCES
    main.cpp
   )

set(APP_INCLUDES
   )

add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES})

# dependencies
find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED)

target_link_libraries(${APP_NAME} Vulkan::Vulkan X11)
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
