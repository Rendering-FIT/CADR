# SPDX-FileCopyrightText: 2018-2025 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
#
# SPDX-License-Identifier: MIT-0

# folder name in IDE (Visual C++,...) where all examples will go
set(examples_folder_name "Examples")

set(PACKAGES
	OpenGLInteroperability
	RenderingPerformance
	VulkanInfo
	gltfReader
	)

foreach(pkg ${PACKAGES})
	add_subdirectory(${pkg})
endforeach()
