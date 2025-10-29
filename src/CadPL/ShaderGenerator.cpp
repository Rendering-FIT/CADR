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
static const uint32_t geometryUberShaderSpirv[]={
#include "shaders/UberShader.geom.spv"
};
static const uint32_t geometryIdBufferUberShaderSpirv[]={
#include "shaders/UberShader-idBuffer.geom.spv"
};
static const uint32_t fragmentUberShaderSpirv[]={
#include "shaders/UberShader.frag.spv"
};
static const uint32_t fragmentIdBufferUberShaderSpirv[]={
#include "shaders/UberShader-idBuffer.frag.spv"
};



vk::ShaderModule ShaderGenerator::createVertexShader(const ShaderState& state, CadR::VulkanDevice& device)
{
	const uint32_t* code;
	size_t size;
	if(state.idBuffer) {
		code = vertexIdBufferUberShaderSpirv;
		size = sizeof(vertexIdBufferUberShaderSpirv);
	}
	else {
		code = vertexUberShaderSpirv;
		size = sizeof(vertexUberShaderSpirv);
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
	if(state.idBuffer) {
		code = geometryIdBufferUberShaderSpirv;
		size = sizeof(geometryIdBufferUberShaderSpirv);
	}
	else {
		code = geometryUberShaderSpirv;
		size = sizeof(geometryUberShaderSpirv);
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
	if(state.idBuffer) {
		code = fragmentIdBufferUberShaderSpirv;
		size = sizeof(fragmentIdBufferUberShaderSpirv);
	}
	else {
		code = fragmentUberShaderSpirv;
		size = sizeof(fragmentUberShaderSpirv);
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
