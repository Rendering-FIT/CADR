# folder name in IDE (Visual C++,...) where all examples will go
set(tests_folder_name "Tests")

# dependencies
find_package(Vulkan REQUIRED)

set(APP_NAME DataAllocationTest)
project(${APP_NAME})
add_executable(${APP_NAME} DataAllocationTest.cpp)
target_link_libraries(${APP_NAME} ${deps} CadR)
set_property(TARGET ${APP_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${tests_folder_name}")

set(APP_NAME ParentChildTest)
project(${APP_NAME})
add_executable(${APP_NAME} ParentChildTest.cpp)
target_link_libraries(${APP_NAME} ${deps} CadR)
set_property(TARGET ${APP_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${tests_folder_name}")

set(APP_NAME VulkanDeviceAndInstanceTest)
project(${APP_NAME})
add_executable(${APP_NAME} VulkanDeviceAndInstanceTest.cpp)
target_link_libraries(${APP_NAME} ${deps} CadR)
set_property(TARGET ${APP_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${tests_folder_name}")
