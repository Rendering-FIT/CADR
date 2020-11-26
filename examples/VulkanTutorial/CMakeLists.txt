# folder name in IDE (Visual C++,...) where all examples will go
set(examples_folder_name "Examples/VulkanTutorial")

set(PACKAGES
	01-deviceList
	02-developmentSetup
	03-funcPointers
	#03-funcPointers/dl - not compatible with the recent changes in vulkan.hpp
	04-deviceInfo
	05-commandSubmission
	06-simpleImage
	07-robustImage
	08-helloTriangle
	)

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
