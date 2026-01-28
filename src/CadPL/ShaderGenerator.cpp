#include <CadPL/ShaderGenerator.h>
#include <CadPL/ShaderLibrary.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadPL;

// shader code in SPIR-V binary
static const uint32_t vertexUberShaderSpirv[]={
#include "shaders/UberShader.vert.spv"
};
static const uint32_t vertexIdBufferUberShaderSpirv[]={
#include "shaders/UberShader-idBuffer.vert.spv"
};
static const uint32_t geometryUberShaderTrianglesSpirv[]={
#include "shaders/UberShaderTriangles.geom.spv"
};
static const uint32_t geometryUberShaderLinesSpirv[]={
#include "shaders/UberShaderLines.geom.spv"
};
static const uint32_t geometryIdBufferUberShaderTrianglesSpirv[]={
#include "shaders/UberShaderTriangles-idBuffer.geom.spv"
};
static const uint32_t geometryIdBufferUberShaderLinesSpirv[]={
#include "shaders/UberShaderLines-idBuffer.geom.spv"
};
static const uint32_t fragmentUberShaderTrianglesSpirv[]={
#include "shaders/UberShaderTriangles.frag.spv"
};
static const uint32_t fragmentUberShaderLinesSpirv[]={
#include "shaders/UberShaderLines.frag.spv"
};
static const uint32_t fragmentIdBufferUberShaderTrianglesSpirv[]={
#include "shaders/UberShaderTriangles-idBuffer.frag.spv"
};
static const uint32_t fragmentIdBufferUberShaderLinesSpirv[]={
#include "shaders/UberShaderLines-idBuffer.frag.spv"
};
static const uint32_t vertexUberShaderPointsSpirv[]={
#include "shaders/UberShaderPoints.vert.spv"
};
static const uint32_t vertexIdBufferUberShaderPointsSpirv[]={
#include "shaders/UberShaderPoints-idBuffer.vert.spv"
};
static const uint32_t fragmentUberShaderPointsSpirv[]={
#include "shaders/UberShaderPoints.frag.spv"
};
static const uint32_t fragmentIdBufferUberShaderPointsSpirv[]={
#include "shaders/UberShaderPoints-idBuffer.frag.spv"
};



vk::ShaderModule ShaderGenerator::createVertexShader(const ShaderState& state, CadR::VulkanDevice& device)
{
	const uint32_t* code;
	size_t size;
	if(state.primitiveTopology == vk::PrimitiveTopology::ePointList) {
		if(state.idBuffer) {
			code = vertexIdBufferUberShaderPointsSpirv;
			size = sizeof(vertexIdBufferUberShaderPointsSpirv);
		}
		else {
			code = vertexUberShaderPointsSpirv;
			size = sizeof(vertexUberShaderPointsSpirv);
		}
	}
	else {
		if(state.idBuffer) {
			code = vertexIdBufferUberShaderSpirv;
			size = sizeof(vertexIdBufferUberShaderSpirv);
		}
		else {
			code = vertexUberShaderSpirv;
			size = sizeof(vertexUberShaderSpirv);
		}
	}

	return
		device.createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				size,  // codeSize
				code   // pCode
			)
		);
}


vk::ShaderModule ShaderGenerator::createGeometryShader(const ShaderState& state, CadR::VulkanDevice& device)
{
	const uint32_t* code;
	size_t size;
	switch(state.primitiveTopology) {
	case vk::PrimitiveTopology::eTriangleList:
	case vk::PrimitiveTopology::eTriangleStrip:
	case vk::PrimitiveTopology::eTriangleFan:
		if(state.idBuffer) {
			code = geometryIdBufferUberShaderTrianglesSpirv;
			size = sizeof(geometryIdBufferUberShaderTrianglesSpirv);
		}
		else {
			code = geometryUberShaderTrianglesSpirv;
			size = sizeof(geometryUberShaderTrianglesSpirv);
		}
		break;
	case vk::PrimitiveTopology::eLineList:
	case vk::PrimitiveTopology::eLineStrip:
		if(state.idBuffer) {
			code = geometryIdBufferUberShaderLinesSpirv;
			size = sizeof(geometryIdBufferUberShaderLinesSpirv);
		}
		else {
			code = geometryUberShaderLinesSpirv;
			size = sizeof(geometryUberShaderLinesSpirv);
		}
		break;
	default:
		return vk::ShaderModule(nullptr);
	}

	return
		device.createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				size,  // codeSize
				code   // pCode
			)
		);
}


vk::ShaderModule ShaderGenerator::createFragmentShader(const ShaderState& state, CadR::VulkanDevice& device)
{
	const uint32_t* code;
	size_t size;
	switch(state.primitiveTopology) {
	case vk::PrimitiveTopology::eTriangleList:
	case vk::PrimitiveTopology::eTriangleStrip:
	case vk::PrimitiveTopology::eTriangleFan:
		if(state.idBuffer) {
			code = fragmentIdBufferUberShaderTrianglesSpirv;
			size = sizeof(fragmentIdBufferUberShaderTrianglesSpirv);
		}
		else {
			code = fragmentUberShaderTrianglesSpirv;
			size = sizeof(fragmentUberShaderTrianglesSpirv);
		}
		break;
	case vk::PrimitiveTopology::eLineList:
	case vk::PrimitiveTopology::eLineStrip:
		if(state.idBuffer) {
			code = fragmentIdBufferUberShaderLinesSpirv;
			size = sizeof(fragmentIdBufferUberShaderLinesSpirv);
		}
		else {
			code = fragmentUberShaderLinesSpirv;
			size = sizeof(fragmentUberShaderLinesSpirv);
		}
		break;
	case vk::PrimitiveTopology::ePointList:
		if(state.idBuffer) {
			code = fragmentIdBufferUberShaderPointsSpirv;
			size = sizeof(fragmentIdBufferUberShaderPointsSpirv);
		}
		else {
			code = fragmentUberShaderPointsSpirv;
			size = sizeof(fragmentUberShaderPointsSpirv);
		}
		break;
	default:
		code = nullptr;
		size = 0;
	}

	return
		device.createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				size,  // codeSize
				code   // pCode
			)
		);
}
