# folder name in IDE (Visual C++,...) where all examples will go
set(examples_folder_name "Examples")

set(PACKAGES
	tut02-funcPointers
	tut02-vulkanInfo
	tut04-robustImage
	tut05-helloTriangle
	tut04-swapChain
	tut05-resizableWindow
	tut06-helloTriangle
	VulkanDepthBuffer
	OpenGLInteroperability
	HelloQtWindow
	VulkanAttributesAndUniforms
	VulkanRenderingPerformance
	VulkanDynamicFuncs
	gltfReader
	)
if(UNIX)
	list(INSERT PACKAGES 2 tut03-helloWindowX11)
elseif(WIN32)
	list(INSERT PACKAGES 2 tut03-helloWindowWin)
endif()

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
add_subdirectory(VulkanTutorial)
