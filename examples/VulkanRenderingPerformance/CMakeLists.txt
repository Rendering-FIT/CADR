set(APP_NAME VulkanRenderingPerformance)

project(${APP_NAME})

set(APP_SOURCES
	main.cpp
	)

set(APP_INCLUDES
	)

set(APP_SHADERS
	attributelessConstantOutput.vert
	attributelessInputIndices.vert
	coordinateAttribute.vert
	coordinate4Buffer.vert
	coordinate3Buffer.vert
	singleUniformMatrix.vert
	matrixAttribute.vert
	matrixBuffer.vert
	twoAttributes.vert
	twoBuffers.vert
	twoBuffer3.vert
	twoInterleavedBuffers.vert
	twoPackedAttributes.vert
	twoPackedBuffers.vert
	twoPackedBuffersUsingStruct.vert
	twoPackedBuffersUsingStructSlow.vert
	singlePackedBuffer.vert
	twoPackedAttributesAndSingleMatrix.vert
	twoPackedAttributesAndMatrix.vert
	twoPackedBuffersAndMatrix.vert
	fourAttributes.vert
	fourBuffers.vert
	fourBuffer3.vert
	fourInterleavedBuffers.vert
	fourAttributesAndMatrix.vert
	geometryShaderConstantOutput.vert
	geometryShaderConstantOutput.geom
	geometryShader.vert
	geometryShader.geom
	transformationThreeMatrices.vert
	transformationFiveMatrices.vert
	transformationFiveMatrices-pushConstants.vert
	transformationFiveMatrices-specializationConstants.vert
	transformationFiveMatrices-constants.vert
	transformationFiveMatricesUsingGS.geom
	transformationFiveMatricesUsingGSAndAttributes.vert
	transformationFiveMatricesUsingGSAndAttributes.geom
	phongTexturedFourAttributesFiveMatrices.vert
	phongTexturedFourAttributes.vert
	phongTextured.vert
	phongTexturedRowMajor.vert
	phongTexturedMat4x3.vert
	phongTexturedMat4x3RowMajor.vert
	phongTexturedQuat1.vert
	phongTexturedQuat2.vert
	phongTexturedQuat3.vert
	phongTexturedQuat2PrimitiveRestart.vert
	phongTexturedSingleQuat2.vert
	phongTexturedDMatricesOnlyInput.vert
	phongTexturedDMatrices.vert
	phongTexturedDMatricesDVertices.vert
	phongTexturedInGSDMatricesDVertices.vert
	phongTexturedInGSDMatricesDVertices.geom
	constantColor.frag
	phongTexturedDummy.frag
	phongTextured.frag
	phongTexturedNotPacked.frag
	fullscreenQuad.vert
	fullscreenQuadFourInterpolators.vert
	fullscreenQuadFourSmoothInterpolators.frag
	fullscreenQuadFourFlatInterpolators.frag
	fullscreenQuadTexturedPhongInterpolators.vert
	fullscreenQuadTexturedPhongInterpolators.frag
	uniformColor4f.frag
	uniformColor4b.frag
	fullscreenQuadTwoVec3Interpolators.vert
	phongNoSpecular.frag
	phongNoSpecularSingleUniform.frag
	)

add_shaders("${APP_SHADERS}" APP_SHADER_DEPS)
add_executable(${APP_NAME} ${APP_SOURCES} ${APP_INCLUDES} ${APP_SHADER_DEPS})

# dependencies
find_package(Vulkan REQUIRED)
if(UNIX)
	find_package(X11 REQUIRED)
	set(deps Vulkan::Vulkan X11)
elseif(WIN32)
	set(deps Vulkan::Vulkan)
endif()

target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${APP_NAME} ${deps})
set_property(TARGET ${APP_NAME} PROPERTY FOLDER "${examples_folder_name}")
