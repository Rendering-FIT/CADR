# folder name in IDE (Visual C++,...) where all examples will go
set(examples_folder_name "Examples")

set(PACKAGES
	OpenGLInteroperability
	HelloQtWindow
	RenderingPerformance
	VulkanAttributesAndUniforms
	VulkanComputeShader
	VulkanInfo
	gltfReader
	)

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
