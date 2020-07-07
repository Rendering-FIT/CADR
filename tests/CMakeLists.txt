# folder name in IDE (Visual C++,...) where all examples will go
set(tests_folder_name "Tests")

set(APP_NAME ParentChildTest)

project(${APP_NAME})

set(APP_SOURCES
    ParentChildTest.cpp
   )

set(APP_INCLUDES
   )

add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES})

# dependencies
find_package(Vulkan REQUIRED)

target_link_libraries(${APP_NAME} CadR)
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${tests_folder_name}")
