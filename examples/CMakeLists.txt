# folder name in IDE (Visual C++,...) where all examples will go
set(examples_folder_name "Examples")

set(PACKAGES
	OpenGLInteroperability
	RenderingPerformance
	VulkanComputeShader
	VulkanInfo
	gltfReader
	)

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
