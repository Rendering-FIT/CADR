# folder name in IDE (Visual C++,...) where all examples will go
set(examples_folder_name "Examples/VulkanTutorial")

set(PACKAGES
	01-deviceList
	02-developmentSetup
	03-funcPointers
	04-deviceInfo
	05-commandSubmission
	05-simpleImage
	06-robustImage
	07-helloTriangle
	)

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
