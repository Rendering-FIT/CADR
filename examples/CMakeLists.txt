# folder name in IDE (Visual C++,...) where all examples will go
set(examples_folder_name "Examples")

set(PACKAGES
	tut02-vulkanInfo
	tut05-resizableWindow
	tut06-helloTriangle
	VulkanDepthBuffer
	OpenGLInteroperability
	HelloQtWindow
	VulkanAttributesAndUniforms
	VulkanRenderingPerformance
	VulkanDynamicFuncs
	VulkanComputeShader
	gltfReader
	)

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
