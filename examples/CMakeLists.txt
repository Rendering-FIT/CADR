# folder name in IDE (Visual C++,...) where all examples will go
set(examples_folder_name "Examples")

set(PACKAGES
	tut01-deviceList
	tut02-vulkanInfo
	tut04-swapChain
	tut05-resizableWindow
	VulkanHelloTriangle
	VulkanDepthBuffer
	)
if(UNIX)
	list(INSERT PACKAGES 2 tut03-helloWindowX11)
elseif(WIN32)
	list(INSERT PACKAGES 2 tut03-helloWindowWin)
endif()

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
