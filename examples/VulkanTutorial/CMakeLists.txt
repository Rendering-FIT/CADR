# folder name in IDE (Visual C++,...) where all examples will go
set(examples_folder_name "Examples/VulkanTutorial")

set(PACKAGES
	01-deviceList
	02-developmentSetup
	)

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
