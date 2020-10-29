#ifdef _WIN32
# define VK_USE_PLATFORM_WIN32_KHR
#else
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# define VK_USE_PLATFORM_XLIB_KHR
#endif
#include <vulkan/vulkan.hpp>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

using namespace std;


// Vulkan instance
// (must be destructed as the last one, at least on Linux, it must be destroyed after display connection)
static vk::UniqueInstance instance;

// windowing variables
#ifdef _WIN32
static HWND window=nullptr;
struct Win32Cleaner {
	~Win32Cleaner() {
		if(window) {
			DestroyWindow(window);
			UnregisterClass("RenderingWindow",GetModuleHandle(NULL));
		}
	}
} win32Cleaner;
#else
static Display* display=nullptr;
static Window window=0;
struct XlibCleaner {
	~XlibCleaner() {
		if(window)  XDestroyWindow(display,window);
		if(display)  XCloseDisplay(display);
	}
} xlibCleaner;
Atom wmDeleteMessage;
#endif
static vk::Extent2D currentSurfaceExtent(0,0);
static vk::Extent2D windowSize;
static bool needResize=true;

// Vulkan handles and objects
// (they need to be placed in particular (not arbitrary) order as it gives their destruction order)
static vk::UniqueSurfaceKHR surface;
static vk::PhysicalDevice physicalDevice;
static uint32_t graphicsQueueFamily;
static uint32_t presentationQueueFamily;
static uint32_t sparseQueueFamily;
static vk::PhysicalDeviceFeatures enabledFeatures;
static vk::UniqueDevice device;
static vk::Queue graphicsQueue;
static vk::Queue presentationQueue;
static vk::Queue sparseQueue;
static vk::SurfaceFormatKHR chosenSurfaceFormat;
static vk::Format depthFormat;
static vk::UniqueRenderPass renderPass;
static vk::UniqueShaderModule attributelessConstantOutputVS;
static vk::UniqueShaderModule attributelessInputIndicesVS;
static vk::UniqueShaderModule coordinateAttributeVS;
static vk::UniqueShaderModule coordinateBufferVS;
static vk::UniqueShaderModule singleUniformMatrixVS;
static vk::UniqueShaderModule matrixAttributeVS;
static vk::UniqueShaderModule matrixBufferVS;
static vk::UniqueShaderModule twoAttributesVS;
static vk::UniqueShaderModule twoPackedAttributesVS;
static vk::UniqueShaderModule twoPackedBuffersVS;
static vk::UniqueShaderModule twoPackedBuffersUsingStructVS;
static vk::UniqueShaderModule twoPackedBuffersUsingStructSlowVS;
static vk::UniqueShaderModule singlePackedBufferVS;
static vk::UniqueShaderModule twoPackedAttributesAndSingleMatrixVS;
static vk::UniqueShaderModule twoPackedAttributesAndMatrixVS;
static vk::UniqueShaderModule twoPackedBuffersAndMatrixVS;
static vk::UniqueShaderModule fourAttributesVS;
static vk::UniqueShaderModule fourAttributesAndMatrixVS;
static vk::UniqueShaderModule geometryShaderConstantOutputVS;
static vk::UniqueShaderModule geometryShaderConstantOutputGS;
static vk::UniqueShaderModule geometryShaderVS;
static vk::UniqueShaderModule geometryShaderGS;
static vk::UniqueShaderModule transformationThreeMatricesVS;
static vk::UniqueShaderModule transformationFiveMatricesVS;
static vk::UniqueShaderModule transformationFiveMatricesUsingGS;
static vk::UniqueShaderModule transformationFiveMatricesUsingGSAndAttributesVS;
static vk::UniqueShaderModule transformationFiveMatricesUsingGSAndAttributesGS;
static vk::UniqueShaderModule phongTexturedFourAttributesFiveMatricesVS;
static vk::UniqueShaderModule phongTexturedFourAttributesVS;
static vk::UniqueShaderModule phongTexturedVS;
static vk::UniqueShaderModule phongTexturedRowMajorVS;
static vk::UniqueShaderModule phongTexturedMat4x3VS;
static vk::UniqueShaderModule phongTexturedMat4x3RowMajorVS;
static vk::UniqueShaderModule phongTexturedQuat1VS;
static vk::UniqueShaderModule phongTexturedQuat2VS;
static vk::UniqueShaderModule phongTexturedQuat3VS;
static vk::UniqueShaderModule phongTexturedQuat2PrimitiveRestartVS;
static vk::UniqueShaderModule phongTexturedSingleQuat2VS;
static vk::UniqueShaderModule phongTexturedDMatricesOnlyInputVS;
static vk::UniqueShaderModule phongTexturedDMatricesVS;
static vk::UniqueShaderModule phongTexturedDMatricesDVerticesVS;
static vk::UniqueShaderModule phongTexturedInGSDMatricesDVerticesVS;
static vk::UniqueShaderModule phongTexturedInGSDMatricesDVerticesGS;
static vk::UniqueShaderModule constantColorFS;
static vk::UniqueShaderModule phongTexturedDummyFS;
static vk::UniqueShaderModule phongTexturedFS;
static vk::UniqueShaderModule phongTexturedNotPackedFS;
static vk::UniqueShaderModule fullscreenQuadVS;
static vk::UniqueShaderModule fullscreenQuadFourInterpolatorsVS;
static vk::UniqueShaderModule fullscreenQuadFourSmoothInterpolatorsFS;
static vk::UniqueShaderModule fullscreenQuadFourFlatInterpolatorsFS;
static vk::UniqueShaderModule fullscreenQuadTexturedPhongInterpolatorsVS;
static vk::UniqueShaderModule fullscreenQuadTexturedPhongInterpolatorsFS;
static vk::UniqueShaderModule uniformColor4fFS;
static vk::UniqueShaderModule uniformColor4bFS;
static vk::UniqueShaderModule fullscreenQuadTwoVec3InterpolatorsVS;
static vk::UniqueShaderModule phongNoSpecularFS;
static vk::UniqueShaderModule phongNoSpecularSingleUniformFS;
static vk::UniquePipelineCache pipelineCache;
static vk::UniquePipelineLayout simplePipelineLayout;
static vk::UniquePipelineLayout oneUniformVSPipelineLayout;
static vk::UniquePipelineLayout oneUniformFSPipelineLayout;
static vk::UniquePipelineLayout oneBufferPipelineLayout;
static vk::UniquePipelineLayout twoBuffersPipelineLayout;
static vk::UniquePipelineLayout threeBuffersPipelineLayout;
static vk::UniquePipelineLayout threeBuffersInGSPipelineLayout;
static vk::UniquePipelineLayout threeUniformFSPipelineLayout;
static vk::UniquePipelineLayout bufferAndUniformPipelineLayout;
static vk::UniquePipelineLayout bufferAndUniformInGSPipelineLayout;
static vk::UniquePipelineLayout twoBuffersAndUniformPipelineLayout;
static vk::UniquePipelineLayout twoBuffersAndUniformInGSPipelineLayout;
static vk::UniquePipelineLayout fourBuffersAndUniformInGSPipelineLayout;
static vk::UniquePipelineLayout phongTexturedPipelineLayout;
static vk::UniqueDescriptorSetLayout oneUniformVSDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout oneUniformFSDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout oneBufferDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout twoBuffersDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout threeBuffersDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout threeBuffersInGSDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout threeUniformFSDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout bufferAndUniformDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout bufferAndUniformInGSDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout twoBuffersAndUniformDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout twoBuffersAndUniformInGSDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout fourBuffersAndUniformInGSDescriptorSetLayout;
static vk::UniqueDescriptorSetLayout phongTexturedDescriptorSetLayout;
static vk::UniqueBuffer singleMatrixUniformBuffer;
static vk::UniqueBuffer singlePATBuffer;
static vk::UniqueBuffer sameMatrixAttribute;  // not used now
static vk::UniqueBuffer sameMatrixBuffer;
static vk::UniqueBuffer sameMatrixRowMajorBuffer;
static vk::UniqueBuffer sameMatrix4x3Buffer;
static vk::UniqueBuffer sameMatrix4x3RowMajorBuffer;
static vk::UniqueBuffer sameDMatrixBuffer;
static vk::UniqueBuffer samePATBuffer;
static vk::UniqueBuffer transformationMatrixAttribute;
static vk::UniqueBuffer transformationMatrixBuffer;
static vk::UniqueBuffer indirectBuffer;
static vk::UniqueBuffer normalMatrix4x3Buffer;
static vk::UniqueBuffer viewAndProjectionMatricesUniformBuffer;
static vk::UniqueBuffer viewAndProjectionDMatricesUniformBuffer;
static vk::UniqueBuffer materialUniformBuffer;
static vk::UniqueBuffer materialNotPackedUniformBuffer;
static vk::UniqueBuffer globalLightUniformBuffer;
static vk::UniqueBuffer lightUniformBuffer;
static vk::UniqueBuffer lightNotPackedUniformBuffer;
static vk::UniqueBuffer allInOneLightingUniformBuffer;
static vk::UniqueDeviceMemory singleMatrixUniformMemory;
static vk::UniqueDeviceMemory singlePATMemory;
static vk::UniqueDeviceMemory sameMatrixAttributeMemory;  // not used now
static vk::UniqueDeviceMemory sameMatrixBufferMemory;
static vk::UniqueDeviceMemory sameMatrixRowMajorBufferMemory;
static vk::UniqueDeviceMemory sameMatrix4x3BufferMemory;
static vk::UniqueDeviceMemory sameMatrix4x3RowMajorBufferMemory;
static vk::UniqueDeviceMemory sameDMatrixBufferMemory;
static vk::UniqueDeviceMemory samePATBufferMemory;
static vk::UniqueDeviceMemory transformationMatrixAttributeMemory;
static vk::UniqueDeviceMemory transformationMatrixBufferMemory;  // not used now
static vk::UniqueDeviceMemory indirectBufferMemory;
static vk::UniqueDeviceMemory normalMatrix4x3Memory;
static vk::UniqueDeviceMemory viewAndProjectionMatricesMemory;
static vk::UniqueDeviceMemory viewAndProjectionDMatricesMemory;
static vk::UniqueDeviceMemory materialUniformBufferMemory;
static vk::UniqueDeviceMemory materialNotPackedUniformBufferMemory;
static vk::UniqueDeviceMemory globalLightUniformBufferMemory;
static vk::UniqueDeviceMemory lightUniformBufferMemory;
static vk::UniqueDeviceMemory lightNotPackedUniformBufferMemory;
static vk::UniqueDeviceMemory allInOneLightingUniformBufferMemory;
static vk::UniqueDescriptorPool descriptorPool;
static vk::DescriptorSet oneUniformVSDescriptorSet;
static vk::DescriptorSet one4fUniformFSDescriptorSet;
static vk::DescriptorSet one4bUniformFSDescriptorSet;
static vk::DescriptorSet coordinateBufferDescriptorSet;
static vk::DescriptorSet sameMatrixBufferDescriptorSet;
static vk::DescriptorSet transformationMatrixBufferDescriptorSet;
static vk::DescriptorSet singlePackedBufferDescriptorSet;
static vk::DescriptorSet twoBuffersDescriptorSet;
static vk::DescriptorSet threeBuffersDescriptorSet;
static vk::DescriptorSet threeBuffersInGSDescriptorSet;
static vk::DescriptorSet threeUniformFSDescriptorSet;
static vk::DescriptorSet transformationThreeMatricesDescriptorSet;
static vk::DescriptorSet transformationThreeMatricesRowMajorDescriptorSet;
static vk::DescriptorSet transformationThreeMatrices4x3DescriptorSet;
static vk::DescriptorSet transformationThreeMatrices4x3RowMajorDescriptorSet;
static vk::DescriptorSet transformationThreeDMatricesDescriptorSet;
static vk::DescriptorSet transformationTwoMatricesAndPATDescriptorSet;
static vk::DescriptorSet transformationTwoMatricesAndSinglePATDescriptorSet;
static vk::DescriptorSet transformationFiveMatricesDescriptorSet;
static vk::DescriptorSet transformationFiveMatricesUsingGSDescriptorSet;
static vk::DescriptorSet transformationFiveMatricesUsingGSAndAttributesDescriptorSet;
static vk::DescriptorSet phongTexturedThreeDMatricesUsingGSAndAttributesDescriptorSet;
static vk::DescriptorSet phongTexturedDescriptorSet;
static vk::DescriptorSet phongTexturedNotPackedDescriptorSet;
static vk::DescriptorSet allInOneLightingUniformDescriptorSet;
static vk::UniqueSwapchainKHR swapchain;
static vector<vk::UniqueImageView> swapchainImageViews;
static vk::UniqueImage depthImage;
static vk::UniqueDeviceMemory depthImageMemory;
static vk::UniqueImageView depthImageView;
static vk::UniquePipeline attributelessConstantOutputPipeline;
static vk::UniquePipeline attributelessInputIndicesPipeline;
static vk::UniquePipeline coordinateAttributePipeline;
static vk::UniquePipeline coordinateBufferPipeline;
static vk::UniquePipeline singleMatrixUniformPipeline;
static vk::UniquePipeline matrixAttributePipeline;
static vk::UniquePipeline matrixBufferPipeline;
static vk::UniquePipeline twoAttributesPipeline;
static vk::UniquePipeline twoPackedAttributesPipeline;
static vk::UniquePipeline twoPackedBuffersPipeline;
static vk::UniquePipeline twoPackedBuffersUsingStructPipeline;
static vk::UniquePipeline twoPackedBuffersUsingStructSlowPipeline;
static vk::UniquePipeline two4F32Two4U8AttributesPipeline;
static vk::UniquePipeline singlePackedBufferPipeline;
static vk::UniquePipeline twoPackedAttributesAndSingleMatrixPipeline;
static vk::UniquePipeline twoPackedAttributesAndMatrixPipeline;
static vk::UniquePipeline twoPackedBuffersAndMatrixPipeline;
static vk::UniquePipeline fourAttributesPipeline;
static vk::UniquePipeline fourAttributesAndMatrixPipeline;
static vk::UniquePipeline geometryShaderConstantOutputPipeline;
static vk::UniquePipeline geometryShaderPipeline;
static vk::UniquePipeline transformationThreeMatricesPipeline;
static vk::UniquePipeline transformationFiveMatricesPipeline;
static vk::UniquePipeline transformationFiveMatricesUsingGSPipeline;
static vk::UniquePipeline transformationFiveMatricesUsingGSAndAttributesPipeline;
static vk::UniquePipeline phongTexturedFourAttributesFiveMatricesPipeline;
static vk::UniquePipeline phongTexturedFourAttributesPipeline;
static vk::UniquePipeline phongTexturedPipeline;
static vk::UniquePipeline phongTexturedRowMajorPipeline;
static vk::UniquePipeline phongTexturedMat4x3Pipeline;
static vk::UniquePipeline phongTexturedMat4x3RowMajorPipeline;
static vk::UniquePipeline phongTexturedQuat1Pipeline;
static vk::UniquePipeline phongTexturedQuat2Pipeline;
static vk::UniquePipeline phongTexturedQuat3Pipeline;
static vk::UniquePipeline phongTexturedQuat2PrimitiveRestartPipeline;
static vk::UniquePipeline phongTexturedDMatricesOnlyInputPipeline;
static vk::UniquePipeline phongTexturedDMatricesPipeline;
static vk::UniquePipeline phongTexturedDMatricesDVerticesPipeline;
static vk::UniquePipeline phongTexturedInGSDMatricesDVerticesPipeline;
static vk::UniquePipeline phongTexturedSingleQuat2Pipeline;
static vk::UniquePipeline phongTexturedSingleQuat2TriStripPipeline;
static vk::UniquePipeline phongTexturedSingleQuat2PrimitiveRestartPipeline;
static vk::UniquePipeline fillrateContantColorPipeline;
static vk::UniquePipeline fillrateFourSmoothInterpolatorsPipeline;
static vk::UniquePipeline fillrateFourFlatInterpolatorsPipeline;
static vk::UniquePipeline fillrateTexturedPhongInterpolatorsPipeline;
static vk::UniquePipeline fillrateTexturedPhongPipeline;
static vk::UniquePipeline fillrateTexturedPhongNotPackedPipeline;
static vk::UniquePipeline fillrateUniformColor4fPipeline;
static vk::UniquePipeline fillrateUniformColor4bPipeline;
static vk::UniquePipeline phongNoSpecularPipeline;
static vk::UniquePipeline phongNoSpecularSingleUniformPipeline;
static vector<vk::UniqueFramebuffer> framebuffers;
static vk::UniqueCommandPool commandPool;
static vector<vk::UniqueCommandBuffer> commandBuffers;
static vk::UniqueSemaphore imageAvailableSemaphore;
static vk::UniqueSemaphore renderFinishedSemaphore;
static vk::UniqueBuffer coordinateAttribute;
static vk::UniqueBuffer coordinateBuffer;
static vk::UniqueBuffer normalAttribute;
static vk::UniqueBuffer colorAttribute;
static vk::UniqueBuffer texCoordAttribute;
static array<vk::UniqueBuffer,3> vec4Attributes;
static array<vk::UniqueBuffer,2> vec4u8Attributes;
static vk::UniqueBuffer packedAttribute1;
static vk::UniqueBuffer packedAttribute2;
static vk::UniqueBuffer packedBuffer1;
static vk::UniqueBuffer packedBuffer2;
static vk::UniqueBuffer singlePackedBuffer;
static vk::UniqueBuffer packedDAttribute1;
static vk::UniqueBuffer packedDAttribute2;
static vk::UniqueBuffer packedDAttribute3;
static vk::UniqueBuffer indexBuffer;
static vk::UniqueBuffer primitiveRestartIndexBuffer;
static vk::UniqueBuffer stripIndexBuffer;
static vk::UniqueBuffer stripPrimitiveRestartIndexBuffer;
static vk::UniqueBuffer stripPrimitiveRestart3IndexBuffer;
static vk::UniqueBuffer stripPrimitiveRestart4IndexBuffer;
static vk::UniqueBuffer stripPrimitiveRestart7IndexBuffer;
static vk::UniqueBuffer stripPrimitiveRestart10IndexBuffer;
static vk::UniqueBuffer primitiveRestartMinusOne2IndexBuffer;
static vk::UniqueBuffer primitiveRestartMinusOne5IndexBuffer;
static vk::UniqueBuffer minusOneIndexBuffer;
static vk::UniqueBuffer zeroIndexBuffer;
static vk::UniqueBuffer plusOneIndexBuffer;
static vk::UniqueBuffer stripPackedAttribute1;
static vk::UniqueBuffer stripPackedAttribute2;
static vk::UniqueBuffer sharedVertexPackedAttribute1;
static vk::UniqueBuffer sharedVertexPackedAttribute2;
static vk::UniqueBuffer sameVertexPackedAttribute1;
static vk::UniqueBuffer sameVertexPackedAttribute2;
static vk::UniqueDeviceMemory coordinateAttributeMemory;
static vk::UniqueDeviceMemory coordinateBufferMemory;
static vk::UniqueDeviceMemory normalAttributeMemory;
static vk::UniqueDeviceMemory colorAttributeMemory;
static vk::UniqueDeviceMemory texCoordAttributeMemory;
static array<vk::UniqueDeviceMemory,3> vec4AttributeMemory;
static array<vk::UniqueDeviceMemory,2> vec4u8AttributeMemory;
static vk::UniqueDeviceMemory packedAttribute1Memory;
static vk::UniqueDeviceMemory packedAttribute2Memory;
static vk::UniqueDeviceMemory packedBuffer1Memory;
static vk::UniqueDeviceMemory packedBuffer2Memory;
static vk::UniqueDeviceMemory singlePackedBufferMemory;
static vk::UniqueDeviceMemory packedDAttribute1Memory;
static vk::UniqueDeviceMemory packedDAttribute2Memory;
static vk::UniqueDeviceMemory packedDAttribute3Memory;
static vk::UniqueDeviceMemory indexBufferMemory;
static vk::UniqueDeviceMemory primitiveRestartIndexBufferMemory;
static vk::UniqueDeviceMemory stripIndexBufferMemory;
static vk::UniqueDeviceMemory stripPrimitiveRestartIndexBufferMemory;
static vk::UniqueDeviceMemory stripPrimitiveRestart3IndexBufferMemory;
static vk::UniqueDeviceMemory stripPrimitiveRestart4IndexBufferMemory;
static vk::UniqueDeviceMemory stripPrimitiveRestart7IndexBufferMemory;
static vk::UniqueDeviceMemory stripPrimitiveRestart10IndexBufferMemory;
static vk::UniqueDeviceMemory primitiveRestartMinusOne2IndexBufferMemory;
static vk::UniqueDeviceMemory primitiveRestartMinusOne5IndexBufferMemory;
static vk::UniqueDeviceMemory minusOneIndexBufferMemory;
static vk::UniqueDeviceMemory zeroIndexBufferMemory;
static vk::UniqueDeviceMemory plusOneIndexBufferMemory;
static vk::UniqueDeviceMemory stripPackedAttribute1Memory;
static vk::UniqueDeviceMemory stripPackedAttribute2Memory;
static vk::UniqueDeviceMemory sharedVertexPackedAttribute1Memory;
static vk::UniqueDeviceMemory sharedVertexPackedAttribute2Memory;
static vk::UniqueDeviceMemory sameVertexPackedAttribute1Memory;
static vk::UniqueDeviceMemory sameVertexPackedAttribute2Memory;
static vk::UniqueImage singleTexelImage;
static vk::UniqueDeviceMemory singleTexelImageMemory;
static vk::UniqueImageView singleTexelImageView;
static vk::UniqueSampler trilinearSampler;
static vk::UniqueQueryPool timestampPool;
static uint32_t timestampValidBits=0;
static float timestampPeriod_ns=0;
static const uint32_t numTrianglesStandard=uint32_t(1*1e6);
static const uint32_t numTrianglesReduced=uint32_t(1*1e5);
static uint32_t numTriangles;
static vk::Extent2D renderingExtent(1366,768);  // HD resultion (it should always fit to FullHD screen, even with window captions, taskbar, etc.)
static const unsigned triangleSize=0;
static uint32_t numFullscreenQuads=10; // note: if you increase the value, make sure that fullscreenQuad*.vert is still drawing to the clip space (by gl_InstanceIndex)
static const unsigned triStripLength=1000;

// sparse memory variables
enum { SPARSE_NONE, SPARSE_BINDING, SPARSE_RESIDENCY, SPARSE_RESIDENCY_ALIASED };
static int sparseMode = SPARSE_NONE;
static size_t memoryBlockSize=~0;
static size_t memoryBlockMask=~0;
static size_t sparseBlockSize=~0;
static vk::BufferCreateFlags bufferCreateFlags = {};
static unsigned bufferSizeMultiplier = 1;

// shader code in SPIR-V binary
static const uint32_t attributelessConstantOutputVS_spirv[]={
#include "attributelessConstantOutput.vert.spv"
};
static const uint32_t attributelessInputIndicesVS_spirv[]={
#include "attributelessInputIndices.vert.spv"
};
static const uint32_t coordinateAttributeVS_spirv[]={
#include "coordinateAttribute.vert.spv"
};
static const uint32_t coordinateBufferVS_spirv[]={
#include "coordinateBuffer.vert.spv"
};
static const uint32_t singleUniformMatrixVS_spirv[]={
#include "singleUniformMatrix.vert.spv"
};
static const uint32_t matrixAttributeVS_spirv[]={
#include "matrixAttribute.vert.spv"
};
static const uint32_t matrixBufferVS_spirv[]={
#include "matrixBuffer.vert.spv"
};
static const uint32_t twoAttributesVS_spirv[]={
#include "twoAttributes.vert.spv"
};
static const uint32_t twoPackedAttributesVS_spirv[]={
#include "twoPackedAttributes.vert.spv"
};
static const uint32_t twoPackedBuffersVS_spirv[]={
#include "twoPackedBuffers.vert.spv"
};
static const uint32_t twoPackedBuffersUsingStructVS_spirv[]={
#include "twoPackedBuffersUsingStruct.vert.spv"
};
static const uint32_t twoPackedBuffersUsingStructSlowVS_spirv[]={
#include "twoPackedBuffersUsingStructSlow.vert.spv"
};
static const uint32_t singlePackedBufferVS_spirv[]={
#include "singlePackedBuffer.vert.spv"
};
static const uint32_t twoPackedAttributesAndSingleMatrixVS_spirv[]={
#include "twoPackedAttributesAndSingleMatrix.vert.spv"
};
static const uint32_t twoPackedAttributesAndMatrixVS_spirv[]={
#include "twoPackedAttributesAndMatrix.vert.spv"
};
static const uint32_t twoPackedBuffersAndMatrixVS_spirv[]={
#include "twoPackedBuffersAndMatrix.vert.spv"
};
static const uint32_t fourAttributesVS_spirv[]={
#include "fourAttributes.vert.spv"
};
static const uint32_t fourAttributesAndMatrixVS_spirv[]={
#include "fourAttributesAndMatrix.vert.spv"
};
static const uint32_t geometryShaderConstantOutputVS_spirv[]={
#include "geometryShaderConstantOutput.vert.spv"
};
static const uint32_t geometryShaderConstantOutputGS_spirv[]={
#include "geometryShaderConstantOutput.geom.spv"
};
static const uint32_t geometryShaderVS_spirv[]={
#include "geometryShader.vert.spv"
};
static const uint32_t geometryShaderGS_spirv[]={
#include "geometryShader.geom.spv"
};
static const uint32_t transformationThreeMatricesVS_spirv[]={
#include "transformationThreeMatrices.vert.spv"
};
static const uint32_t transformationFiveMatricesVS_spirv[]={
#include "transformationFiveMatrices.vert.spv"
};
static const uint32_t transformationFiveMatricesUsingGS_spirv[]={
#include "transformationFiveMatricesUsingGS.geom.spv"
};
static const uint32_t transformationFiveMatricesUsingGSAndAttributesVS_spirv[]={
#include "transformationFiveMatricesUsingGSAndAttributes.vert.spv"
};
static const uint32_t transformationFiveMatricesUsingGSAndAttributesGS_spirv[]={
#include "transformationFiveMatricesUsingGSAndAttributes.geom.spv"
};
static const uint32_t phongTexturedFourAttributesFiveMatricesVS_spirv[]={
#include "phongTexturedFourAttributesFiveMatrices.vert.spv"
};
static const uint32_t phongTexturedFourAttributesVS_spirv[]={
#include "phongTexturedFourAttributes.vert.spv"
};
static const uint32_t phongTexturedVS_spirv[]={
#include "phongTextured.vert.spv"
};
static const uint32_t phongTexturedRowMajorVS_spirv[]={
#include "phongTexturedRowMajor.vert.spv"
};
static const uint32_t phongTexturedMat4x3VS_spirv[]={
#include "phongTexturedMat4x3.vert.spv"
};
static const uint32_t phongTexturedMat4x3RowMajorVS_spirv[]={
#include "phongTexturedMat4x3RowMajor.vert.spv"
};
static const uint32_t phongTexturedQuat1VS_spirv[]={
#include "phongTexturedQuat1.vert.spv"
};
static const uint32_t phongTexturedQuat2VS_spirv[]={
#include "phongTexturedQuat2.vert.spv"
};
static const uint32_t phongTexturedQuat3VS_spirv[]={
#include "phongTexturedQuat3.vert.spv"
};
static const uint32_t phongTexturedQuat2PrimitiveRestartVS_spirv[]={
#include "phongTexturedQuat2PrimitiveRestart.vert.spv"
};
static const uint32_t phongTexturedSingleQuat2VS_spirv[]={
#include "phongTexturedSingleQuat2.vert.spv"
};
static const uint32_t phongTexturedDMatricesOnlyInputVS_spirv[]={
#include "phongTexturedDMatricesOnlyInput.vert.spv"
};
static const uint32_t phongTexturedDMatricesVS_spirv[]={
#include "phongTexturedDMatrices.vert.spv"
};
static const uint32_t phongTexturedDMatricesDVerticesVS_spirv[]={
#include "phongTexturedDMatricesDVertices.vert.spv"
};
static const uint32_t phongTexturedInGSDMatricesDVerticesVS_spirv[]={
#include "phongTexturedInGSDMatricesDVertices.vert.spv"
};
static const uint32_t phongTexturedInGSDMatricesDVerticesGS_spirv[]={
#include "phongTexturedInGSDMatricesDVertices.geom.spv"
};
static const uint32_t constantColorFS_spirv[]={
#include "constantColor.frag.spv"
};
static const uint32_t phongTexturedDummyFS_spirv[]={
#include "phongTexturedDummy.frag.spv"
};
static const uint32_t phongTexturedFS_spirv[]={
#include "phongTextured.frag.spv"
};
static const uint32_t phongTexturedNotPackedFS_spirv[]={
#include "phongTexturedNotPacked.frag.spv"
};
static const uint32_t fullscreenQuadVS_spirv[]={
#include "fullscreenQuad.vert.spv"
};
static const uint32_t fullscreenQuadFourInterpolatorsVS_spirv[]={
#include "fullscreenQuadFourInterpolators.vert.spv"
};
static const uint32_t fullscreenQuadFourSmoothInterpolatorsFS_spirv[]={
#include "fullscreenQuadFourSmoothInterpolators.frag.spv"
};
static const uint32_t fullscreenQuadFourFlatInterpolatorsFS_spirv[]={
#include "fullscreenQuadFourFlatInterpolators.frag.spv"
};
static const uint32_t fullscreenQuadTexturedPhongInterpolatorsVS_spirv[]={
#include "fullscreenQuadTexturedPhongInterpolators.vert.spv"
};
static const uint32_t fullscreenQuadTexturedPhongInterpolatorsFS_spirv[]={
#include "fullscreenQuadTexturedPhongInterpolators.frag.spv"
};
static const uint32_t uniformColor4fFS_spirv[]={
#include "uniformColor4f.frag.spv"
};
static const uint32_t uniformColor4bFS_spirv[]={
#include "uniformColor4b.frag.spv"
};
static const uint32_t fullscreenQuadTwoVec3InterpolatorsVS_spirv[]={
#include "fullscreenQuadTwoVec3Interpolators.vert.spv"
};
static const uint32_t phongNoSpecularFS_spirv[]={
#include "phongNoSpecular.frag.spv"
};
static const uint32_t phongNoSpecularSingleUniformFS_spirv[]={
#include "phongNoSpecularSingleUniform.frag.spv"
};


struct Test {
	vector<uint64_t> renderingTimes;
	string resultString;
	bool enabled = true;
	enum class Type { VertexThroughput, FragmentThroughput };
	Type type;
	double numRenderedItems;
	Test(const char* resultString_,Type t=Type::VertexThroughput) : resultString(resultString_), type(t)  {}
};
static vector<Test> tests={
	Test("One draw call, attributeless, constant output"),
	Test("One draw call, attributeless, input indices"),
	Test("One draw call, attributeless, instancing"),
	Test("One draw call, geometry shader constant output"),
	Test("Indirect buffer instancing, attributeless"),
	Test("Indirect buffer per-triangle record, attr.less"),
	Test("Indirect buffer per-triangle record"),
	Test("Per-triangle drawCmd, attr.less, const. output"),
	Test("Per-triangle drawCmd, coordinates in attr."),
	Test("Coordinates in attribute"),
	Test("Coordinates in buffer"),
	Test("One attribute, constant uniform 1xMatrix"),
	Test("One attribute, per-triangle 1xMatrix in buffer"),
	Test("One attribute, per-triangle 1xMatrix in attr."),
	Test("Two attributes"),
	Test("Two packed attributes"),
	Test("Two packed buffers"),
	Test("Two packed buffers using struct"),
	Test("Two packed buffers using struct slow"),
	Test("All-in-one packed buffer"),
	Test("Two packed attributes, single 1xMatrix"),
	Test("Two packed attributes, 1xMatrix"),
	Test("Two packed buffers, 1xMatrix"),
	Test("Two packed buffers in geom. shader, 1xMatrix"),
	Test("Four (2xf32,2xu8) attributes"),
	Test("Four attributes"),
	Test("Four attributes, 1xMatrix"),
	Test("Transformation 3xMatrix in VS"),
	Test("Transformation 5xMatrix in VS"),
	Test("Transformation 5xMatrix in GS, attributes"),
	Test("Transformation 5xMatrix in GS, buffers"),
	Test("Phong, texture, four attributes, 5xMatrix"),
	Test("Phong, texture, four attributes, 3xMatrix"),
	Test("Phong, texture, 3xMatrix"),
	Test("Phong, texture, 3xMatrix, RowMajor"),
	Test("Phong, texture, 3xMatrix, Mat4x3"),
	Test("Phong, texture, 3xMatrix, Mat4x3 RowMajor"),
	Test("Phong, texture, 2xMatrix+Quat1"),
	Test("Phong, texture, 2xMatrix+Quat2"),
	Test("Phong, texture, 2xMatrix+Quat3"),
	Test("Phong, texture, 2xMatrix+Quat2, indexed"),
	Test("Phong, texture, 2xM+Q2, indexed prim.restart"),
	Test("Phong, texture, 3xMatrix, dmat. only on input"),
	Test("Phong, texture, 3xMatrix, dmatrices"),
	Test("Phong, texture, 3xMatrix, dmatrices, dvertices"),
	Test("Phong, texture, 3xMatrix, in GS, dmat., dvert."),
	Test("TexPhong, const 2xMat+Quat2, separate triangls"),
	Test("TP, c.2xM+Q2, separate triangles, indexed"),
	Test("TP, c.2xM+Q2, separate tri. primitive restart"),
	Test("TP, c.2xM+Q2, shared vertex triangles, noIndex"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after 1"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after 2"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after 5"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after 8"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after 10"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after 20"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after 25"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after 40"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after 50"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after100"),
	Test("TP,c2MQ2, sh.vtx.tri-noIndex, drawCmd after125"),
	Test("TP, c.2xM+Q2, shared vertex triangles, indexed"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 1"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 2"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 5"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 8"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 10"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 20"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 25"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 40"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 50"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 64"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 100"),
	Test("TP,c2MQ2, sh.vtx.tri-idx, drawCmd after 125"),
	Test("TP, c.2xM+Q2, sh.vtx.tri-strip, noIndex"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd after1"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd after2"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd after5"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd after8"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd aftr10"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd aftr20"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd aftr25"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd aftr40"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd aftr50"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd af.100"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-noI. drawCmd af.125"),
	Test("TP, c.2xM+Q2, sh.vtx.tri-strip, indexed"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd after1"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd after2"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd after5"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd after8"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd aftr10"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd aftr20"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd aftr25"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd aftr40"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd aftr50"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd af.100"),
	Test("TP,c2MQ2, sh.vtx.tri-strip-idx. drawCmd af.125"),
	Test("TP, c.2xM+Q2, tri-strip, primRes, 1call"),
	Test("TP, c.2xM+Q2, tri-strip, primRes, 1call/strip"),
	Test("TP,c2MQ2, sh.vtx.tri-strip, primRes. after 1"),
	Test("TP,c2MQ2, sh.vtx.tri-strip, primRes. after 2"),
	Test("TP,c2MQ2, sh.vtx.tri-strip, primRes. after 5"),
	Test("TP,c2MQ2, sh.vtx.tri-strip, primRes. after 8"),
	Test("TP,c2MQ2, sh.vtx.tri-strip, primRes. two -1"),
	Test("TP,c2MQ2, sh.vtx.tri-strip, primRes. five -1"),
	Test("TP,c2MQ2, idx all -1, primRes., numTri+2"),
	Test("TP,c2MQ2, idx all 0, primRest., numTri+2"),
	Test("TP,c2MQ2, idx all 1, primRest., numTri+2"),
	Test("TP,c2MQ2, idx all 0, tri-strip, numTri+2"),
	Test("TP,c2MQ2, idx all 1, tri-strip, numTri+2"),
	Test("TP,c2MQ2, idx all 0, tri-list, numTri*3"),
	Test("TP,c2MQ2, idx all 1, tri-list, numTri*3"),
	Test("TP, c.2xM+Q2, same vtx, triStrip, numTri+2"),
	Test("TP, c.2xM+Q2, same vtx, tri-list, numTri*3"),
	Test("Fullscreen quad 1x",Test::Type::FragmentThroughput),
	Test("Fullscreen quad 10x",Test::Type::FragmentThroughput),
	Test("Fullscreen quad 10x, four smooth interpolators",Test::Type::FragmentThroughput),
	Test("Fullscreen quad 10x, four flat interpolators",Test::Type::FragmentThroughput),
	Test("Fullscreen quad 10x, textured Phong interp.",Test::Type::FragmentThroughput),
	Test("Phong, texture",Test::Type::FragmentThroughput),
	Test("Phong, texture, not packed uniforms",Test::Type::FragmentThroughput),
	Test("Uniform color4f",Test::Type::FragmentThroughput),
	Test("Uniform color4b",Test::Type::FragmentThroughput),
	Test("Phong no specular",Test::Type::FragmentThroughput),
	Test("Phong no specular, single uniform",Test::Type::FragmentThroughput),
};


static size_t getBufferSize(uint32_t numTriangles,bool useVec4)
{
	return size_t(numTriangles)*3*(useVec4?4:3)*sizeof(float);
}


static void generateCoordinates(float* vertices,uint32_t numTriangles,unsigned triangleSize,
                                unsigned regionWidth,unsigned regionHeight,bool useVec4,
                                double scaleX=1.,double scaleY=1.,double offsetX=0.,double offsetY=0.)
{
	unsigned stride=triangleSize+2;
	unsigned numTrianglesPerLine=regionWidth/stride*2;
	unsigned numLinesPerScreen=regionHeight/stride;
	size_t idx=0;
	size_t idxEnd=size_t(numTriangles)*(useVec4?4:3)*3;

	// Each iteration generates two triangles.
	// triangleSize is dimension of square that is cut into the two triangles.
	// When triangleSize is set to 1:
	//    Both triangles together produce 4 pixels: the first triangle 3 pixels and
	//    the second triangle a single pixel. For more detail, see OpenGL rasterization rules.
	for(float z=0.9f; z>0.01f; z-=0.01f) {
		for(unsigned j=0; j<numLinesPerScreen; j++) {
			for(unsigned i=0; i<numTrianglesPerLine; i+=2) {

				double x=i/2*stride;
				double y=j*stride;

				// triangle 1, vertex 1
				vertices[idx++]=float((x+0.5-0.1)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5-0.1)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				// triangle 1, vertex 2
				vertices[idx++]=float((x+0.5+triangleSize-0.8)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5-0.1)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				// triangle 1, vertex 3
				vertices[idx++]=float((x+0.5-0.1)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5+triangleSize-0.8)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				if(idx==idxEnd)
					return;

				// triangle 2, vertex 1
				vertices[idx++]=float((x+0.5+triangleSize+0.6)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5+1.3)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				// triangle 2, vertex 2
				vertices[idx++]=float((x+0.5+1.3)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5+triangleSize+0.6)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				// triangle 2, vertex 3
				vertices[idx++]=float((x+0.5+triangleSize+0.6)*scaleX+offsetX);
				vertices[idx++]=float((y+0.5+triangleSize+0.6)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				if(idx==idxEnd)
					return;
			}
		}
	}

	// throw if we did not managed to put all the triangles in designed area
	throw std::runtime_error("Triangles do not fit into the rendered area.");
}


static void generateMatrices(float* matrices,uint32_t numMatrices,unsigned triangleSize,
                             unsigned regionWidth,unsigned regionHeight,
                             double scaleX=1.,double scaleY=1.,double offsetX=0.,double offsetY=0.)
{
	unsigned stride=triangleSize+2;
	unsigned numTrianglesPerLine=regionWidth/stride;
	unsigned numLinesPerScreen=regionHeight/stride;
	size_t idx=0;
	size_t idxEnd=size_t(numMatrices)*16;

	for(float z=0.9f; z>0.01f; z-=0.01f) {
		for(unsigned j=0; j<numLinesPerScreen; j++) {
			for(unsigned i=0; i<numTrianglesPerLine; i++) {

				double x=i*stride;
				double y=j*stride;

				float m[]{
					1.f,0.f,0.f,0.f,
					0.f,1.f,0.f,0.f,
					0.f,0.f,1.f,0.f,
					float(x*scaleX+offsetX), float(y*scaleY+offsetY), z-0.9f, 1.f
				};
				memcpy(&matrices[idx],m,sizeof(m));
				idx+=16;

				if(idx==idxEnd)
					return;

			}
		}
	}

	// throw if we did not managed to put all the triangles in designed area
	throw std::runtime_error("Triangles do not fit into the rendered area.");
}


static size_t getBufferSize(uint32_t numStrips,uint32_t numTrianglesInStrip,bool useVec4)
{
	return (2+numTrianglesInStrip)*size_t(numStrips)*(useVec4?4:3)*sizeof(float);
}


static void generateStrips(float* vertices,uint32_t numStrips,uint32_t numTrianglesInStrip,unsigned triangleSize,
                           unsigned regionWidth,unsigned regionHeight,bool useVec4,
                           double scaleX=1.,double scaleY=1.,double offsetX=0.,double offsetY=0.)
{
	unsigned spaceBetweenStrips=1;
	unsigned numStripsPerLine=regionWidth/(numTrianglesInStrip+spaceBetweenStrips);
	unsigned numLinesPerScreen=regionHeight/(triangleSize+1)-1;

	size_t idx=0;
	uint32_t stripCounter=0;

	for(float z=0.9f; z>0.01f; z-=0.01f) {

		for(unsigned k=0; k<numLinesPerScreen; k++) {

			double y=double(k*(triangleSize+1));
			double x=0.;

			for(unsigned j=0; j<numStripsPerLine; j++) {

				// starting triangle, vertex 1
				vertices[idx++]=float((x+0.1)*scaleX+offsetX);
				vertices[idx++]=float((y+0.6)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				// starting triangle, vertex 2
				vertices[idx++]=float((x+0.1)*scaleX+offsetX);
				vertices[idx++]=float((y+0.7+triangleSize)*scaleY+offsetY);
				vertices[idx++]=z;
				if(useVec4)
					vertices[idx++]=1.f;

				for(unsigned i=0; i<numTrianglesInStrip;) {

					x+=1.0;

					// vertex 3
					vertices[idx++]=float((x+0.1)*scaleX+offsetX);
					vertices[idx++]=float((y+0.6)*scaleY+offsetY);
					vertices[idx++]=z;
					if(useVec4)
						vertices[idx++]=1.f;

					i++;
					if(i>=numTrianglesInStrip)
						break;

					// vertex 4
					vertices[idx++]=float((x+0.1)*scaleX+offsetX);
					vertices[idx++]=float((y+0.7+triangleSize)*scaleY+offsetY);
					vertices[idx++]=z;
					if(useVec4)
						vertices[idx++]=1.f;

					i++;

				}

				x+=spaceBetweenStrips;

				stripCounter++;
				if(stripCounter>=numStrips)
					return;
			}
		}
	}

	// throw if we did not managed to put all the triangles in designed area
	throw std::runtime_error("Triangle strips do not fit into the rendered area.");
}


static size_t getIndexBufferSize(uint32_t numStrips,uint32_t numTrianglesInStrip)
{
	return 3*size_t(numTrianglesInStrip)*numStrips*sizeof(uint32_t);
}


static void generateStripIndices(uint32_t* indices,uint32_t numStrips,uint32_t numTrianglesInStrip)
{
	size_t idx=0;

	for(uint32_t j=0; j<numStrips; j++) {
		for(uint32_t i=j*(numTrianglesInStrip+2),e=i+numTrianglesInStrip; i<e; i++) {
			indices[idx++]=i;
			indices[idx++]=i+1;
			indices[idx++]=i+2;
		}
	}
}


static size_t getStripIndexPrimitiveRestartBufferSize(uint32_t numStrips,uint32_t numTrianglesInStrip,uint32_t numIndicesBetweenRestarts)
{
	auto numStripLets=(numTrianglesInStrip/(numIndicesBetweenRestarts-2));
	return size_t(numIndicesBetweenRestarts+1)*numStripLets*numStrips*sizeof(uint32_t);
}


static void generateStripPrimitiveRestartIndices(uint32_t* indices,uint32_t numStrips,uint32_t numTrianglesInStrip,uint32_t numIndicesBetweenRestarts)
{
	if(numTrianglesInStrip%(numIndicesBetweenRestarts-2)!=0)
		throw runtime_error("generateStripPrimitiveRestartIndices(): numIndicesBetweenRestarts must match "
		                    "numTrianglesInStrip in a way to be able to generate all the strips of the same "
		                    "length without any excessive small strip at the end.");

	size_t idx=0;

	for(uint32_t j=0; j<numStrips; j++) {
		for(uint32_t i=j*(numTrianglesInStrip+2),e=i+numTrianglesInStrip; i<e;) {
			for(uint32_t k=0; k<numIndicesBetweenRestarts; k++)
				indices[idx++]=i+k;
			indices[idx++]=-1;
			i+=numIndicesBetweenRestarts-2;
		}
	}
}


static size_t getPrimitiveRestartMinusOneBufferSize(uint32_t numStrips,uint32_t numTrianglesInStrip,uint32_t numMinusOneBetweenRestarts)
{
	return size_t(numTrianglesInStrip)*(3+numMinusOneBetweenRestarts)*numStrips*sizeof(uint32_t);
}


static void generatePrimitiveRestartMinusOneIndices(uint32_t* indices,uint32_t numStrips,uint32_t numTrianglesInStrip,uint32_t numMinusOneBetweenRestarts)
{
	size_t idx=0;

	for(uint32_t j=0; j<numStrips; j++) {
		for(uint32_t i=j*(numTrianglesInStrip+2),e=i+numTrianglesInStrip; i<e; i++) {
			indices[idx++]=i;
			indices[idx++]=i+1;
			indices[idx++]=i+2;
			for(uint32_t k=0; k<numMinusOneBetweenRestarts; k++)
				indices[idx++]=-1;
		}
	}
}


static size_t getBufferSizeForSharedVertexTriangles(uint32_t numStrips,uint32_t numTrianglesInStrip,bool useVec4)
{
	return 3*size_t(numTrianglesInStrip)*numStrips*(useVec4?4:3)*sizeof(float);
}


static void generateSharedVertexTriangles(
		float* vertices,uint32_t numStrips,uint32_t numTrianglesInStrip,unsigned triangleSize,
		unsigned regionWidth,unsigned regionHeight,bool useVec4,
		double scaleX=1.,double scaleY=1.,double offsetX=0.,double offsetY=0.)
{
	unsigned spaceBetweenStrips=1;
	unsigned numStripsPerLine=regionWidth/(numTrianglesInStrip+spaceBetweenStrips);
	unsigned numLinesPerScreen=regionHeight/(triangleSize+1)-1;

	size_t idx=0;
	uint32_t stripCounter=0;

	for(float z=0.9f; z>0.01f; z-=0.01f) {

		for(unsigned k=0; k<numLinesPerScreen; k++) {

			double y=double(k*(triangleSize+1));
			double x=0.;

			for(unsigned j=0; j<numStripsPerLine; j++) {

				for(unsigned i=0; i<numTrianglesInStrip;) {

					// triangle 1, vertex 1
					vertices[idx++]=float((x+0.1)*scaleX+offsetX);
					vertices[idx++]=float((y+0.6)*scaleY+offsetY);
					vertices[idx++]=z;
					if(useVec4)
						vertices[idx++]=1.f;

					// triangle 1, vertex 2
					auto idx12=idx;
					vertices[idx++]=float((x+0.1)*scaleX+offsetX);
					vertices[idx++]=float((y+0.7+triangleSize)*scaleY+offsetY);
					vertices[idx++]=z;
					if(useVec4)
						vertices[idx++]=1.f;

					x+=1.0;

					// triangle 1, vertex 3
					auto idx13=idx;
					vertices[idx++]=float((x+0.1)*scaleX+offsetX);
					vertices[idx++]=float((y+0.6)*scaleY+offsetY);
					vertices[idx++]=z;
					if(useVec4)
						vertices[idx++]=1.f;

					i++;
					if(i>=numTrianglesInStrip)
						break;

					// triangle 2, vertex 1
					vertices[idx++]=vertices[idx13++];
					vertices[idx++]=vertices[idx13++];
					vertices[idx++]=z;
					if(useVec4)
						vertices[idx++]=1.f;

					// triangle 2, vertex 2
					vertices[idx++]=vertices[idx12++];
					vertices[idx++]=vertices[idx12++];
					vertices[idx++]=z;
					if(useVec4)
						vertices[idx++]=1.f;

					// triangle 2, vertex 3
					vertices[idx++]=float((x+0.1)*scaleX+offsetX);
					vertices[idx++]=float((y+0.7+triangleSize)*scaleY+offsetY);
					vertices[idx++]=z;
					if(useVec4)
						vertices[idx++]=1.f;

					i++;

				}

				x+=spaceBetweenStrips;

				stripCounter++;
				if(stripCounter>=numStrips)
					return;
			}
		}
	}

	// throw if we did not managed to put all the triangles in designed area
	throw std::runtime_error("Triangle strips do not fit into the rendered area.");
}


// allocate memory for buffers
static tuple<vk::UniqueDeviceMemory,size_t> allocateMemory(vk::Buffer buffer,vk::MemoryPropertyFlags requiredFlags,size_t memorySizeDivisor)
{
	vk::MemoryRequirements memoryRequirements=device->getBufferMemoryRequirements(buffer);
	if(memorySizeDivisor!=1)
		memoryRequirements.size=(memoryRequirements.size/memorySizeDivisor+memoryBlockSize)&memoryBlockMask; // memoryBlockSize example value: 65536 and memoryBlockMask: ~0xffff

	vk::PhysicalDeviceMemoryProperties memoryProperties=physicalDevice.getMemoryProperties();
	for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++)
		if(memoryRequirements.memoryTypeBits&(1<<i))
			if((memoryProperties.memoryTypes[i].propertyFlags&requiredFlags)==requiredFlags)
				return tuple(
					device->allocateMemoryUnique(
						vk::MemoryAllocateInfo(
							memoryRequirements.size,  // allocationSize
							i                         // memoryTypeIndex
						)
					),
					memoryRequirements.size);
	throw std::runtime_error("No suitable memory type found for the buffer.");
}


// allocate memory for images
static vk::UniqueDeviceMemory allocateMemory(vk::Image image,vk::MemoryPropertyFlags requiredFlags)
{
	vk::MemoryRequirements memoryRequirements=device->getImageMemoryRequirements(image);
	vk::PhysicalDeviceMemoryProperties memoryProperties=physicalDevice.getMemoryProperties();
	for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++)
		if(memoryRequirements.memoryTypeBits&(1<<i))
			if((memoryProperties.memoryTypes[i].propertyFlags&requiredFlags)==requiredFlags)
				return
					device->allocateMemoryUnique(
						vk::MemoryAllocateInfo(
							memoryRequirements.size,  // allocationSize
							i                         // memoryTypeIndex
						)
					);
	throw std::runtime_error("No suitable memory type found for the image.");
}


/// Returns memory alignment for particular device, memory size and buffer flags.
static size_t getMemoryAlignment(vk::Device device,size_t size,vk::BufferCreateFlags flags)
{
	return
		device.getBufferMemoryRequirements(
			device.createBufferUnique(
				vk::BufferCreateInfo(
					flags,                        // flags
					size,                         // size
					vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr                       // pQueueFamilyIndices
				)
			).get())
			.alignment;
}


/// Init Vulkan and open the window.
static void init(size_t deviceIndex)
{
	// Vulkan instance
	instance=
		vk::createInstanceUnique(
			vk::InstanceCreateInfo{
				vk::InstanceCreateFlags(),  // flags
				&(const vk::ApplicationInfo&)vk::ApplicationInfo{
					"CADR Vk VulkanRenderingPerformance",  // application name
					VK_MAKE_VERSION(0,0,0),  // application version
					"CADR",                  // engine name
					VK_MAKE_VERSION(0,0,0),  // engine version
					VK_API_VERSION_1_0,      // api version
				},
				0,nullptr,  // no layers
				2,          // enabled extension count
#ifdef _WIN32
				array<const char*,2>{"VK_KHR_surface","VK_KHR_win32_surface"}.data(),  // enabled extension names
#else
				array<const char*,2>{"VK_KHR_surface","VK_KHR_xlib_surface"}.data(),  // enabled extension names
#endif
			});


#ifdef _WIN32

	// initial window size
	RECT screenSize;
	if(GetWindowRect(GetDesktopWindow(),&screenSize)==0)
		throw runtime_error("GetWindowRect() failed.");
	windowSize.setWidth(screenSize.right-screenSize.left);
	windowSize.setHeight(screenSize.bottom-screenSize.top);

	// window's message handling procedure
	auto wndProc=[](HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)->LRESULT {
		switch(msg)
		{
			case WM_SIZE:
				needResize=true;
				windowSize.setWidth(LOWORD(lParam));
				windowSize.setHeight(HIWORD(lParam));
				return DefWindowProc(hwnd,msg,wParam,lParam);
			case WM_CLOSE:
				DestroyWindow(hwnd);
				UnregisterClass("RenderingWindow",GetModuleHandle(NULL));
				window=nullptr;
				return 0;
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			default:
				return DefWindowProc(hwnd,msg,wParam,lParam);
		}
	};

	// register window class
	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = wndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle(NULL);
	wc.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "RenderingWindow";
	wc.hIconSm       = LoadIcon(NULL,IDI_APPLICATION);
	if(!RegisterClassEx(&wc))
		throw runtime_error("Can not register window class.");

	// create window
	window=CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"RenderingWindow",
		"Rendering performance",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,windowSize.width,windowSize.height,
		NULL,NULL,wc.hInstance,NULL);
	if(window==NULL) {
		UnregisterClass("RenderingWindow",GetModuleHandle(NULL));
		throw runtime_error("Can not create window.");
	}

	// show window
	ShowWindow(window,SW_SHOWDEFAULT);

	// create surface
	surface=instance->createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(),wc.hInstance,window));

#else

	// open X connection
	display=XOpenDisplay(nullptr);
	if(display==nullptr)
		throw runtime_error("Can not open display. No X-server running or wrong DISPLAY variable.");

	// create window
	int blackColor=BlackPixel(display,DefaultScreen(display));
	Screen* screen=XDefaultScreenOfDisplay(display);
	windowSize.setWidth(XWidthOfScreen(screen));
	windowSize.setHeight(XHeightOfScreen(screen));
	window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,windowSize.width,
	                           windowSize.height,0,blackColor,blackColor);
	XSetStandardProperties(display,window,"Rendering performance",NULL,None,NULL,0,NULL);
	XSelectInput(display,window,StructureNotifyMask);
	wmDeleteMessage=XInternAtom(display,"WM_DELETE_WINDOW",False);
	XSetWMProtocols(display,window,&wmDeleteMessage,1);
	XMapWindow(display,window);

	// create surface
	surface=instance->createXlibSurfaceKHRUnique(vk::XlibSurfaceCreateInfoKHR(vk::XlibSurfaceCreateFlagsKHR(),display,window));

#endif

	// find compatible devices
	// (On Windows, all graphics adapters capable of monitor output are usually compatible devices.
	// On Linux X11 platform, only one graphics adapter is compatible device (the one that
	// renders the window).
	vector<vk::PhysicalDevice> deviceList=instance->enumeratePhysicalDevices();
	vector<tuple<vk::PhysicalDevice,uint32_t>> compatibleDevicesSingleQueue;
	vector<tuple<vk::PhysicalDevice,uint32_t,uint32_t,uint32_t>> compatibleDevicesThreeQueues;
	for(vk::PhysicalDevice pd:deviceList) {

		// skip devices without VK_KHR_swapchain
		auto extensionList=pd.enumerateDeviceExtensionProperties();
		for(vk::ExtensionProperties& e:extensionList)
			if(strcmp(e.extensionName,"VK_KHR_swapchain")==0)
				goto swapchainSupported;
		continue;
		swapchainSupported:

		// skip devices without surface formats and presentation modes
		uint32_t formatCount;
		vk::createResultValue(
			pd.getSurfaceFormatsKHR(surface.get(),&formatCount,nullptr,vk::DispatchLoaderStatic()),
			VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfaceFormatsKHR");
		uint32_t presentationModeCount;
		vk::createResultValue(
			pd.getSurfacePresentModesKHR(surface.get(),&presentationModeCount,nullptr,vk::DispatchLoaderStatic()),
			VULKAN_HPP_NAMESPACE_STRING"::PhysicalDevice::getSurfacePresentModesKHR");
		if(formatCount==0||presentationModeCount==0)
			continue;

		// select queues (for graphics rendering and for presentation)
		uint32_t graphicsQueueFamily=UINT32_MAX;
		uint32_t presentationQueueFamily=UINT32_MAX;
		uint32_t sparseQueueFamily=UINT32_MAX;
		uint32_t graphicsAndPresentationQueueFamily=UINT32_MAX;
		vector<vk::QueueFamilyProperties> queueFamilyList=pd.getQueueFamilyProperties();
		uint32_t i=0;
		for(auto it=queueFamilyList.begin(); it!=queueFamilyList.end(); it++,i++) {
			bool p=pd.getSurfaceSupportKHR(i,surface.get())!=0;
			if(it->queueFlags&vk::QueueFlagBits::eGraphics) {
				if(p) {
					if(it->queueFlags&vk::QueueFlagBits::eSparseBinding) {
						compatibleDevicesSingleQueue.emplace_back(pd,i);
						goto nextDevice;
					}
					else
						if(graphicsAndPresentationQueueFamily==UINT32_MAX)
							graphicsAndPresentationQueueFamily=i;
				}
				else {
					if(graphicsQueueFamily==UINT32_MAX)
						graphicsQueueFamily=i;
				}
			}
			else {
				if(p)
					if(presentationQueueFamily==UINT32_MAX)
						presentationQueueFamily=i;
			}
			if(sparseQueueFamily==UINT32_MAX && it->queueFlags&vk::QueueFlagBits::eSparseBinding)
				sparseQueueFamily=i;
		}
		if(sparseMode==SPARSE_NONE) {
			if(graphicsAndPresentationQueueFamily!=UINT32_MAX)
				compatibleDevicesSingleQueue.emplace_back(pd,graphicsAndPresentationQueueFamily);
			else
				if(graphicsQueueFamily!=UINT32_MAX && presentationQueueFamily!=UINT32_MAX)
					compatibleDevicesThreeQueues.emplace_back(pd,graphicsQueueFamily,presentationQueueFamily,sparseQueueFamily);
		}
		else
			if(sparseQueueFamily!=UINT32_MAX) {
				if(graphicsAndPresentationQueueFamily!=UINT32_MAX)
					compatibleDevicesThreeQueues.emplace_back(pd,graphicsAndPresentationQueueFamily,graphicsAndPresentationQueueFamily,sparseQueueFamily);
				else
					if(graphicsQueueFamily!=UINT32_MAX && presentationQueueFamily!=UINT32_MAX)
						compatibleDevicesThreeQueues.emplace_back(pd,graphicsQueueFamily,presentationQueueFamily,sparseQueueFamily);
			}
		nextDevice:;
	}
	cout<<"Compatible devices:"<<endl;
	for(auto& t:compatibleDevicesSingleQueue)
		cout<<"   "<<get<0>(t).getProperties().deviceName<<endl;
	for(auto& t:compatibleDevicesThreeQueues)
		cout<<"   "<<get<0>(t).getProperties().deviceName<<endl;

	// choose device
	if(deviceIndex<compatibleDevicesSingleQueue.size()) {
		auto t=compatibleDevicesSingleQueue[deviceIndex];
		physicalDevice=get<0>(t);
		graphicsQueueFamily=get<1>(t);
		presentationQueueFamily=graphicsQueueFamily;
		sparseQueueFamily=graphicsQueueFamily;
	}
	else if((deviceIndex-compatibleDevicesSingleQueue.size())<compatibleDevicesThreeQueues.size()) {
		auto t=compatibleDevicesThreeQueues[deviceIndex-compatibleDevicesSingleQueue.size()];
		physicalDevice=get<0>(t);
		graphicsQueueFamily=get<1>(t);
		presentationQueueFamily=get<2>(t);
		sparseQueueFamily=get<3>(t);
	}
	else
		throw runtime_error("No compatible devices.");
	vk::PhysicalDeviceProperties pdProperties=physicalDevice.getProperties();
	cout<<"Using device:\n"
	      "   "<<pdProperties.deviceName<<endl;

	// get supported features
	vk::PhysicalDeviceFeatures physicalFeatures=physicalDevice.getFeatures();
	enabledFeatures.setMultiDrawIndirect(physicalFeatures.multiDrawIndirect);
	enabledFeatures.setGeometryShader(physicalFeatures.geometryShader);
	enabledFeatures.setShaderFloat64(physicalFeatures.shaderFloat64);

	// GPU memory and sparse props
	size_t gpuMemory=0;
	{
		vk::PhysicalDeviceMemoryProperties memoryProperties=physicalDevice.getMemoryProperties();
		for(uint32_t i=0; i<memoryProperties.memoryHeapCount; i++)
			if(memoryProperties.memoryHeaps[i].flags&vk::MemoryHeapFlagBits::eDeviceLocal) {
				gpuMemory=memoryProperties.memoryHeaps[i].size;
				break;
			}
	}
	cout<<"GPU memory:  "<<gpuMemory/1024/1024/1024<<" GiB"<<endl;
	cout<<"Max memory allocations: "<<pdProperties.limits.maxMemoryAllocationCount<<endl;

	switch(sparseMode) {
	case SPARSE_NONE:
		break;
	case SPARSE_BINDING:
		if(!physicalFeatures.sparseBinding)
			throw runtime_error("Sparse binding is not supported.");
		enabledFeatures.setSparseBinding(true);
		bufferCreateFlags=vk::BufferCreateFlagBits::eSparseBinding;
		bufferSizeMultiplier=1;
		break;
	case SPARSE_RESIDENCY:
		if(!physicalFeatures.sparseBinding || !physicalFeatures.sparseResidencyBuffer)
			throw runtime_error("Sparse residency is not supported.");
		enabledFeatures.setSparseBinding(true);
		enabledFeatures.setSparseResidencyBuffer(true);
		bufferCreateFlags=vk::BufferCreateFlagBits::eSparseBinding|vk::BufferCreateFlagBits::eSparseResidency;
		bufferSizeMultiplier=10;
		break;
	case SPARSE_RESIDENCY_ALIASED:
		if(!physicalFeatures.sparseBinding || !physicalFeatures.sparseResidencyBuffer)
			throw runtime_error("Sparse residency is not supported.");
		enabledFeatures.setSparseBinding(true);
		enabledFeatures.setSparseResidencyBuffer(true);
		if(!physicalFeatures.sparseResidencyAliased)
			throw runtime_error("Sparse aliased is not supported.");
		enabledFeatures.setSparseResidencyAliased(true);
		bufferCreateFlags=vk::BufferCreateFlagBits::eSparseBinding|vk::BufferCreateFlagBits::eSparseResidency|vk::BufferCreateFlagBits::eSparseAliased;
		bufferSizeMultiplier=10;
		break;
	}

	// create device
	device=
		physicalDevice.createDeviceUnique(
			vk::DeviceCreateInfo{
				vk::DeviceCreateFlags(),  // flags
				(sparseMode==SPARSE_NONE)  // queueCreateInfoCount
					?(presentationQueueFamily==graphicsQueueFamily)?uint32_t(1):uint32_t(2)
					:(presentationQueueFamily==graphicsQueueFamily && sparseQueueFamily==graphicsQueueFamily)
						?uint32_t(1):uint32_t(3),
				array<const vk::DeviceQueueCreateInfo,3>{  // pQueueCreateInfos
					vk::DeviceQueueCreateInfo{
						vk::DeviceQueueCreateFlags(),
						graphicsQueueFamily,
						1,
						&(const float&)1.f,
					},
					vk::DeviceQueueCreateInfo{
						vk::DeviceQueueCreateFlags(),
						presentationQueueFamily,
						1,
						&(const float&)1.f,
					},
					vk::DeviceQueueCreateInfo{
						vk::DeviceQueueCreateFlags(),
						sparseQueueFamily,
						1,
						&(const float&)1.f,
					},
				}.data(),
				0,nullptr,  // no layers
				1,          // number of enabled extensions
				array<const char*,1>{"VK_KHR_swapchain"}.data(),  // enabled extension names
				&enabledFeatures,  // enabled features
			}
		);

	// get queues
	graphicsQueue=device->getQueue(graphicsQueueFamily,0);
	presentationQueue=device->getQueue(presentationQueueFamily,0);
	if(sparseQueueFamily!=UINT32_MAX)
		sparseQueue=device->getQueue(sparseQueueFamily,0);

	// print memory alignment
	cout<<"Standard (non-sparse) buffer alignment: "
	    <<getMemoryAlignment(device.get(),1,vk::BufferCreateFlags())<<endl;

	// set sparse block variables
	if(sparseMode==SPARSE_NONE) {
		memoryBlockSize=1;
		memoryBlockMask=~0;
	} else {
		memoryBlockSize=getMemoryAlignment(device.get(),1,bufferCreateFlags);
		memoryBlockMask=~(memoryBlockSize-1);
		sparseBlockSize=memoryBlockSize;
	}

	// number of triangles
	// (reduce the number on integrated graphics as it may easily run out of memory)
	if(gpuMemory>=2.8*1024*1024*1024) // >=2.8GiB
		numTriangles=numTrianglesStandard;
	else if(gpuMemory>=1.4*1024*1024*1024) // >=1.4GiB
		numTriangles=numTrianglesStandard/2;
	else if(gpuMemory>=700*1024*1024) // >=700MiB
		numTriangles=numTrianglesStandard/5;
	else if(gpuMemory>=400*1024*1024) // >=400MiB
		numTriangles=numTrianglesStandard/10;
	else
		numTriangles=numTrianglesStandard/20;
	if(physicalDevice.getProperties().deviceType==vk::PhysicalDeviceType::eIntegratedGpu)
		numTriangles=min(numTriangles,numTrianglesReduced);
	cout<<"Number of triangles for tests: "<<numTriangles<<endl;

	// print sparse mode for tests
	switch(sparseMode) {
	case SPARSE_NONE:              cout<<"Sparse mode for tests: None"<<endl; break;
	case SPARSE_BINDING:           cout<<"Sparse mode for tests: Binding"<<endl; break;
	case SPARSE_RESIDENCY:         cout<<"Sparse mode for tests: Residency"<<endl; break;
	case SPARSE_RESIDENCY_ALIASED: cout<<"Sparse mode for tests: ResidencyAliased"<<endl; break;
	default: assert(0 && "This should never happen.");
	}

	// choose surface format
	vector<vk::SurfaceFormatKHR> surfaceFormats=physicalDevice.getSurfaceFormatsKHR(surface.get());
	const vk::SurfaceFormatKHR wantedSurfaceFormat{vk::Format::eB8G8R8A8Unorm,vk::ColorSpaceKHR::eSrgbNonlinear};
	chosenSurfaceFormat=
		surfaceFormats.size()==1&&surfaceFormats[0].format==vk::Format::eUndefined
			?wantedSurfaceFormat
			:std::find(surfaceFormats.begin(),surfaceFormats.end(),
			           wantedSurfaceFormat)!=surfaceFormats.end()
				           ?wantedSurfaceFormat
				           :surfaceFormats[0];
	depthFormat=
		[](){
			for(vk::Format f:array<vk::Format,3>{vk::Format::eD32Sfloat,vk::Format::eD32SfloatS8Uint,vk::Format::eD24UnormS8Uint}) {
				vk::FormatProperties p=physicalDevice.getFormatProperties(f);
				if(p.optimalTilingFeatures&vk::FormatFeatureFlagBits::eDepthStencilAttachment)
					return f;
			}
			throw std::runtime_error("No suitable depth buffer format.");
		}();

	// render pass
	renderPass=
		device->createRenderPassUnique(
			vk::RenderPassCreateInfo(
				vk::RenderPassCreateFlags(),  // flags
				2,                            // attachmentCount
				array<const vk::AttachmentDescription,2>{  // pAttachments
					vk::AttachmentDescription{  // color attachment
						vk::AttachmentDescriptionFlags(),  // flags
						chosenSurfaceFormat.format,        // format
						vk::SampleCountFlagBits::e1,       // samples
						vk::AttachmentLoadOp::eClear,      // loadOp
						vk::AttachmentStoreOp::eStore,     // storeOp
						vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
						vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
						vk::ImageLayout::eUndefined,       // initialLayout
						vk::ImageLayout::ePresentSrcKHR    // finalLayout
					},
					vk::AttachmentDescription{  // depth attachment
						vk::AttachmentDescriptionFlags(),  // flags
						depthFormat,                       // format
						vk::SampleCountFlagBits::e1,       // samples
						vk::AttachmentLoadOp::eClear,      // loadOp
						vk::AttachmentStoreOp::eDontCare,  // storeOp
						vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
						vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
						vk::ImageLayout::eUndefined,       // initialLayout
						vk::ImageLayout::eDepthStencilAttachmentOptimal  // finalLayout
					}
				}.data(),
				1,  // subpassCount
				&(const vk::SubpassDescription&)vk::SubpassDescription(  // pSubpasses
					vk::SubpassDescriptionFlags(),     // flags
					vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
					0,        // inputAttachmentCount
					nullptr,  // pInputAttachments
					1,        // colorAttachmentCount
					&(const vk::AttachmentReference&)vk::AttachmentReference(  // pColorAttachments
						0,  // attachment
						vk::ImageLayout::eColorAttachmentOptimal  // layout
					),
					nullptr,  // pResolveAttachments
					&(const vk::AttachmentReference&)vk::AttachmentReference(  // pDepthStencilAttachment
						1,  // attachment
						vk::ImageLayout::eDepthStencilAttachmentOptimal  // layout
					),
					0,        // preserveAttachmentCount
					nullptr   // pPreserveAttachments
				),
				1,  // dependencyCount
				&(const vk::SubpassDependency&)vk::SubpassDependency(  // pDependencies
					0,                     // srcSubpass
					0,                     // dstSubpass
					vk::PipelineStageFlagBits::eAllGraphics,  // srcStageMask
					vk::PipelineStageFlagBits::eColorAttachmentOutput|vk::PipelineStageFlagBits::eTopOfPipe,  // dstStageMask
					vk::AccessFlags(),     // srcAccessMask
					vk::AccessFlagBits::eColorAttachmentRead|vk::AccessFlagBits::eColorAttachmentWrite,  // dstAccessMask
					vk::DependencyFlags()  // dependencyFlags
				)
			)
		);

	// create shader modules
	attributelessConstantOutputVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(attributelessConstantOutputVS_spirv),  // codeSize
				attributelessConstantOutputVS_spirv           // pCode
			)
		);
	attributelessInputIndicesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(attributelessInputIndicesVS_spirv),  // codeSize
				attributelessInputIndicesVS_spirv           // pCode
			)
		);
	coordinateAttributeVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),        // flags
				sizeof(coordinateAttributeVS_spirv),  // codeSize
				coordinateAttributeVS_spirv           // pCode
			)
		);
	coordinateBufferVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),     // flags
				sizeof(coordinateBufferVS_spirv),  // codeSize
				coordinateBufferVS_spirv           // pCode
			)
		);
	singleUniformMatrixVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),        // flags
				sizeof(singleUniformMatrixVS_spirv),  // codeSize
				singleUniformMatrixVS_spirv           // pCode
			)
		);
	matrixAttributeVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),    // flags
				sizeof(matrixAttributeVS_spirv),  // codeSize
				matrixAttributeVS_spirv           // pCode
			)
		);
	matrixBufferVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(matrixBufferVS_spirv),   // codeSize
				matrixBufferVS_spirv            // pCode
			)
		);
	twoAttributesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(twoAttributesVS_spirv),  // codeSize
				twoAttributesVS_spirv           // pCode
			)
		);
	twoPackedAttributesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),        // flags
				sizeof(twoPackedAttributesVS_spirv),  // codeSize
				twoPackedAttributesVS_spirv           // pCode
			)
		);
	twoPackedBuffersVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),     // flags
				sizeof(twoPackedBuffersVS_spirv),  // codeSize
				twoPackedBuffersVS_spirv           // pCode
			)
		);
	twoPackedBuffersUsingStructVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                // flags
				sizeof(twoPackedBuffersUsingStructVS_spirv),  // codeSize
				twoPackedBuffersUsingStructVS_spirv           // pCode
			)
		);
	twoPackedBuffersUsingStructSlowVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                    // flags
				sizeof(twoPackedBuffersUsingStructSlowVS_spirv),  // codeSize
				twoPackedBuffersUsingStructSlowVS_spirv           // pCode
			)
		);
	singlePackedBufferVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),       // flags
				sizeof(singlePackedBufferVS_spirv),  // codeSize
				singlePackedBufferVS_spirv           // pCode
			)
		);
	twoPackedAttributesAndSingleMatrixVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                       // flags
				sizeof(twoPackedAttributesAndSingleMatrixVS_spirv),  // codeSize
				twoPackedAttributesAndSingleMatrixVS_spirv           // pCode
			)
		);
	twoPackedAttributesAndMatrixVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                 // flags
				sizeof(twoPackedAttributesAndMatrixVS_spirv),  // codeSize
				twoPackedAttributesAndMatrixVS_spirv           // pCode
			)
		);
	twoPackedBuffersAndMatrixVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),              // flags
				sizeof(twoPackedBuffersAndMatrixVS_spirv),  // codeSize
				twoPackedBuffersAndMatrixVS_spirv           // pCode
			)
		);
	fourAttributesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),   // flags
				sizeof(fourAttributesVS_spirv),  // codeSize
				fourAttributesVS_spirv           // pCode
			)
		);
	fourAttributesAndMatrixVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),            // flags
				sizeof(fourAttributesAndMatrixVS_spirv),  // codeSize
				fourAttributesAndMatrixVS_spirv           // pCode
			)
		);
	geometryShaderConstantOutputVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                 // flags
				sizeof(geometryShaderConstantOutputVS_spirv),  // codeSize
				geometryShaderConstantOutputVS_spirv           // pCode
			)
		);
	geometryShaderConstantOutputGS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                 // flags
				sizeof(geometryShaderConstantOutputGS_spirv),  // codeSize
				geometryShaderConstantOutputGS_spirv           // pCode
			)
		);
	geometryShaderVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),   // flags
				sizeof(geometryShaderVS_spirv),  // codeSize
				geometryShaderVS_spirv           // pCode
			)
		);
	geometryShaderGS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),   // flags
				sizeof(geometryShaderGS_spirv),  // codeSize
				geometryShaderGS_spirv           // pCode
			)
		);
	transformationThreeMatricesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                // flags
				sizeof(transformationThreeMatricesVS_spirv),  // codeSize
				transformationThreeMatricesVS_spirv           // pCode
			)
		);
	transformationFiveMatricesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),               // flags
				sizeof(transformationFiveMatricesVS_spirv),  // codeSize
				transformationFiveMatricesVS_spirv           // pCode
			)
		);
	transformationFiveMatricesUsingGS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                    // flags
				sizeof(transformationFiveMatricesUsingGS_spirv),  // codeSize
				transformationFiveMatricesUsingGS_spirv           // pCode
			)
		);
	transformationFiveMatricesUsingGSAndAttributesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                                   // flags
				sizeof(transformationFiveMatricesUsingGSAndAttributesVS_spirv),  // codeSize
				transformationFiveMatricesUsingGSAndAttributesVS_spirv           // pCode
			)
		);
	transformationFiveMatricesUsingGSAndAttributesGS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                                   // flags
				sizeof(transformationFiveMatricesUsingGSAndAttributesGS_spirv),  // codeSize
				transformationFiveMatricesUsingGSAndAttributesGS_spirv           // pCode
			)
		);
	phongTexturedFourAttributesFiveMatricesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                            // flags
				sizeof(phongTexturedFourAttributesFiveMatricesVS_spirv),  // codeSize
				phongTexturedFourAttributesFiveMatricesVS_spirv           // pCode
			)
		);
	phongTexturedFourAttributesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                // flags
				sizeof(phongTexturedFourAttributesVS_spirv),  // codeSize
				phongTexturedFourAttributesVS_spirv           // pCode
			)
		);
	phongTexturedVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(phongTexturedVS_spirv),  // codeSize
				phongTexturedVS_spirv           // pCode
			)
		);
	phongTexturedRowMajorVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),          // flags
				sizeof(phongTexturedRowMajorVS_spirv),  // codeSize
				phongTexturedRowMajorVS_spirv           // pCode
			)
		);
	phongTexturedMat4x3VS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),        // flags
				sizeof(phongTexturedMat4x3VS_spirv),  // codeSize
				phongTexturedMat4x3VS_spirv           // pCode
			)
		);
	phongTexturedMat4x3RowMajorVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                // flags
				sizeof(phongTexturedMat4x3RowMajorVS_spirv),  // codeSize
				phongTexturedMat4x3RowMajorVS_spirv           // pCode
			)
		);
	phongTexturedQuat1VS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),       // flags
				sizeof(phongTexturedQuat1VS_spirv),  // codeSize
				phongTexturedQuat1VS_spirv           // pCode
			)
		);
	phongTexturedQuat2VS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),       // flags
				sizeof(phongTexturedQuat2VS_spirv),  // codeSize
				phongTexturedQuat2VS_spirv           // pCode
			)
		);
	phongTexturedQuat3VS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),       // flags
				sizeof(phongTexturedQuat3VS_spirv),  // codeSize
				phongTexturedQuat3VS_spirv           // pCode
			)
		);
	phongTexturedQuat2PrimitiveRestartVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                       // flags
				sizeof(phongTexturedQuat2PrimitiveRestartVS_spirv),  // codeSize
				phongTexturedQuat2PrimitiveRestartVS_spirv           // pCode
			)
		);
	phongTexturedSingleQuat2VS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),             // flags
				sizeof(phongTexturedSingleQuat2VS_spirv),  // codeSize
				phongTexturedSingleQuat2VS_spirv           // pCode
			)
		);
	phongTexturedDMatricesOnlyInputVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                    // flags
				sizeof(phongTexturedDMatricesOnlyInputVS_spirv),  // codeSize
				phongTexturedDMatricesOnlyInputVS_spirv           // pCode
			)
		);
	phongTexturedDMatricesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),           // flags
				sizeof(phongTexturedDMatricesVS_spirv),  // codeSize
				phongTexturedDMatricesVS_spirv           // pCode
			)
		);
	phongTexturedDMatricesDVerticesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                    // flags
				sizeof(phongTexturedDMatricesDVerticesVS_spirv),  // codeSize
				phongTexturedDMatricesDVerticesVS_spirv           // pCode
			)
		);
	phongTexturedInGSDMatricesDVerticesVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                        // flags
				sizeof(phongTexturedInGSDMatricesDVerticesVS_spirv),  // codeSize
				phongTexturedInGSDMatricesDVerticesVS_spirv           // pCode
			)
		);
	phongTexturedInGSDMatricesDVerticesGS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                        // flags
				sizeof(phongTexturedInGSDMatricesDVerticesGS_spirv),  // codeSize
				phongTexturedInGSDMatricesDVerticesGS_spirv           // pCode
			)
		);
	constantColorFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(constantColorFS_spirv),  // codeSize
				constantColorFS_spirv           // pCode
			)
		);
	phongTexturedDummyFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),       // flags
				sizeof(phongTexturedDummyFS_spirv),  // codeSize
				phongTexturedDummyFS_spirv           // pCode
			)
		);
	phongTexturedFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(phongTexturedFS_spirv),  // codeSize
				phongTexturedFS_spirv           // pCode
			)
		);
	phongTexturedNotPackedFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),           // flags
				sizeof(phongTexturedNotPackedFS_spirv),  // codeSize
				phongTexturedNotPackedFS_spirv           // pCode
			)
		);
	fullscreenQuadVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),   // flags
				sizeof(fullscreenQuadVS_spirv),  // codeSize
				fullscreenQuadVS_spirv           // pCode
			)
		);
	fullscreenQuadFourInterpolatorsVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                    // flags
				sizeof(fullscreenQuadFourInterpolatorsVS_spirv),  // codeSize
				fullscreenQuadFourInterpolatorsVS_spirv           // pCode
			)
		);
	fullscreenQuadFourSmoothInterpolatorsFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                          // flags
				sizeof(fullscreenQuadFourSmoothInterpolatorsFS_spirv),  // codeSize
				fullscreenQuadFourSmoothInterpolatorsFS_spirv           // pCode
			)
		);
	fullscreenQuadFourFlatInterpolatorsFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                        // flags
				sizeof(fullscreenQuadFourFlatInterpolatorsFS_spirv),  // codeSize
				fullscreenQuadFourFlatInterpolatorsFS_spirv           // pCode
			)
		);
	fullscreenQuadTexturedPhongInterpolatorsVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                             // flags
				sizeof(fullscreenQuadTexturedPhongInterpolatorsVS_spirv),  // codeSize
				fullscreenQuadTexturedPhongInterpolatorsVS_spirv           // pCode
			)
		);
	fullscreenQuadTexturedPhongInterpolatorsFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                             // flags
				sizeof(fullscreenQuadTexturedPhongInterpolatorsFS_spirv),  // codeSize
				fullscreenQuadTexturedPhongInterpolatorsFS_spirv           // pCode
			)
		);
	uniformColor4fFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),   // flags
				sizeof(uniformColor4fFS_spirv),  // codeSize
				uniformColor4fFS_spirv           // pCode
			)
		);
	uniformColor4bFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),   // flags
				sizeof(uniformColor4bFS_spirv),  // codeSize
				uniformColor4bFS_spirv           // pCode
			)
		);
	fullscreenQuadTwoVec3InterpolatorsVS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                       // flags
				sizeof(fullscreenQuadTwoVec3InterpolatorsVS_spirv),  // codeSize
				fullscreenQuadTwoVec3InterpolatorsVS_spirv           // pCode
			)
		);
	phongNoSpecularFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),    // flags
				sizeof(phongNoSpecularFS_spirv),  // codeSize
				phongNoSpecularFS_spirv           // pCode
			)
		);
	phongNoSpecularSingleUniformFS=
		device->createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),                 // flags
				sizeof(phongNoSpecularSingleUniformFS_spirv),  // codeSize
				phongNoSpecularSingleUniformFS_spirv           // pCode
			)
		);

	// pipeline cache
	pipelineCache=
		device->createPipelineCacheUnique(
			vk::PipelineCacheCreateInfo(
				vk::PipelineCacheCreateFlags(),  // flags
				0,       // initialDataSize
				nullptr  // pInitialData
			)
		);

	// descriptor set layouts
	oneUniformVSDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				1,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,1>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	oneUniformFSDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				1,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,1>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eFragment,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	oneBufferDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				1,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,1>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	twoBuffersDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				2,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,2>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	threeBuffersDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				3,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,3>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						2,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	threeBuffersInGSDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				3,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,3>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						2,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	threeUniformFSDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				3,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,3>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eFragment,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eFragment,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						2,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eFragment,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	bufferAndUniformDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				2,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,2>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	bufferAndUniformInGSDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				2,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,2>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	twoBuffersAndUniformDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				3,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,3>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						2,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	twoBuffersAndUniformInGSDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				3,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,3>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						2,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	fourBuffersAndUniformInGSDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				5,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,5>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						2,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						3,  // binding
						vk::DescriptorType::eStorageBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						4,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eGeometry,  // stageFlags
						nullptr  // pImmutableSamplers
					)
				}.data()
			)
		);
	phongTexturedDescriptorSetLayout=
		device->createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),  // flags
				4,  // bindingCount
				array<vk::DescriptorSetLayoutBinding,4>{  // pBindings
					vk::DescriptorSetLayoutBinding(
						0,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eFragment,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						1,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eFragment,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						2,  // binding
						vk::DescriptorType::eUniformBuffer,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eFragment,  // stageFlags
						nullptr  // pImmutableSamplers
					),
					vk::DescriptorSetLayoutBinding(
						3,  // binding
						vk::DescriptorType::eCombinedImageSampler,  // descriptorType
						1,  // descriptorCount
						vk::ShaderStageFlagBits::eFragment,  // stageFlags
						nullptr  // pImmutableSamplers
					),
				}.data()
			)
		);

	// pipeline layouts
	simplePipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				0,       // setLayoutCount
				nullptr, // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	oneUniformVSPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&oneUniformVSDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	oneUniformFSPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&oneUniformFSDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	oneBufferPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&oneBufferDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	twoBuffersPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&twoBuffersDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	threeBuffersPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&threeBuffersDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	threeBuffersInGSPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&threeBuffersInGSDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	threeUniformFSPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&threeUniformFSDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	bufferAndUniformPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&bufferAndUniformDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	bufferAndUniformInGSPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&bufferAndUniformInGSDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	twoBuffersAndUniformPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&twoBuffersAndUniformDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	twoBuffersAndUniformInGSPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&twoBuffersAndUniformInGSDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	fourBuffersAndUniformInGSPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&fourBuffersAndUniformInGSDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);
	phongTexturedPipelineLayout=
		device->createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				1,       // setLayoutCount
				&phongTexturedDescriptorSetLayout.get(),  // pSetLayouts
				0,       // pushConstantRangeCount
				nullptr  // pPushConstantRanges
			}
		);

	// textures
	singleTexelImage=
		device->createImageUnique(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				vk::Format::eR8G8B8A8Unorm,  // format
				vk::Extent3D(1,1,1),     // extent
				1,                       // mipLevels
				1,                       // arrayLayers
				vk::SampleCountFlagBits::e1,  // samples
				vk::ImageTiling::eOptimal,    // tiling
				vk::ImageUsageFlagBits::eSampled|vk::ImageUsageFlagBits::eTransferDst,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr,                      // pQueueFamilyIndices
				vk::ImageLayout::eUndefined   // initialLayout
			)
		);
	singleTexelImageMemory=allocateMemory(singleTexelImage.get(),vk::MemoryPropertyFlagBits::eDeviceLocal);
	device->bindImageMemory(
		singleTexelImage.get(),  // image
		singleTexelImageMemory.get(),  // memory
		0  // memoryOffset
	);
	singleTexelImageView=
		device->createImageViewUnique(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),  // flags
				singleTexelImage.get(),      // image
				vk::ImageViewType::e2D,      // viewType
				vk::Format::eR8G8B8A8Unorm,  // format
				vk::ComponentMapping(),      // components
				vk::ImageSubresourceRange(   // subresourceRange
					vk::ImageAspectFlagBits::eColor,  // aspectMask
					0,  // baseMipLevel
					1,  // levelCount
					0,  // baseArrayLayer
					1   // layerCount
				)
			)
		);
	trilinearSampler=
		device->createSamplerUnique(
			vk::SamplerCreateInfo(
				vk::SamplerCreateFlags(),  // flags
				vk::Filter::eLinear,   // magFilter
				vk::Filter::eLinear,   // minFilter
				vk::SamplerMipmapMode::eLinear,  // mipmapMode
				vk::SamplerAddressMode::eClampToEdge,  // addressModeU
				vk::SamplerAddressMode::eClampToEdge,  // addressModeV
				vk::SamplerAddressMode::eClampToEdge,  // addressModeW
				0.f,  // mipLodBias
				VK_FALSE,  // anisotropyEnable
				0.f,  // maxAnisotropy
				VK_FALSE,  // compareEnable
				vk::CompareOp::eNever,  // compareOp
				0.f,  // minLod
				0.f,  // maxLod
				vk::BorderColor::eFloatTransparentBlack,  // borderColor
				VK_FALSE  // unnormalizedCoordinates
			)
		);

	// command pool
	commandPool=
		device->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlags(),  // flags
				graphicsQueueFamily  // queueFamilyIndex
			)
		);

	// semaphores
	imageAvailableSemaphore=
		device->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()  // flags
			)
		);
	renderFinishedSemaphore=
		device->createSemaphoreUnique(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()  // flags
			)
		);
}


/// Recreate swapchain and pipeline. The function is usually used on each window resize event and on application start.
static void recreateSwapchainAndPipeline()
{
	// print new size
	cout<<"New window size: "<<currentSurfaceExtent.width<<"x"<<currentSurfaceExtent.height<<endl;

	// stop device and clear resources
	device->waitIdle();
	commandBuffers.clear();
	framebuffers.clear();
	depthImage.reset();
	depthImageMemory.reset();
	depthImageView.reset();
	attributelessConstantOutputPipeline.reset();
	attributelessInputIndicesPipeline.reset();
	coordinateAttributePipeline.reset();
	coordinateBufferPipeline.reset();
	singleMatrixUniformPipeline.reset();
	matrixAttributePipeline.reset();
	matrixBufferPipeline.reset();
	twoAttributesPipeline.reset();
	two4F32Two4U8AttributesPipeline.reset();
	twoPackedAttributesPipeline.reset();
	twoPackedBuffersPipeline.reset();
	twoPackedBuffersUsingStructPipeline.reset();
	twoPackedBuffersUsingStructSlowPipeline.reset();
	twoPackedAttributesAndSingleMatrixPipeline.reset();
	twoPackedAttributesAndMatrixPipeline.reset();
	twoPackedBuffersAndMatrixPipeline.reset();
	singlePackedBufferPipeline.reset();
	fourAttributesPipeline.reset();
	fourAttributesAndMatrixPipeline.reset();
	geometryShaderConstantOutputPipeline.reset();
	geometryShaderPipeline.reset();
	transformationThreeMatricesPipeline.reset();
	transformationFiveMatricesPipeline.reset();
	transformationFiveMatricesUsingGSPipeline.reset();
	transformationFiveMatricesUsingGSAndAttributesPipeline.reset();
	phongTexturedFourAttributesFiveMatricesPipeline.reset();
	phongTexturedFourAttributesPipeline.reset();
	phongTexturedPipeline.reset();
	phongTexturedRowMajorPipeline.reset();
	phongTexturedMat4x3Pipeline.reset();
	phongTexturedMat4x3RowMajorPipeline.reset();
	phongTexturedQuat1Pipeline.reset();
	phongTexturedQuat2Pipeline.reset();
	phongTexturedQuat3Pipeline.reset();
	phongTexturedQuat2PrimitiveRestartPipeline.reset();
	phongTexturedDMatricesOnlyInputPipeline.reset();
	phongTexturedDMatricesPipeline.reset();
	phongTexturedDMatricesDVerticesPipeline.reset();
	phongTexturedInGSDMatricesDVerticesPipeline.reset();
	phongTexturedSingleQuat2Pipeline.reset();
	phongTexturedSingleQuat2TriStripPipeline.reset();
	phongTexturedSingleQuat2PrimitiveRestartPipeline.reset();
	fillrateContantColorPipeline.reset();
	fillrateFourSmoothInterpolatorsPipeline.reset();
	fillrateFourFlatInterpolatorsPipeline.reset();
	fillrateTexturedPhongInterpolatorsPipeline.reset();
	fillrateTexturedPhongPipeline.reset();
	fillrateTexturedPhongNotPackedPipeline.reset();
	fillrateUniformColor4fPipeline.reset();
	fillrateUniformColor4bPipeline.reset();
	phongNoSpecularPipeline.reset();
	phongNoSpecularSingleUniformPipeline.reset();
	swapchainImageViews.clear();
	coordinateAttribute.reset();
	coordinateAttributeMemory.reset();
	coordinateBuffer.reset();
	coordinateBufferMemory.reset();
	normalAttribute.reset();
	normalAttributeMemory.reset();
	colorAttribute.reset();
	colorAttributeMemory.reset();
	texCoordAttribute.reset();
	texCoordAttributeMemory.reset();
	for(auto& a : vec4Attributes)         a.reset();
	for(auto& m : vec4AttributeMemory)    m.reset();
	for(auto& a : vec4u8Attributes)       a.reset();
	for(auto& m : vec4u8AttributeMemory)  m.reset();
	packedAttribute1.reset();
	packedAttribute1Memory.reset();
	packedAttribute2.reset();
	packedAttribute2Memory.reset();
	packedBuffer1.reset();
	packedBuffer1Memory.reset();
	packedBuffer2.reset();
	packedBuffer2Memory.reset();
	packedDAttribute1.reset();
	packedDAttribute1Memory.reset();
	packedDAttribute2.reset();
	packedDAttribute2Memory.reset();
	packedDAttribute3.reset();
	packedDAttribute3Memory.reset();
	indexBuffer.reset();
	indexBufferMemory.reset();
	primitiveRestartIndexBuffer.reset();
	primitiveRestartIndexBufferMemory.reset();
	stripIndexBuffer.reset();
	stripIndexBufferMemory.reset();
	stripPrimitiveRestartIndexBuffer.reset();
	stripPrimitiveRestartIndexBufferMemory.reset();
	stripPrimitiveRestart3IndexBuffer.reset();
	stripPrimitiveRestart3IndexBufferMemory.reset();
	stripPrimitiveRestart4IndexBuffer.reset();
	stripPrimitiveRestart4IndexBufferMemory.reset();
	stripPrimitiveRestart7IndexBuffer.reset();
	stripPrimitiveRestart7IndexBufferMemory.reset();
	stripPrimitiveRestart10IndexBuffer.reset();
	stripPrimitiveRestart10IndexBufferMemory.reset();
	primitiveRestartMinusOne2IndexBuffer.reset();
	primitiveRestartMinusOne2IndexBufferMemory.reset();
	primitiveRestartMinusOne5IndexBuffer.reset();
	primitiveRestartMinusOne5IndexBufferMemory.reset();
	minusOneIndexBuffer.reset();
	minusOneIndexBufferMemory.reset();
	zeroIndexBuffer.reset();
	zeroIndexBufferMemory.reset();
	plusOneIndexBuffer.reset();
	plusOneIndexBufferMemory.reset();
	stripPackedAttribute1.reset();
	stripPackedAttribute1Memory.reset();
	stripPackedAttribute2.reset();
	stripPackedAttribute2Memory.reset();
	sharedVertexPackedAttribute1.reset();
	sharedVertexPackedAttribute1Memory.reset();
	sharedVertexPackedAttribute2.reset();
	sharedVertexPackedAttribute2Memory.reset();
	sameVertexPackedAttribute1.reset();
	sameVertexPackedAttribute1Memory.reset();
	sameVertexPackedAttribute2.reset();
	sameVertexPackedAttribute2Memory.reset();
	singlePackedBuffer.reset();
	singlePackedBufferMemory.reset();
	singleMatrixUniformBuffer.reset();
	singleMatrixUniformMemory.reset();
	singlePATBuffer.reset();
	singlePATMemory.reset();
	sameMatrixAttribute.reset();
	sameMatrixAttributeMemory.reset();
	sameMatrixBuffer.reset();
	sameMatrixBufferMemory.reset();
	sameMatrixRowMajorBuffer.reset();
	sameMatrixRowMajorBufferMemory.reset();
	sameMatrix4x3Buffer.reset();
	sameMatrix4x3BufferMemory.reset();
	sameMatrix4x3RowMajorBuffer.reset();
	sameMatrix4x3RowMajorBufferMemory.reset();
	sameDMatrixBuffer.reset();
	sameDMatrixBufferMemory.reset();
	samePATBuffer.reset();
	samePATBufferMemory.reset();
	transformationMatrixAttribute.reset();
	transformationMatrixBuffer.reset();
	transformationMatrixAttributeMemory.reset();
	transformationMatrixBufferMemory.reset();
	normalMatrix4x3Buffer.reset();
	viewAndProjectionMatricesUniformBuffer.reset();
	materialUniformBuffer.reset();
	materialUniformBufferMemory.reset();
	materialNotPackedUniformBuffer.reset();
	materialNotPackedUniformBufferMemory.reset();
	globalLightUniformBuffer.reset();
	globalLightUniformBufferMemory.reset();
	lightUniformBuffer.reset();
	lightUniformBufferMemory.reset();
	lightNotPackedUniformBuffer.reset();
	lightNotPackedUniformBufferMemory.reset();
	allInOneLightingUniformBuffer.reset();
	allInOneLightingUniformBufferMemory.reset();
	descriptorPool.reset();
	timestampPool.reset();

	// submitNowCommandBuffer
	// that will be submitted at the end of this function
	vk::UniqueCommandPool commandPoolTransient=
		device->createCommandPoolUnique(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient|vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
				graphicsQueueFamily  // queueFamilyIndex
			)
		);
	vk::UniqueCommandBuffer submitNowCommandBuffer=std::move(
		device->allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(
				commandPoolTransient.get(),        // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1                                  // commandBufferCount
			)
		)[0]);
	submitNowCommandBuffer->begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);

	// create swapchain
	vk::SurfaceCapabilitiesKHR surfaceCapabilities=physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
	swapchain=
		device->createSwapchainKHRUnique(
			vk::SwapchainCreateInfoKHR(
				vk::SwapchainCreateFlagsKHR(),   // flags
				surface.get(),                   // surface
				surfaceCapabilities.maxImageCount==0  // minImageCount
					?surfaceCapabilities.minImageCount+1
					:min(surfaceCapabilities.maxImageCount,surfaceCapabilities.minImageCount+1),
				chosenSurfaceFormat.format,      // imageFormat
				chosenSurfaceFormat.colorSpace,  // imageColorSpace
				currentSurfaceExtent,  // imageExtent
				1,  // imageArrayLayers
				vk::ImageUsageFlagBits::eColorAttachment,  // imageUsage
				(graphicsQueueFamily==presentationQueueFamily)?vk::SharingMode::eExclusive:vk::SharingMode::eConcurrent, // imageSharingMode
				(graphicsQueueFamily==presentationQueueFamily)?uint32_t(0):uint32_t(2),  // queueFamilyIndexCount
				(graphicsQueueFamily==presentationQueueFamily)?nullptr:array<uint32_t,2>{graphicsQueueFamily,presentationQueueFamily}.data(),  // pQueueFamilyIndices
				surfaceCapabilities.currentTransform,    // preTransform
				vk::CompositeAlphaFlagBitsKHR::eOpaque,  // compositeAlpha
				[](vector<vk::PresentModeKHR>&& modes){  // presentMode
						return find(modes.begin(),modes.end(),vk::PresentModeKHR::eMailbox)!=modes.end()
							?vk::PresentModeKHR::eMailbox
							:vk::PresentModeKHR::eFifo; // fifo is guaranteed to be supported
					}(physicalDevice.getSurfacePresentModesKHR(surface.get())),
				VK_TRUE,         // clipped
				swapchain.get()  // oldSwapchain
			)
		);

	// swapchain images and image views
	vector<vk::Image> swapchainImages=device->getSwapchainImagesKHR(swapchain.get());
	swapchainImageViews.reserve(swapchainImages.size());
	for(vk::Image image:swapchainImages)
		swapchainImageViews.emplace_back(
			device->createImageViewUnique(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),  // flags
					image,                       // image
					vk::ImageViewType::e2D,      // viewType
					chosenSurfaceFormat.format,  // format
					vk::ComponentMapping(),      // components
					vk::ImageSubresourceRange(   // subresourceRange
						vk::ImageAspectFlagBits::eColor,  // aspectMask
						0,  // baseMipLevel
						1,  // levelCount
						0,  // baseArrayLayer
						1   // layerCount
					)
				)
			)
		);

	// depth image
	depthImage=
		device->createImageUnique(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				depthFormat,             // format
				vk::Extent3D(currentSurfaceExtent.width,currentSurfaceExtent.height,1),  // extent
				1,                       // mipLevels
				1,                       // arrayLayers
				vk::SampleCountFlagBits::e1,  // samples
				vk::ImageTiling::eOptimal,    // tiling
				vk::ImageUsageFlagBits::eDepthStencilAttachment,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr,                      // pQueueFamilyIndices
				vk::ImageLayout::eUndefined   // initialLayout
			)
		);

	// depth image memory
	depthImageMemory=allocateMemory(depthImage.get(),vk::MemoryPropertyFlagBits::eDeviceLocal);
	device->bindImageMemory(
		depthImage.get(),  // image
		depthImageMemory.get(),  // memory
		0  // memoryOffset
	);

	// depth image view
	depthImageView=
		device->createImageViewUnique(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),  // flags
				depthImage.get(),            // image
				vk::ImageViewType::e2D,      // viewType
				depthFormat,                 // format
				vk::ComponentMapping(),      // components
				vk::ImageSubresourceRange(   // subresourceRange
					vk::ImageAspectFlagBits::eDepth,  // aspectMask
					0,  // baseMipLevel
					1,  // levelCount
					0,  // baseArrayLayer
					1   // layerCount
				)
			)
		);

	// depth image layout
	submitNowCommandBuffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,  // srcStageMask
		vk::PipelineStageFlagBits::eEarlyFragmentTests,  // dstStageMask
		vk::DependencyFlags(),  // dependencyFlags
		0,nullptr,  // memoryBarrierCount,pMemoryBarriers
		0,nullptr,  // bufferMemoryBarrierCount,pBufferMemoryBarriers
		1,  // imageMemoryBarrierCount
		&(const vk::ImageMemoryBarrier&)vk::ImageMemoryBarrier(  // pImageMemoryBarriers
			vk::AccessFlags(),  // srcAccessMask
			vk::AccessFlagBits::eDepthStencilAttachmentRead|vk::AccessFlagBits::eDepthStencilAttachmentWrite,  // dstAccessMask
			vk::ImageLayout::eUndefined,  // oldLayout
			vk::ImageLayout::eDepthStencilAttachmentOptimal,  // newLayout
			VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
			VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
			depthImage.get(),  // image
			vk::ImageSubresourceRange(  // subresourceRange
				(depthFormat==vk::Format::eD32Sfloat)  // aspectMask
					?vk::ImageAspectFlagBits::eDepth
					:vk::ImageAspectFlagBits::eDepth|vk::ImageAspectFlagBits::eStencil,
				0,  // baseMipLevel
				1,  // levelCount
				0,  // baseArrayLayer
				1   // layerCount
			)
		)
	);

	// pipeline
	auto createPipeline=
		[](vk::ShaderModule vsModule,vk::ShaderModule fsModule,vk::PipelineLayout pipelineLayout,
		   const vk::Extent2D currentSurfaceExtent,
		   const vk::PipelineVertexInputStateCreateInfo* vertexInputState=nullptr,
		   vk::ShaderModule gsModule=nullptr,
		   const vk::PipelineInputAssemblyStateCreateInfo* inputAssemblyState=nullptr)
		   ->vk::UniquePipeline
		{
			return device->createGraphicsPipelineUnique(
				pipelineCache.get(),
				vk::GraphicsPipelineCreateInfo(
					vk::PipelineCreateFlags(),  // flags
					!gsModule?2:3,  // stageCount
					array<const vk::PipelineShaderStageCreateInfo,3>{  // pStages
						vk::PipelineShaderStageCreateInfo{
							vk::PipelineShaderStageCreateFlags(),  // flags
							vk::ShaderStageFlagBits::eVertex,      // stage
							vsModule,  // module
							"main",    // pName
							nullptr    // pSpecializationInfo
						},
						vk::PipelineShaderStageCreateInfo{
							vk::PipelineShaderStageCreateFlags(),  // flags
							vk::ShaderStageFlagBits::eFragment,    // stage
							fsModule,  // module
							"main",    // pName
							nullptr    // pSpecializationInfo
						},
						vk::PipelineShaderStageCreateInfo{
							vk::PipelineShaderStageCreateFlags(),  // flags
							vk::ShaderStageFlagBits::eGeometry,    // stage
							gsModule,  // module
							"main",    // pName
							nullptr    // pSpecializationInfo
						}
					}.data(),
					vertexInputState!=nullptr  // pVertexInputState
						?vertexInputState
						:&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
							vk::PipelineVertexInputStateCreateFlags(),  // flags
							1,        // vertexBindingDescriptionCount
							array<const vk::VertexInputBindingDescription,1>{  // pVertexBindingDescriptions
								vk::VertexInputBindingDescription(
									0,  // binding
									4*sizeof(float),  // stride
									vk::VertexInputRate::eVertex  // inputRate
								),
							}.data(),
							1,        // vertexAttributeDescriptionCount
							array<const vk::VertexInputAttributeDescription,1>{  // pVertexAttributeDescriptions
								vk::VertexInputAttributeDescription(
									0,  // location
									0,  // binding
									vk::Format::eR32G32B32A32Sfloat,  // format
									0   // offset
								),
							}.data()
						},
					inputAssemblyState!=nullptr  // pInputAssemblyState
						?inputAssemblyState
						:&(const vk::PipelineInputAssemblyStateCreateInfo&)vk::PipelineInputAssemblyStateCreateInfo{
							vk::PipelineInputAssemblyStateCreateFlags(),  // flags
							vk::PrimitiveTopology::eTriangleList,  // topology
							VK_FALSE  // primitiveRestartEnable
						},
					nullptr, // pTessellationState
					&(const vk::PipelineViewportStateCreateInfo&)vk::PipelineViewportStateCreateInfo{  // pViewportState
						vk::PipelineViewportStateCreateFlags(),  // flags
						1,  // viewportCount
						&(const vk::Viewport&)vk::Viewport(0.f,0.f,float(currentSurfaceExtent.width),float(currentSurfaceExtent.height),0.f,1.f),  // pViewports
						1,  // scissorCount
						&(const vk::Rect2D&)vk::Rect2D(vk::Offset2D(0,0),currentSurfaceExtent)  // pScissors
					},
					&(const vk::PipelineRasterizationStateCreateInfo&)vk::PipelineRasterizationStateCreateInfo{  // pRasterizationState
						vk::PipelineRasterizationStateCreateFlags(),  // flags
						VK_FALSE,  // depthClampEnable
						VK_FALSE,  // rasterizerDiscardEnable
						vk::PolygonMode::eFill,  // polygonMode
						vk::CullModeFlagBits::eNone,  // cullMode
						vk::FrontFace::eCounterClockwise,  // frontFace
						VK_FALSE,  // depthBiasEnable
						0.f,  // depthBiasConstantFactor
						0.f,  // depthBiasClamp
						0.f,  // depthBiasSlopeFactor
						1.f   // lineWidth
					},
					&(const vk::PipelineMultisampleStateCreateInfo&)vk::PipelineMultisampleStateCreateInfo{  // pMultisampleState
						vk::PipelineMultisampleStateCreateFlags(),  // flags
						vk::SampleCountFlagBits::e1,  // rasterizationSamples
						VK_FALSE,  // sampleShadingEnable
						0.f,       // minSampleShading
						nullptr,   // pSampleMask
						VK_FALSE,  // alphaToCoverageEnable
						VK_FALSE   // alphaToOneEnable
					},
					&(const vk::PipelineDepthStencilStateCreateInfo&)vk::PipelineDepthStencilStateCreateInfo{  // pDepthStencilState
						vk::PipelineDepthStencilStateCreateFlags(),  // flags
						VK_TRUE,  // depthTestEnable
						VK_TRUE,  // depthWriteEnable
						vk::CompareOp::eLess,  // depthCompareOp
						VK_FALSE,  // depthBoundsTestEnable
						VK_FALSE,  // stencilTestEnable
						vk::StencilOpState(),  // front
						vk::StencilOpState(),  // back
						0.f,  // minDepthBounds
						0.f   // maxDepthBounds
					},
					&(const vk::PipelineColorBlendStateCreateInfo&)vk::PipelineColorBlendStateCreateInfo{  // pColorBlendState
						vk::PipelineColorBlendStateCreateFlags(),  // flags
						VK_FALSE,  // logicOpEnable
						vk::LogicOp::eClear,  // logicOp
						1,  // attachmentCount
						&(const vk::PipelineColorBlendAttachmentState&)vk::PipelineColorBlendAttachmentState{  // pAttachments
							VK_FALSE,  // blendEnable
							vk::BlendFactor::eZero,  // srcColorBlendFactor
							vk::BlendFactor::eZero,  // dstColorBlendFactor
							vk::BlendOp::eAdd,       // colorBlendOp
							vk::BlendFactor::eZero,  // srcAlphaBlendFactor
							vk::BlendFactor::eZero,  // dstAlphaBlendFactor
							vk::BlendOp::eAdd,       // alphaBlendOp
							vk::ColorComponentFlagBits::eR|vk::ColorComponentFlagBits::eG|
								vk::ColorComponentFlagBits::eB|vk::ColorComponentFlagBits::eA  // colorWriteMask
						},
						array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
					},
					nullptr,  // pDynamicState
					pipelineLayout,  // layout
					renderPass.get(),  // renderPass
					0,  // subpass
					vk::Pipeline(nullptr),  // basePipelineHandle
					-1 // basePipelineIndex
				)
			).value;
		};

	const array<const vk::VertexInputBindingDescription,4> stride16AttributesBinding{
		vk::VertexInputBindingDescription(
			0,   // binding
			16,  // stride
			vk::VertexInputRate::eVertex  // inputRate
		),
		vk::VertexInputBindingDescription(
			1,   // binding
			16,  // stride
			vk::VertexInputRate::eVertex  // inputRate
		),
		vk::VertexInputBindingDescription(
			2,   // binding
			16,  // stride
			vk::VertexInputRate::eVertex  // inputRate
		),
		vk::VertexInputBindingDescription(
			3,   // binding
			16,  // stride
			vk::VertexInputRate::eVertex  // inputRate
		),
	};
	const array<const vk::VertexInputAttributeDescription,4> fourAttributesDescription{
		vk::VertexInputAttributeDescription(
			0,  // location
			0,  // binding
			vk::Format::eR32G32B32A32Sfloat,  // format
			0   // offset
		),
		vk::VertexInputAttributeDescription(
			1,  // location
			1,  // binding
			vk::Format::eR32G32B32A32Sfloat,  // format
			0   // offset
		),
		vk::VertexInputAttributeDescription(
			2,  // location
			2,  // binding
			vk::Format::eR32G32B32A32Sfloat,  // format
			0   // offset
		),
		vk::VertexInputAttributeDescription(
			3,  // location
			3,  // binding
			vk::Format::eR32G32B32A32Sfloat,  // format
			0   // offset
		),
	};
	const vk::PipelineVertexInputStateCreateInfo fourAttributesInputState{
		vk::PipelineVertexInputStateCreateFlags(),  // flags
		4,  // vertexBindingDescriptionCount
		stride16AttributesBinding.data(),  // pVertexBindingDescriptions
		4,  // vertexAttributeDescriptionCount
		fourAttributesDescription.data(),  // pVertexAttributeDescriptions
	};
	const array<const vk::VertexInputAttributeDescription,2> twoPackedAttributesDescription{
		vk::VertexInputAttributeDescription(
			0,  // location
			0,  // binding
			vk::Format::eR32G32B32A32Uint,  // format
			0   // offset
		),
		vk::VertexInputAttributeDescription(
			1,  // location
			1,  // binding
			vk::Format::eR32G32B32A32Uint,  // format
			0   // offset
		),
	};
	const vk::PipelineVertexInputStateCreateInfo twoPackedAttributesInputState{
		vk::PipelineVertexInputStateCreateFlags(),  // flags
		2,  // vertexBindingDescriptionCount
		stride16AttributesBinding.data(),  // pVertexBindingDescriptions
		2,  // vertexAttributeDescriptionCount
		twoPackedAttributesDescription.data()  // pVertexAttributeDescriptions
	};
	const vk::PipelineInputAssemblyStateCreateInfo triangleStripInputAssemblyState{
		vk::PipelineInputAssemblyStateCreateFlags(),  // flags
		vk::PrimitiveTopology::eTriangleStrip,  // topology
		VK_FALSE  // primitiveRestartEnable
	};

	attributelessConstantOutputPipeline=
		createPipeline(attributelessConstantOutputVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	attributelessInputIndicesPipeline=
		createPipeline(attributelessInputIndicesVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	coordinateAttributePipeline=
		createPipeline(coordinateAttributeVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent);
	coordinateBufferPipeline=
		createPipeline(coordinateBufferVS.get(),constantColorFS.get(),oneBufferPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	singleMatrixUniformPipeline=
		createPipeline(singleUniformMatrixVS.get(),constantColorFS.get(),oneUniformVSPipelineLayout.get(),currentSurfaceExtent);
	matrixAttributePipeline=
		createPipeline(matrixAttributeVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               2,  // vertexBindingDescriptionCount
			               array<const vk::VertexInputBindingDescription,2>{  // pVertexBindingDescriptions
				               vk::VertexInputBindingDescription(
					               0,  // binding
					               4*sizeof(float),  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               1,  // binding
					               16*sizeof(float),  // stride
					               vk::VertexInputRate::eInstance  // inputRate
				               ),
			               }.data(),
			               5,  // vertexAttributeDescriptionCount
			               array<const vk::VertexInputAttributeDescription,5>{  // pVertexAttributeDescriptions
				               vk::VertexInputAttributeDescription(
					               0,  // location
					               0,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               1,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               2,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               4*sizeof(float)  // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               3,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               8*sizeof(float)  // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               4,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               12*sizeof(float)  // offset
				               ),
			               }.data()
		               });
	matrixBufferPipeline=
		createPipeline(matrixBufferVS.get(),constantColorFS.get(),oneBufferPipelineLayout.get(),currentSurfaceExtent);
	twoAttributesPipeline=
		createPipeline(twoAttributesVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               2,  // vertexBindingDescriptionCount
			               stride16AttributesBinding.data(),  // pVertexBindingDescriptions
			               2,  // vertexAttributeDescriptionCount
			               fourAttributesDescription.data(),  // pVertexAttributeDescriptions
		               });
	twoPackedAttributesPipeline=
		createPipeline(twoPackedAttributesVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               2,  // vertexBindingDescriptionCount
			               stride16AttributesBinding.data(),  // pVertexBindingDescriptions
			               2,  // vertexAttributeDescriptionCount
			               array<const vk::VertexInputAttributeDescription,2>{  // pVertexAttributeDescriptions
				               vk::VertexInputAttributeDescription(
					               0,  // location
					               0,  // binding
					               vk::Format::eR32G32B32A32Uint,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               1,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Uint,  // format
					               0   // offset
				               ),
			               }.data()
		               });
	twoPackedBuffersPipeline=
		createPipeline(twoPackedBuffersVS.get(),constantColorFS.get(),twoBuffersPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	twoPackedBuffersUsingStructPipeline=
		createPipeline(twoPackedBuffersUsingStructVS.get(),constantColorFS.get(),twoBuffersPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	twoPackedBuffersUsingStructSlowPipeline=
		createPipeline(twoPackedBuffersUsingStructSlowVS.get(),constantColorFS.get(),twoBuffersPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	singlePackedBufferPipeline=
		createPipeline(singlePackedBufferVS.get(),constantColorFS.get(),oneBufferPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	two4F32Two4U8AttributesPipeline=
		createPipeline(fourAttributesVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               4,  // vertexBindingDescriptionCount
			               array<const vk::VertexInputBindingDescription,4>{  // pVertexBindingDescriptions
				               vk::VertexInputBindingDescription(
					               0,  // binding
					               4*sizeof(float),  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               1,  // binding
					               4*sizeof(float),  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               2,  // binding
					               4,  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               3,  // binding
					               4,  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
			               }.data(),
			               4,  // vertexAttributeDescriptionCount
			               array<const vk::VertexInputAttributeDescription,4>{  // pVertexAttributeDescriptions
				               vk::VertexInputAttributeDescription(
					               0,  // location
					               0,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               1,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               2,  // location
					               2,  // binding
					               vk::Format::eR8G8B8A8Unorm,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               3,  // location
					               3,  // binding
					               vk::Format::eR8G8B8A8Unorm,  // format
					               0   // offset
				               ),
			               }.data()
		               });
	twoPackedAttributesAndSingleMatrixPipeline=
		createPipeline(twoPackedAttributesAndSingleMatrixVS.get(),constantColorFS.get(),oneBufferPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState);
	twoPackedAttributesAndMatrixPipeline=
		createPipeline(twoPackedAttributesAndMatrixVS.get(),constantColorFS.get(),oneBufferPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               2,  // vertexBindingDescriptionCount
			               stride16AttributesBinding.data(),  // pVertexBindingDescriptions
			               2,  // vertexAttributeDescriptionCount
			               array<const vk::VertexInputAttributeDescription,2>{  // pVertexAttributeDescriptions
				               vk::VertexInputAttributeDescription(
					               0,  // location
					               0,  // binding
					               vk::Format::eR32G32B32A32Uint,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               1,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Uint,  // format
					               0   // offset
				               ),
			               }.data()
		               });
	twoPackedBuffersAndMatrixPipeline=
		createPipeline(twoPackedBuffersAndMatrixVS.get(),constantColorFS.get(),threeBuffersPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               });
	fourAttributesPipeline=
		createPipeline(fourAttributesVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &fourAttributesInputState);
	fourAttributesAndMatrixPipeline=
		createPipeline(fourAttributesAndMatrixVS.get(),constantColorFS.get(),oneBufferPipelineLayout.get(),currentSurfaceExtent,
		               &fourAttributesInputState);
	if(enabledFeatures.geometryShader)
		geometryShaderPipeline=
			createPipeline(geometryShaderVS.get(),constantColorFS.get(),threeBuffersInGSPipelineLayout.get(),currentSurfaceExtent,
			               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
				               vk::PipelineVertexInputStateCreateFlags(),  // flags
				               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
				               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
			               },
			               geometryShaderGS.get());
	if(enabledFeatures.geometryShader)
		geometryShaderConstantOutputPipeline=
			createPipeline(geometryShaderConstantOutputVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
			               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
				               vk::PipelineVertexInputStateCreateFlags(),  // flags
				               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
				               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
			               },
			               geometryShaderConstantOutputGS.get());
	transformationThreeMatricesPipeline=
		createPipeline(transformationThreeMatricesVS.get(),constantColorFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               2,  // vertexBindingDescriptionCount
			               stride16AttributesBinding.data(),  // pVertexBindingDescriptions
			               2,  // vertexAttributeDescriptionCount
			               array<const vk::VertexInputAttributeDescription,2>{  // pVertexAttributeDescriptions
				               vk::VertexInputAttributeDescription(
					               0,  // location
					               0,  // binding
					               vk::Format::eR32G32B32A32Uint,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               1,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Uint,  // format
					               0   // offset
				               ),
			               }.data()
		               });
	transformationFiveMatricesPipeline=
		createPipeline(transformationFiveMatricesVS.get(),constantColorFS.get(),twoBuffersAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               2,  // vertexBindingDescriptionCount
			               stride16AttributesBinding.data(),  // pVertexBindingDescriptions
			               2,  // vertexAttributeDescriptionCount
			               array<const vk::VertexInputAttributeDescription,2>{  // pVertexAttributeDescriptions
				               vk::VertexInputAttributeDescription(
					               0,  // location
					               0,  // binding
					               vk::Format::eR32G32B32A32Uint,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               1,  // location
					               1,  // binding
					               vk::Format::eR32G32B32A32Uint,  // format
					               0   // offset
				               ),
			               }.data()
		               });
	if(enabledFeatures.geometryShader)
		transformationFiveMatricesUsingGSPipeline=
			createPipeline(geometryShaderVS.get(),constantColorFS.get(),fourBuffersAndUniformInGSPipelineLayout.get(),currentSurfaceExtent,
			               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
				               vk::PipelineVertexInputStateCreateFlags(),  // flags
				               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
				               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
			               },
			               transformationFiveMatricesUsingGS.get());
	if(enabledFeatures.geometryShader)
		transformationFiveMatricesUsingGSAndAttributesPipeline=
			createPipeline(transformationFiveMatricesUsingGSAndAttributesVS.get(),constantColorFS.get(),twoBuffersAndUniformInGSPipelineLayout.get(),currentSurfaceExtent,
			               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
				               vk::PipelineVertexInputStateCreateFlags(),  // flags
				               2,  // vertexBindingDescriptionCount
				               stride16AttributesBinding.data(),  // pVertexBindingDescriptions
				               2,  // vertexAttributeDescriptionCount
				               array<const vk::VertexInputAttributeDescription,2>{  // pVertexAttributeDescriptions
					               vk::VertexInputAttributeDescription(
						               0,  // location
						               0,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
					               vk::VertexInputAttributeDescription(
						               1,  // location
						               1,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
				               }.data()
			               },
			               transformationFiveMatricesUsingGSAndAttributesGS.get());
	phongTexturedFourAttributesFiveMatricesPipeline=
		createPipeline(phongTexturedFourAttributesFiveMatricesVS.get(),phongTexturedDummyFS.get(),twoBuffersAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               4,  // vertexBindingDescriptionCount
			               array<const vk::VertexInputBindingDescription,4>{  // pVertexBindingDescriptions
				               vk::VertexInputBindingDescription(
					               0,  // binding
					               4*sizeof(float),  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               1,  // binding
					               3*sizeof(float),  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               2,  // binding
					               4,  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               3,  // binding
					               2*sizeof(float),  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
			               }.data(),
			               4,  // vertexAttributeDescriptionCount
			               array<const vk::VertexInputAttributeDescription,4>{  // pVertexAttributeDescriptions
				               vk::VertexInputAttributeDescription(
					               0,  // location
					               0,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               1,  // location
					               1,  // binding
					               vk::Format::eR32G32B32Sfloat,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               2,  // location
					               2,  // binding
					               vk::Format::eR8G8B8A8Unorm,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               3,  // location
					               3,  // binding
					               vk::Format::eR32G32Sfloat,  // format
					               0   // offset
				               ),
			               }.data()
		               });
	phongTexturedFourAttributesPipeline=
		createPipeline(phongTexturedFourAttributesVS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               4,  // vertexBindingDescriptionCount
			               array<const vk::VertexInputBindingDescription,4>{  // pVertexBindingDescriptions
				               vk::VertexInputBindingDescription(
					               0,  // binding
					               4*sizeof(float),  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               1,  // binding
					               3*sizeof(float),  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               2,  // binding
					               4,  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
				               vk::VertexInputBindingDescription(
					               3,  // binding
					               2*sizeof(float),  // stride
					               vk::VertexInputRate::eVertex  // inputRate
				               ),
			               }.data(),
			               4,  // vertexAttributeDescriptionCount
			               array<const vk::VertexInputAttributeDescription,4>{  // pVertexAttributeDescriptions
				               vk::VertexInputAttributeDescription(
					               0,  // location
					               0,  // binding
					               vk::Format::eR32G32B32A32Sfloat,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               1,  // location
					               1,  // binding
					               vk::Format::eR32G32B32Sfloat,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               2,  // location
					               2,  // binding
					               vk::Format::eR8G8B8A8Unorm,  // format
					               0   // offset
				               ),
				               vk::VertexInputAttributeDescription(
					               3,  // location
					               3,  // binding
					               vk::Format::eR32G32Sfloat,  // format
					               0   // offset
				               ),
			               }.data()
		               });
	phongTexturedPipeline=
		createPipeline(phongTexturedVS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState);
	phongTexturedRowMajorPipeline=
		createPipeline(phongTexturedRowMajorVS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState);
	phongTexturedMat4x3Pipeline=
		createPipeline(phongTexturedMat4x3VS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState);
	phongTexturedMat4x3RowMajorPipeline=
		createPipeline(phongTexturedMat4x3RowMajorVS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState);
	phongTexturedQuat1Pipeline=
		createPipeline(phongTexturedQuat1VS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState);
	phongTexturedQuat2Pipeline=
		createPipeline(phongTexturedQuat2VS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState);
	phongTexturedQuat3Pipeline=
		createPipeline(phongTexturedQuat3VS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState);
	phongTexturedQuat2PrimitiveRestartPipeline=
		createPipeline(phongTexturedQuat2PrimitiveRestartVS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState,
		               nullptr,
		               &(const vk::PipelineInputAssemblyStateCreateInfo&)vk::PipelineInputAssemblyStateCreateInfo{
			               vk::PipelineInputAssemblyStateCreateFlags(),  // flags
			               vk::PrimitiveTopology::eTriangleStrip,  // topology
			               VK_TRUE  // primitiveRestartEnable
		               });
	phongTexturedSingleQuat2Pipeline=
		createPipeline(phongTexturedSingleQuat2VS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState,
		               nullptr);
	phongTexturedSingleQuat2TriStripPipeline=
		createPipeline(phongTexturedSingleQuat2VS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState,
		               nullptr,
		               &triangleStripInputAssemblyState);
	phongTexturedSingleQuat2PrimitiveRestartPipeline=
		createPipeline(phongTexturedSingleQuat2VS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
		               &twoPackedAttributesInputState,
		               nullptr,
		               &(const vk::PipelineInputAssemblyStateCreateInfo&)vk::PipelineInputAssemblyStateCreateInfo{
			               vk::PipelineInputAssemblyStateCreateFlags(),  // flags
			               vk::PrimitiveTopology::eTriangleStrip,  // topology
			               VK_TRUE  // primitiveRestartEnable
		               });
	if(enabledFeatures.shaderFloat64)
		phongTexturedDMatricesOnlyInputPipeline=
			createPipeline(phongTexturedDMatricesOnlyInputVS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
			               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
				               vk::PipelineVertexInputStateCreateFlags(),  // flags
				               2,  // vertexBindingDescriptionCount
				               stride16AttributesBinding.data(),  // pVertexBindingDescriptions
				               2,  // vertexAttributeDescriptionCount
				               array<const vk::VertexInputAttributeDescription,2>{  // pVertexAttributeDescriptions
					               vk::VertexInputAttributeDescription(
						               0,  // location
						               0,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
					               vk::VertexInputAttributeDescription(
						               1,  // location
						               1,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
				               }.data()
			               });
	if(enabledFeatures.shaderFloat64)
		phongTexturedDMatricesPipeline=
			createPipeline(phongTexturedDMatricesVS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
			               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
				               vk::PipelineVertexInputStateCreateFlags(),  // flags
				               2,  // vertexBindingDescriptionCount
				               stride16AttributesBinding.data(),  // pVertexBindingDescriptions
				               2,  // vertexAttributeDescriptionCount
				               array<const vk::VertexInputAttributeDescription,2>{  // pVertexAttributeDescriptions
					               vk::VertexInputAttributeDescription(
						               0,  // location
						               0,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
					               vk::VertexInputAttributeDescription(
						               1,  // location
						               1,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
				               }.data()
			               });
	if(enabledFeatures.shaderFloat64)
		phongTexturedDMatricesDVerticesPipeline=
			createPipeline(phongTexturedDMatricesDVerticesVS.get(),phongTexturedDummyFS.get(),bufferAndUniformPipelineLayout.get(),currentSurfaceExtent,
			               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
				               vk::PipelineVertexInputStateCreateFlags(),  // flags
				               3,  // vertexBindingDescriptionCount
				               stride16AttributesBinding.data(),  // pVertexBindingDescriptions
				               3,  // vertexAttributeDescriptionCount
				               array<const vk::VertexInputAttributeDescription,3>{  // pVertexAttributeDescriptions
					               vk::VertexInputAttributeDescription(
						               0,  // location
						               0,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
					               vk::VertexInputAttributeDescription(
						               1,  // location
						               1,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
					               vk::VertexInputAttributeDescription(
						               2,  // location
						               2,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
				               }.data()
			               });
	if(enabledFeatures.shaderFloat64 && enabledFeatures.geometryShader)
		phongTexturedInGSDMatricesDVerticesPipeline=
			createPipeline(phongTexturedInGSDMatricesDVerticesVS.get(),phongTexturedDummyFS.get(),bufferAndUniformInGSPipelineLayout.get(),currentSurfaceExtent,
			               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
				               vk::PipelineVertexInputStateCreateFlags(),  // flags
				               3,  // vertexBindingDescriptionCount
				               stride16AttributesBinding.data(),  // pVertexBindingDescriptions
				               3,  // vertexAttributeDescriptionCount
				               array<const vk::VertexInputAttributeDescription,3>{  // pVertexAttributeDescriptions
					               vk::VertexInputAttributeDescription(
						               0,  // location
						               0,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
					               vk::VertexInputAttributeDescription(
						               1,  // location
						               1,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
					               vk::VertexInputAttributeDescription(
						               2,  // location
						               2,  // binding
						               vk::Format::eR32G32B32A32Uint,  // format
						               0   // offset
					               ),
				               }.data()
			               },
			               phongTexturedInGSDMatricesDVerticesGS.get());
	fillrateContantColorPipeline=
		createPipeline(fullscreenQuadVS.get(),constantColorFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               },
		               nullptr,
		               &triangleStripInputAssemblyState);
	fillrateFourSmoothInterpolatorsPipeline=
		createPipeline(fullscreenQuadFourInterpolatorsVS.get(),fullscreenQuadFourSmoothInterpolatorsFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               },
		               nullptr,
		               &triangleStripInputAssemblyState);
	fillrateFourFlatInterpolatorsPipeline=
		createPipeline(fullscreenQuadFourInterpolatorsVS.get(),fullscreenQuadFourFlatInterpolatorsFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               },
		               nullptr,
		               &triangleStripInputAssemblyState);
	fillrateTexturedPhongInterpolatorsPipeline=
		createPipeline(fullscreenQuadTexturedPhongInterpolatorsVS.get(),fullscreenQuadTexturedPhongInterpolatorsFS.get(),simplePipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               },
		               nullptr,
		               &triangleStripInputAssemblyState);
	fillrateTexturedPhongPipeline=
		createPipeline(fullscreenQuadTexturedPhongInterpolatorsVS.get(),phongTexturedFS.get(),phongTexturedPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               },
		               nullptr,
		               &triangleStripInputAssemblyState);
	fillrateTexturedPhongNotPackedPipeline=
		createPipeline(fullscreenQuadTexturedPhongInterpolatorsVS.get(),phongTexturedNotPackedFS.get(),phongTexturedPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               },
		               nullptr,
		               &triangleStripInputAssemblyState);
	fillrateUniformColor4fPipeline=
		createPipeline(fullscreenQuadVS.get(),uniformColor4fFS.get(),oneUniformFSPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               },
		               nullptr,
		               &triangleStripInputAssemblyState);
	fillrateUniformColor4bPipeline=
		createPipeline(fullscreenQuadVS.get(),uniformColor4bFS.get(),oneUniformFSPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               },
		               nullptr,
		               &triangleStripInputAssemblyState);
	phongNoSpecularPipeline=
		createPipeline(fullscreenQuadTwoVec3InterpolatorsVS.get(),phongNoSpecularFS.get(),threeUniformFSPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               },
		               nullptr,
		               &triangleStripInputAssemblyState);
	phongNoSpecularSingleUniformPipeline=
		createPipeline(fullscreenQuadTwoVec3InterpolatorsVS.get(),phongNoSpecularSingleUniformFS.get(),oneUniformFSPipelineLayout.get(),currentSurfaceExtent,
		               &(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
			               vk::PipelineVertexInputStateCreateFlags(),  // flags
			               0,nullptr,  // vertexBindingDescriptionCount,pVertexBindingDescriptions
			               0,nullptr   // vertexAttributeDescriptionCount,pVertexAttributeDescriptions
		               },
		               nullptr,
		               &triangleStripInputAssemblyState);

	// framebuffers
	framebuffers.reserve(swapchainImages.size());
	for(size_t i=0,c=swapchainImages.size(); i<c; i++)
		framebuffers.emplace_back(
			device->createFramebufferUnique(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),   // flags
					renderPass.get(),               // renderPass
					2,  // attachmentCount
					array<vk::ImageView,2>{  // pAttachments
						swapchainImageViews[i].get(),
						depthImageView.get()
					}.data(),
					currentSurfaceExtent.width,     // width
					currentSurfaceExtent.height,    // height
					1  // layers
				)
			)
		);

	// coordinate attributes and storage buffers
	struct BindInfo {
		bool sparseAllowed;
		vk::Buffer buffer;
		vk::DeviceMemory memory;
		size_t size;
		BindInfo(bool sparseAllowed_,vk::Buffer buffer_,vk::DeviceMemory memory_,size_t size_) : sparseAllowed(sparseAllowed_), buffer(buffer_), memory(memory_), size(size_)  {}
	};
	typedef vector<BindInfo> BindInfoList;
	BindInfoList bindInfoList;
	auto createBuffer=
		[](vk::UniqueBuffer& buffer,vk::UniqueDeviceMemory& memory,size_t size,bool sparseAllowed,vk::BufferUsageFlags usage,BindInfoList& bindInfoList) {
			buffer=
				device->createBufferUnique(
					vk::BufferCreateInfo(
						(sparseAllowed) ? bufferCreateFlags : vk::BufferCreateFlags(),  // flags
						(sparseAllowed) ? size*bufferSizeMultiplier : size,    // size
						usage,                        // usage
						vk::SharingMode::eExclusive,  // sharingMode
						0,                            // queueFamilyIndexCount
						nullptr                       // pQueueFamilyIndices
					)
				);
			size_t allocatedSize;
			tie(memory,allocatedSize)=allocateMemory(buffer.get(),vk::MemoryPropertyFlagBits::eDeviceLocal,bufferSizeMultiplier);
			bindInfoList.emplace_back(sparseAllowed,buffer.get(),memory.get(),allocatedSize);
		};
	auto startTime=chrono::high_resolution_clock::now();
	size_t coordinateBufferSize=getBufferSize(numTriangles,true); // ~48MB
	size_t normalBufferSize=size_t(numTriangles)*3*3*sizeof(float); // ~36MB
	size_t colorBufferSize=size_t(numTriangles)*3*4;
	size_t texCoordBufferSize=size_t(numTriangles)*3*2*sizeof(float);
	size_t vec4u8BufferSize=size_t(numTriangles)*3*4;
	size_t packedDataBufferSize=size_t(numTriangles)*3*16; // ~48MB
	size_t indexBufferSize=size_t(numTriangles)*3*4;
	size_t primitiveRestartIndexBufferSize=size_t(numTriangles)*4*4;
	size_t stripIndexBufferSize=getIndexBufferSize(numTriangles/triStripLength,triStripLength);
	size_t stripPrimitiveRestartIndexBufferSize=getStripIndexPrimitiveRestartBufferSize(numTriangles/triStripLength,triStripLength,triStripLength+2);
	size_t stripPrimitiveRestart3IndexBufferSize=getStripIndexPrimitiveRestartBufferSize(numTriangles/triStripLength,triStripLength,3);
	size_t stripPrimitiveRestart4IndexBufferSize=getStripIndexPrimitiveRestartBufferSize(numTriangles/triStripLength,triStripLength,4);
	size_t stripPrimitiveRestart7IndexBufferSize=getStripIndexPrimitiveRestartBufferSize(numTriangles/triStripLength,triStripLength,7);
	size_t stripPrimitiveRestart10IndexBufferSize=getStripIndexPrimitiveRestartBufferSize(numTriangles/triStripLength,triStripLength,10);
	size_t primitiveRestartMinusOne2IndexBufferSize=getPrimitiveRestartMinusOneBufferSize(numTriangles/triStripLength,triStripLength,2);
	size_t primitiveRestartMinusOne5IndexBufferSize=getPrimitiveRestartMinusOneBufferSize(numTriangles/triStripLength,triStripLength,5);
	size_t minusOneIndexBufferSize=size_t(numTriangles)*4*4;
	size_t zeroIndexBufferSize=size_t(numTriangles)*4*4;
	size_t plusOneIndexBufferSize=size_t(numTriangles)*4*4;
	size_t stripPackedDataBufferSize=getBufferSize(numTriangles/triStripLength,triStripLength,true);
	size_t sharedVertexPackedDataBufferSize=getBufferSizeForSharedVertexTriangles(numTriangles/triStripLength,triStripLength,true);
	size_t sameVertexPackedDataBufferSize=size_t(numTriangles)*3*16; // ~48MB
	createBuffer(coordinateAttribute,coordinateAttributeMemory,coordinateBufferSize,true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(coordinateBuffer,   coordinateBufferMemory,   coordinateBufferSize,true,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(normalAttribute,    normalAttributeMemory,    normalBufferSize,    true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(colorAttribute,     colorAttributeMemory,     colorBufferSize,     true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(texCoordAttribute,  texCoordAttributeMemory,  texCoordBufferSize,  true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	for(size_t i=0,c=vec4Attributes.size(); i<c; i++)
		createBuffer(vec4Attributes[i],vec4AttributeMemory[i],coordinateBufferSize, true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	for(size_t i=0,c=vec4u8Attributes.size(); i<c; i++)
		createBuffer(vec4u8Attributes[i],vec4u8AttributeMemory[i],vec4u8BufferSize, true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(packedAttribute1,  packedAttribute1Memory,  packedDataBufferSize,  true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(packedAttribute2,  packedAttribute2Memory,  packedDataBufferSize,  true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(packedBuffer1,     packedBuffer1Memory,     packedDataBufferSize,  true,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(packedBuffer2,     packedBuffer2Memory,     packedDataBufferSize,  true,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(singlePackedBuffer,singlePackedBufferMemory,packedDataBufferSize*2,true,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(packedDAttribute1, packedDAttribute1Memory, packedDataBufferSize,  true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(packedDAttribute2, packedDAttribute2Memory, packedDataBufferSize,  true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(packedDAttribute3, packedDAttribute3Memory, packedDataBufferSize,  true,vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(indexBuffer,       indexBufferMemory,       indexBufferSize,       true,vk::BufferUsageFlagBits::eIndexBuffer  |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(primitiveRestartIndexBuffer,         primitiveRestartIndexBufferMemory,         primitiveRestartIndexBufferSize,         true,vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(stripIndexBuffer,                    stripIndexBufferMemory,                    stripIndexBufferSize,                    true,vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(stripPrimitiveRestartIndexBuffer,    stripPrimitiveRestartIndexBufferMemory,    stripPrimitiveRestartIndexBufferSize,    true,vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(stripPrimitiveRestart3IndexBuffer,   stripPrimitiveRestart3IndexBufferMemory,   stripPrimitiveRestart3IndexBufferSize,   true,vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(stripPrimitiveRestart4IndexBuffer,   stripPrimitiveRestart4IndexBufferMemory,   stripPrimitiveRestart4IndexBufferSize,   true,vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(stripPrimitiveRestart7IndexBuffer,   stripPrimitiveRestart7IndexBufferMemory,   stripPrimitiveRestart7IndexBufferSize,   true,vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(stripPrimitiveRestart10IndexBuffer,  stripPrimitiveRestart10IndexBufferMemory,  stripPrimitiveRestart10IndexBufferSize,  true,vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(primitiveRestartMinusOne2IndexBuffer,primitiveRestartMinusOne2IndexBufferMemory,primitiveRestartMinusOne2IndexBufferSize,true,vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(primitiveRestartMinusOne5IndexBuffer,primitiveRestartMinusOne5IndexBufferMemory,primitiveRestartMinusOne5IndexBufferSize,true,vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(minusOneIndexBuffer,         minusOneIndexBufferMemory,         minusOneIndexBufferSize,         true,vk::BufferUsageFlagBits::eIndexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(zeroIndexBuffer,             zeroIndexBufferMemory,             zeroIndexBufferSize,             true,vk::BufferUsageFlagBits::eIndexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(plusOneIndexBuffer,          plusOneIndexBufferMemory,          plusOneIndexBufferSize,          true,vk::BufferUsageFlagBits::eIndexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(stripPackedAttribute1,       stripPackedAttribute1Memory,       stripPackedDataBufferSize,       true,vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(stripPackedAttribute2,       stripPackedAttribute2Memory,       stripPackedDataBufferSize,       true,vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(sharedVertexPackedAttribute1,sharedVertexPackedAttribute1Memory,sharedVertexPackedDataBufferSize,true,vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(sharedVertexPackedAttribute2,sharedVertexPackedAttribute2Memory,sharedVertexPackedDataBufferSize,true,vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(sameVertexPackedAttribute1,  sameVertexPackedAttribute1Memory,  sameVertexPackedDataBufferSize,  true,vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(sameVertexPackedAttribute2,  sameVertexPackedAttribute2Memory,  sameVertexPackedDataBufferSize,  true,vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	double totalMeasurementTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();
	cout<<"First buffer and memory set"<<endl;
	cout<<"   Creation time: "<<totalMeasurementTime*1000<<"ms"<<endl;

	if(sparseMode==SPARSE_NONE)
	{
		size_t totalMemorySize=0;
		auto startTime=chrono::high_resolution_clock::now();
		for(auto &bindInfo : bindInfoList) {
			device->bindBufferMemory(
				bindInfo.buffer,  // buffer
				bindInfo.memory,  // memory
				0  // memoryOffset
			);
			totalMemorySize+=bindInfo.size;
		}
		double totalMeasurementTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();
		cout<<"   Standard binding: "<<totalMeasurementTime*1000<<"ms"<<endl;
		cout<<"   (total amount of memory: "<<totalMemorySize/1024/1024<<"MiB, number of memory allocations: "<<bindInfoList.size()<<")"<<endl;
		bindInfoList.clear();
	}
	else
	{
		vector<vk::SparseBufferMemoryBindInfo> bufferBinds;
		vector<vk::SparseMemoryBind> memoryBinds;
		bufferBinds.reserve(bindInfoList.size());
		memoryBinds.reserve(bindInfoList.size());
		size_t numBlocks=0;
		size_t numMemObjs=0;
		for(auto &bindInfo : bindInfoList) {
			if(!bindInfo.sparseAllowed) {
				device->bindBufferMemory(
					bindInfo.buffer,  // buffer
					bindInfo.memory,  // memory
					0  // memoryOffset
				);
				continue;
			}
			auto& r=memoryBinds.emplace_back(
				0,  // resourceOffset
				bindInfo.size,  // size
				bindInfo.memory,  // memory
				0,  // memoryOffset
				vk::SparseMemoryBindFlags()  // flags
			);
			bufferBinds.emplace_back(
				bindInfo.buffer,  // buffer
				1,  // bindCount
				&r  // pBinds
			);
			numBlocks+=bindInfo.size/sparseBlockSize;
			numMemObjs++;
		}
		bindInfoList.clear();
		auto startTime=chrono::high_resolution_clock::now();
		sparseQueue.bindSparse(
			vk::BindSparseInfo(
				nullptr,  // waitSemaphores
				bufferBinds,  // bufferBinds
				nullptr,  // imageOpaqueBinds
				nullptr,  // imageBinds
				nullptr  // signalSemaphores
			),
			vk::Fence()
		);
		sparseQueue.waitIdle();
		double totalMeasurementTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();
		cout<<"   Sparse binding: "<<totalMeasurementTime*1000<<"ms\n"
		    <<"   Binding time of a memory block: "<<totalMeasurementTime/numBlocks*1e6<<"us"<<endl;
		cout<<"   (number of blocks: "<<numBlocks<<", number of memory objects: "<<numMemObjs<<")"<<endl;
	}

	// staging buffer struct
	struct StagingBuffer {
		vk::UniqueBuffer buffer;
		vk::UniqueDeviceMemory memory;
		void* ptr = nullptr;
		void* map()  { if(!ptr) ptr=device->mapMemory(memory.get(),0,VK_WHOLE_SIZE,vk::MemoryMapFlags()); return ptr; }
		void unmap()  { if(ptr){ device->unmapMemory(memory.get()); ptr=nullptr; } }
		void reset()  { buffer.reset(); memory.reset(); }
		~StagingBuffer() { unmap(); }
		StagingBuffer() = default;
		StagingBuffer(StagingBuffer&& s) = default;
		StagingBuffer(size_t bufferSize)
		{
			buffer=
				device->createBufferUnique(
					vk::BufferCreateInfo(
						vk::BufferCreateFlags(),      // flags
						bufferSize,                   // size
						vk::BufferUsageFlagBits::eTransferSrc,  // usage
						vk::SharingMode::eExclusive,  // sharingMode
						0,                            // queueFamilyIndexCount
						nullptr                       // pQueueFamilyIndices
					)
				);
			tie(memory,ignore)=
				allocateMemory(buffer.get(),
				               vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent|vk::MemoryPropertyFlagBits::eHostCached,1);
			device->bindBufferMemory(
				buffer.get(),  // buffer
				memory.get(),  // memory
				0  // memoryOffset
			);
		}
	};

	// attribute staging buffers
	StagingBuffer coordinateStagingBuffer(coordinateBufferSize);
	StagingBuffer normalStagingBuffer(normalBufferSize);
	StagingBuffer colorStagingBuffer(colorBufferSize);
	StagingBuffer texCoordStagingBuffer(texCoordBufferSize);
	StagingBuffer vec4Attribute1StagingBuffer(coordinateBufferSize);
	StagingBuffer vec4Attribute2StagingBuffer(coordinateBufferSize);
	StagingBuffer vec4u8AttributeStagingBuffer(vec4u8BufferSize);
	StagingBuffer packedAttribute1StagingBuffer(packedDataBufferSize);
	StagingBuffer packedAttribute2StagingBuffer(packedDataBufferSize);
	StagingBuffer singlePackedBufferStagingBuffer(packedDataBufferSize*2);
	StagingBuffer packedDAttribute1StagingBuffer(packedDataBufferSize);
	StagingBuffer packedDAttribute2StagingBuffer(packedDataBufferSize);
	StagingBuffer packedDAttribute3StagingBuffer(packedDataBufferSize);
	StagingBuffer indexStagingBuffer(indexBufferSize);
	StagingBuffer primitiveRestartIndexStagingBuffer(primitiveRestartIndexBufferSize);
	StagingBuffer stripIndexStagingBuffer(stripIndexBufferSize);
	StagingBuffer stripPrimitiveRestartIndexStagingBuffer(stripPrimitiveRestartIndexBufferSize);
	StagingBuffer stripPrimitiveRestart3IndexStagingBuffer(stripPrimitiveRestart3IndexBufferSize);
	StagingBuffer stripPrimitiveRestart4IndexStagingBuffer(stripPrimitiveRestart4IndexBufferSize);
	StagingBuffer stripPrimitiveRestart7IndexStagingBuffer(stripPrimitiveRestart7IndexBufferSize);
	StagingBuffer stripPrimitiveRestart10IndexStagingBuffer(stripPrimitiveRestart10IndexBufferSize);
	StagingBuffer primitiveRestartMinusOne2IndexStagingBuffer(primitiveRestartMinusOne2IndexBufferSize);
	StagingBuffer primitiveRestartMinusOne5IndexStagingBuffer(primitiveRestartMinusOne5IndexBufferSize);
	StagingBuffer minusOneIndexStagingBuffer(minusOneIndexBufferSize);
	StagingBuffer zeroIndexStagingBuffer(zeroIndexBufferSize);
	StagingBuffer plusOneIndexStagingBuffer(plusOneIndexBufferSize);
	StagingBuffer stripPackedAttribute1StagingBuffer(stripPackedDataBufferSize);
	StagingBuffer stripPackedAttribute2StagingBuffer(stripPackedDataBufferSize);
	StagingBuffer sharedVertexPackedAttribute1StagingBuffer(sharedVertexPackedDataBufferSize);
	StagingBuffer sharedVertexPackedAttribute2StagingBuffer(sharedVertexPackedDataBufferSize);
	StagingBuffer sameVertexPackedAttribute1StagingBuffer(sameVertexPackedDataBufferSize);
	StagingBuffer sameVertexPackedAttribute2StagingBuffer(sameVertexPackedDataBufferSize);
#if 0
	cout<<"First buffer set memory requirements: "<<(coordinateBufferSize+normalBufferSize+colorBufferSize+
			texCoordBufferSize+coordinateBufferSize+coordinateBufferSize+vec4u8BufferSize+packedDataBufferSize+
			packedDataBufferSize+packedDataBufferSize*2+packedDataBufferSize+packedDataBufferSize+
			packedDataBufferSize+indexBufferSize+primitiveRestartIndexBufferSize+stripIndexBufferSize+
			stripPrimitiveRestartIndexBufferSize+stripPrimitiveRestart3IndexBufferSize+
			stripPrimitiveRestart4IndexBufferSize+stripPrimitiveRestart7IndexBufferSize+
			stripPrimitiveRestart10IndexBufferSize+
			primitiveRestartMinusOne2IndexBufferSize+primitiveRestartMinusOne5IndexBufferSize+
			minusOneIndexBufferSize+zeroIndexBufferSize+plusOneIndexBufferSize+
			stripPackedDataBufferSize+stripPackedDataBufferSize+sharedVertexPackedDataBufferSize+
			sharedVertexPackedDataBufferSize+sameVertexPackedDataBufferSize+
			sameVertexPackedDataBufferSize)/1024/1024<<"MiB"<<endl;
#endif
	generateCoordinates(
		reinterpret_cast<float*>(coordinateStagingBuffer.map()),numTriangles,triangleSize,
		renderingExtent.width,renderingExtent.height,true,
		2./currentSurfaceExtent.width,2./currentSurfaceExtent.height,-1.,-1.);
	coordinateStagingBuffer.unmap();
	float* pfloat=reinterpret_cast<float*>(normalStagingBuffer.map());
	for(size_t i=0,c=size_t(numTriangles)*3*3; i<c; i++)
		pfloat[i]=1.f;
	normalStagingBuffer.unmap();
	uint32_t* puint=reinterpret_cast<uint32_t*>(colorStagingBuffer.map());
	for(size_t i=0,c=size_t(numTriangles)*3; i<c; i++)
		puint[i]=0x0055AAFF;
	colorStagingBuffer.unmap();
	pfloat=reinterpret_cast<float*>(texCoordStagingBuffer.map());
	for(size_t i=0,c=size_t(numTriangles)*3*2; i<c; ) {
		pfloat[i++]=0.f;
		pfloat[i++]=0.f;
		pfloat[i++]=1.f;
		pfloat[i++]=0.f;
		pfloat[i++]=0.f;
		pfloat[i++]=1.f;
	}
	texCoordStagingBuffer.unmap();
	pfloat=reinterpret_cast<float*>(vec4Attribute1StagingBuffer.map());
	for(size_t i=0,c=size_t(numTriangles)*3*4; i<c; i++)
		pfloat[i]=-2.f;
	vec4Attribute1StagingBuffer.unmap();
	pfloat=reinterpret_cast<float*>(vec4Attribute2StagingBuffer.map());
	for(size_t i=0,c=size_t(numTriangles)*3*4; i<c; i++)
		pfloat[i]=4.f;
	vec4Attribute2StagingBuffer.unmap();
	puint=reinterpret_cast<uint32_t*>(vec4u8AttributeStagingBuffer.map());
	for(size_t i=0,c=size_t(numTriangles)*3; i<c; i++)
		puint[i]=0xFFFFFFFF;
	vec4u8AttributeStagingBuffer.unmap();
	generateCoordinates(
		reinterpret_cast<float*>(packedAttribute1StagingBuffer.map()),numTriangles,triangleSize,
		renderingExtent.width,renderingExtent.height,true,
		2./currentSurfaceExtent.width,2./currentSurfaceExtent.height,-1.,-1.);
	for(size_t i=3,e=size_t(numTriangles)*3*4; i<e; i+=4)
		reinterpret_cast<uint32_t*>(packedAttribute1StagingBuffer.ptr)[i]=0x3c003c00; // two half-floats, both set to one
	puint=reinterpret_cast<uint32_t*>(packedAttribute2StagingBuffer.map());
	for(size_t i=0,e=size_t(numTriangles)*3*4; i<e; ) {
		puint[i++]=0x3f800000;  // texture U (float), one (1.f is 0x3f800000, 0.f is 0x00000000)
		puint[i++]=0x3f800000;  // texture V (float), one
		puint[i++]=0x3c003c00;  // normalX+Y (2x half), two times one (1.f in half-float is 0x3c00, 0.f is 0x0000)
		puint[i++]=0xffffffff;  // color
		puint[i++]=0x3f800000;  // texture U (float), one
		puint[i++]=0x3f800000;  // texture V (float), one
		puint[i++]=0x3c003c00;  // normalX+Y (2x half), two times one
		puint[i++]=0xffffffff;  // color
		puint[i++]=0x3f800000;  // texture U (float), one
		puint[i++]=0x3f800000;  // texture V (float), one
		puint[i++]=0x3c003c00;  // normalX+Y (2x half), two times one
		puint[i++]=0xffffffff;  // color
	}
	singlePackedBufferStagingBuffer.map();
	for(size_t i=0,e=size_t(numTriangles)*3*4; i<e; i+=4) {
		uint32_t* src1=&reinterpret_cast<uint32_t*>(packedAttribute1StagingBuffer.ptr)[i];
		uint32_t* src2=&reinterpret_cast<uint32_t*>(packedAttribute2StagingBuffer.ptr)[i];
		uint32_t* dest=&reinterpret_cast<uint32_t*>(singlePackedBufferStagingBuffer.ptr)[i*2];
		dest[0]=src1[0];  // posX
		dest[1]=src1[1];  // posY
		dest[2]=src1[2];  // posZ
		dest[3]=src2[3];  // packedColor
		dest[4]=src2[0];  // texCoord U
		dest[5]=src2[1];  // texCoord V
		dest[6]=src2[2];  // normalX+Y
		dest[7]=src1[3];  // normalZ+posW
	}
	packedDAttribute1StagingBuffer.map();
	packedDAttribute2StagingBuffer.map();
	packedDAttribute3StagingBuffer.map();
	for(size_t i=0,e=size_t(numTriangles)*3*2; i<e; i+=2) {
		reinterpret_cast<double*>(packedDAttribute1StagingBuffer.ptr)[i+0]=reinterpret_cast<float*>(packedAttribute1StagingBuffer.ptr)[i*2+0];
		reinterpret_cast<double*>(packedDAttribute1StagingBuffer.ptr)[i+1]=reinterpret_cast<float*>(packedAttribute1StagingBuffer.ptr)[i*2+1];
		reinterpret_cast<double*>(packedDAttribute2StagingBuffer.ptr)[i+0]=reinterpret_cast<float*>(packedAttribute1StagingBuffer.ptr)[i*2+2];
		uint32_t normalXY=reinterpret_cast<uint32_t*>(packedAttribute2StagingBuffer.ptr)[i*2+2];
		uint32_t normalZposW=reinterpret_cast<uint32_t*>(packedAttribute1StagingBuffer.ptr)[i*2+3];
		reinterpret_cast<uint64_t*>(packedDAttribute2StagingBuffer.ptr)[i+1]=normalXY+(uint64_t(normalZposW)<<32);
		uint32_t texU=reinterpret_cast<uint32_t*>(packedAttribute2StagingBuffer.ptr)[i*2+0];
		uint32_t texV=reinterpret_cast<uint32_t*>(packedAttribute2StagingBuffer.ptr)[i*2+1];
		reinterpret_cast<uint64_t*>(packedDAttribute3StagingBuffer.ptr)[i+0]=texU+(uint64_t(texV)<<32);
		reinterpret_cast<uint64_t*>(packedDAttribute3StagingBuffer.ptr)[i+1]=reinterpret_cast<uint32_t*>(packedAttribute2StagingBuffer.ptr)[i*2+3];
	}
	packedDAttribute1StagingBuffer.unmap();
	packedDAttribute2StagingBuffer.unmap();
	packedDAttribute3StagingBuffer.unmap();
	singlePackedBufferStagingBuffer.unmap();
	sameVertexPackedAttribute1StagingBuffer.map();
	for(size_t i=0,e=size_t(numTriangles)*3*16; i<e; i+=16)
		memcpy(&reinterpret_cast<char*>(sameVertexPackedAttribute1StagingBuffer.ptr)[i],packedAttribute1StagingBuffer.ptr,16);
	sameVertexPackedAttribute1StagingBuffer.unmap();
	sameVertexPackedAttribute2StagingBuffer.map();
	for(size_t i=0,e=size_t(numTriangles)*3*16; i<e; i+=16)
		memcpy(&reinterpret_cast<char*>(sameVertexPackedAttribute2StagingBuffer.ptr)[i],packedAttribute2StagingBuffer.ptr,16);
	sameVertexPackedAttribute2StagingBuffer.unmap();
	packedAttribute1StagingBuffer.unmap();
	packedAttribute2StagingBuffer.unmap();
	indexStagingBuffer.map();
	for(uint32_t i=0,e=uint32_t(numTriangles)*3; i<e; i++)
		reinterpret_cast<uint32_t*>(indexStagingBuffer.ptr)[i]=i;
	indexStagingBuffer.unmap();
	primitiveRestartIndexStagingBuffer.map();
	for(uint32_t i=0,j=0,e=uint32_t(numTriangles)*3; i<e;) {
		reinterpret_cast<uint32_t*>(primitiveRestartIndexStagingBuffer.ptr)[j++]=i++;
		reinterpret_cast<uint32_t*>(primitiveRestartIndexStagingBuffer.ptr)[j++]=i++;
		reinterpret_cast<uint32_t*>(primitiveRestartIndexStagingBuffer.ptr)[j++]=i++;
		reinterpret_cast<uint32_t*>(primitiveRestartIndexStagingBuffer.ptr)[j++]=-1;
	}
	primitiveRestartIndexStagingBuffer.unmap();
	stripIndexStagingBuffer.map();
	generateStripIndices(reinterpret_cast<uint32_t*>(stripIndexStagingBuffer.ptr),numTriangles/triStripLength,triStripLength);
	stripIndexStagingBuffer.unmap();
	stripPrimitiveRestartIndexStagingBuffer.map();
	generateStripPrimitiveRestartIndices(reinterpret_cast<uint32_t*>(stripPrimitiveRestartIndexStagingBuffer.ptr),numTriangles/triStripLength,triStripLength,triStripLength+2);
	stripPrimitiveRestartIndexStagingBuffer.unmap();
	stripPrimitiveRestart3IndexStagingBuffer.map();
	generateStripPrimitiveRestartIndices(reinterpret_cast<uint32_t*>(stripPrimitiveRestart3IndexStagingBuffer.ptr),numTriangles/triStripLength,triStripLength,3);
	stripPrimitiveRestart3IndexStagingBuffer.unmap();
	stripPrimitiveRestart4IndexStagingBuffer.map();
	generateStripPrimitiveRestartIndices(reinterpret_cast<uint32_t*>(stripPrimitiveRestart4IndexStagingBuffer.ptr),numTriangles/triStripLength,triStripLength,4);
	stripPrimitiveRestart4IndexStagingBuffer.unmap();
	stripPrimitiveRestart7IndexStagingBuffer.map();
	generateStripPrimitiveRestartIndices(reinterpret_cast<uint32_t*>(stripPrimitiveRestart7IndexStagingBuffer.ptr),numTriangles/triStripLength,triStripLength,7);
	stripPrimitiveRestart7IndexStagingBuffer.unmap();
	stripPrimitiveRestart10IndexStagingBuffer.map();
	generateStripPrimitiveRestartIndices(reinterpret_cast<uint32_t*>(stripPrimitiveRestart10IndexStagingBuffer.ptr),numTriangles/triStripLength,triStripLength,10);
	stripPrimitiveRestart10IndexStagingBuffer.unmap();
	primitiveRestartMinusOne2IndexStagingBuffer.map();
	generatePrimitiveRestartMinusOneIndices(reinterpret_cast<uint32_t*>(primitiveRestartMinusOne2IndexStagingBuffer.ptr),numTriangles/triStripLength,triStripLength,2);
	primitiveRestartMinusOne2IndexStagingBuffer.unmap();
	primitiveRestartMinusOne5IndexStagingBuffer.map();
	generatePrimitiveRestartMinusOneIndices(reinterpret_cast<uint32_t*>(primitiveRestartMinusOne5IndexStagingBuffer.ptr),numTriangles/triStripLength,triStripLength,5);
	primitiveRestartMinusOne5IndexStagingBuffer.unmap();
	minusOneIndexStagingBuffer.map();
	fill(reinterpret_cast<int32_t*>(minusOneIndexStagingBuffer.ptr),reinterpret_cast<int32_t*>(minusOneIndexStagingBuffer.ptr)+minusOneIndexBufferSize/4,int32_t(-1));
	minusOneIndexStagingBuffer.unmap();
	zeroIndexStagingBuffer.map();
	fill(reinterpret_cast<uint32_t*>(zeroIndexStagingBuffer.ptr),reinterpret_cast<uint32_t*>(zeroIndexStagingBuffer.ptr)+zeroIndexBufferSize/4,uint32_t(0));
	zeroIndexStagingBuffer.unmap();
	plusOneIndexStagingBuffer.map();
	fill(reinterpret_cast<uint32_t*>(plusOneIndexStagingBuffer.ptr),reinterpret_cast<uint32_t*>(plusOneIndexStagingBuffer.ptr)+plusOneIndexBufferSize/4,uint32_t(1));
	plusOneIndexStagingBuffer.unmap();
	generateStrips(
		reinterpret_cast<float*>(stripPackedAttribute1StagingBuffer.map()),numTriangles/triStripLength,
		triStripLength,triangleSize,renderingExtent.width,renderingExtent.height,true,
		2./currentSurfaceExtent.width,2./currentSurfaceExtent.height,-1.,-1.);
	for(size_t i=3,e=stripPackedDataBufferSize/4; i<e; i+=4)
		reinterpret_cast<uint32_t*>(stripPackedAttribute1StagingBuffer.ptr)[i]=0x3c003c00; // two half-floats, both set to one
	stripPackedAttribute1StagingBuffer.unmap();
	puint=reinterpret_cast<uint32_t*>(stripPackedAttribute2StagingBuffer.map());
	for(size_t i=0,e=stripPackedDataBufferSize/4; i<e; ) {
		puint[i++]=0x3f800000;  // texture U (float), one (1.f is 0x3f800000, 0.f is 0x00000000)
		puint[i++]=0x3f800000;  // texture V (float), one
		puint[i++]=0x3c003c00;  // normalX+Y (2x half), two times one (1.f in half-float is 0x3c00, 0.f is 0x0000)
		puint[i++]=0xffffffff;  // color
		puint[i++]=0x3f800000;  // texture U (float), one
		puint[i++]=0x3f800000;  // texture V (float), one
		puint[i++]=0x3c003c00;  // normalX+Y (2x half), two times one
		puint[i++]=0xffffffff;  // color
		puint[i++]=0x3f800000;  // texture U (float), one
		puint[i++]=0x3f800000;  // texture V (float), one
		puint[i++]=0x3c003c00;  // normalX+Y (2x half), two times one
		puint[i++]=0xffffffff;  // color
	}
	stripPackedAttribute2StagingBuffer.unmap();
	generateSharedVertexTriangles(
		reinterpret_cast<float*>(sharedVertexPackedAttribute1StagingBuffer.map()),numTriangles/triStripLength,
		triStripLength,triangleSize,renderingExtent.width,renderingExtent.height,true,
		2./currentSurfaceExtent.width,2./currentSurfaceExtent.height,-1.,-1.);
	for(size_t i=3,e=sharedVertexPackedDataBufferSize/4; i<e; i+=4)
		reinterpret_cast<uint32_t*>(sharedVertexPackedAttribute1StagingBuffer.ptr)[i]=0x3c003c00; // two half-floats, both set to one
	sharedVertexPackedAttribute1StagingBuffer.unmap();
	puint=reinterpret_cast<uint32_t*>(sharedVertexPackedAttribute2StagingBuffer.map());
	for(size_t i=0,e=sharedVertexPackedDataBufferSize/4; i<e; ) {
		puint[i++]=0x3f800000;  // texture U (float), one (1.f is 0x3f800000, 0.f is 0x00000000)
		puint[i++]=0x3f800000;  // texture V (float), one
		puint[i++]=0x3c003c00;  // normalX+Y (2x half), two times one (1.f in half-float is 0x3c00, 0.f is 0x0000)
		puint[i++]=0xffffffff;  // color
		puint[i++]=0x3f800000;  // texture U (float), one
		puint[i++]=0x3f800000;  // texture V (float), one
		puint[i++]=0x3c003c00;  // normalX+Y (2x half), two times one
		puint[i++]=0xffffffff;  // color
		puint[i++]=0x3f800000;  // texture U (float), one
		puint[i++]=0x3f800000;  // texture V (float), one
		puint[i++]=0x3c003c00;  // normalX+Y (2x half), two times one
		puint[i++]=0xffffffff;  // color
	}
	sharedVertexPackedAttribute2StagingBuffer.unmap();

	// copy data from staging to attribute and storage buffer
	submitNowCommandBuffer->copyBuffer(
		coordinateStagingBuffer.buffer.get(),  // srcBuffer
		coordinateAttribute.get(),             // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,coordinateBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		coordinateStagingBuffer.buffer.get(),  // srcBuffer
		coordinateBuffer.get(),                // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,coordinateBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		normalStagingBuffer.buffer.get(),  // srcBuffer
		normalAttribute.get(),             // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,normalBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		colorStagingBuffer.buffer.get(),  // srcBuffer
		colorAttribute.get(),             // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,colorBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		texCoordStagingBuffer.buffer.get(),  // srcBuffer
		texCoordAttribute.get(),             // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,texCoordBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		vec4Attribute1StagingBuffer.buffer.get(),  // srcBuffer
		vec4Attributes[0].get(),                   // dstBuffer
		1,                                         // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,coordinateBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		vec4Attribute1StagingBuffer.buffer.get(),  // srcBuffer
		vec4Attributes[1].get(),                   // dstBuffer
		1,                                         // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,coordinateBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		vec4Attribute2StagingBuffer.buffer.get(),  // srcBuffer
		vec4Attributes[2].get(),                   // dstBuffer
		1,                                         // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,coordinateBufferSize)  // pRegions
	);
	for(size_t i=0; i<vec4u8Attributes.size(); i++)
		submitNowCommandBuffer->copyBuffer(
			vec4u8AttributeStagingBuffer.buffer.get(),  // srcBuffer
			vec4u8Attributes[i].get(),                  // dstBuffer
			1,                                          // regionCount
			&(const vk::BufferCopy&)vk::BufferCopy(0,0,vec4u8BufferSize)  // pRegions
		);
	submitNowCommandBuffer->copyBuffer(
		packedAttribute1StagingBuffer.buffer.get(),  // srcBuffer
		packedAttribute1.get(),                      // dstBuffer
		1,                                           // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,packedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		packedAttribute2StagingBuffer.buffer.get(),  // srcBuffer
		packedAttribute2.get(),                      // dstBuffer
		1,                                           // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,packedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		packedAttribute1StagingBuffer.buffer.get(),  // srcBuffer
		packedBuffer1.get(),                         // dstBuffer
		1,                                           // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,packedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		packedAttribute2StagingBuffer.buffer.get(),  // srcBuffer
		packedBuffer2.get(),                         // dstBuffer
		1,                                           // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,packedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		singlePackedBufferStagingBuffer.buffer.get(),  // srcBuffer
		singlePackedBuffer.get(),                      // dstBuffer
		1,                                             // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,packedDataBufferSize*2)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		packedDAttribute1StagingBuffer.buffer.get(),  // srcBuffer
		packedDAttribute1.get(),                      // dstBuffer
		1,                                            // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,packedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		packedDAttribute2StagingBuffer.buffer.get(),  // srcBuffer
		packedDAttribute2.get(),                      // dstBuffer
		1,                                            // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,packedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		packedDAttribute3StagingBuffer.buffer.get(),  // srcBuffer
		packedDAttribute3.get(),                      // dstBuffer
		1,                                            // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,packedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		indexStagingBuffer.buffer.get(),  // srcBuffer
		indexBuffer.get(),                // dstBuffer
		1,                                // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,indexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		primitiveRestartIndexStagingBuffer.buffer.get(),  // srcBuffer
		primitiveRestartIndexBuffer.get(),                // dstBuffer
		1,                                                // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,primitiveRestartIndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		stripIndexStagingBuffer.buffer.get(),  // srcBuffer
		stripIndexBuffer.get(),                // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,stripIndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		stripPrimitiveRestartIndexStagingBuffer.buffer.get(),  // srcBuffer
		stripPrimitiveRestartIndexBuffer.get(),                // dstBuffer
		1,                                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,stripPrimitiveRestartIndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		stripPrimitiveRestart3IndexStagingBuffer.buffer.get(),  // srcBuffer
		stripPrimitiveRestart3IndexBuffer.get(),                // dstBuffer
		1,                                                      // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,stripPrimitiveRestart3IndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		stripPrimitiveRestart4IndexStagingBuffer.buffer.get(),  // srcBuffer
		stripPrimitiveRestart4IndexBuffer.get(),                // dstBuffer
		1,                                                      // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,stripPrimitiveRestart4IndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		stripPrimitiveRestart7IndexStagingBuffer.buffer.get(),  // srcBuffer
		stripPrimitiveRestart7IndexBuffer.get(),                // dstBuffer
		1,                                                      // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,stripPrimitiveRestart7IndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		stripPrimitiveRestart10IndexStagingBuffer.buffer.get(),  // srcBuffer
		stripPrimitiveRestart10IndexBuffer.get(),                // dstBuffer
		1,                                                       // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,stripPrimitiveRestart10IndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		primitiveRestartMinusOne2IndexStagingBuffer.buffer.get(),  // srcBuffer
		primitiveRestartMinusOne2IndexBuffer.get(),                // dstBuffer
		1,                                                         // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,primitiveRestartMinusOne2IndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		primitiveRestartMinusOne5IndexStagingBuffer.buffer.get(),  // srcBuffer
		primitiveRestartMinusOne5IndexBuffer.get(),                // dstBuffer
		1,                                                         // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,primitiveRestartMinusOne5IndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		minusOneIndexStagingBuffer.buffer.get(),  // srcBuffer
		minusOneIndexBuffer.get(),                // dstBuffer
		1,                                        // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,minusOneIndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		zeroIndexStagingBuffer.buffer.get(),  // srcBuffer
		zeroIndexBuffer.get(),                // dstBuffer
		1,                                    // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,zeroIndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		plusOneIndexStagingBuffer.buffer.get(),  // srcBuffer
		plusOneIndexBuffer.get(),                // dstBuffer
		1,                                       // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,plusOneIndexBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		stripPackedAttribute1StagingBuffer.buffer.get(),  // srcBuffer
		stripPackedAttribute1.get(),                      // dstBuffer
		1,                                                // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,stripPackedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		stripPackedAttribute2StagingBuffer.buffer.get(),  // srcBuffer
		stripPackedAttribute2.get(),                      // dstBuffer
		1,                                                // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,stripPackedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		sharedVertexPackedAttribute1StagingBuffer.buffer.get(),  // srcBuffer
		sharedVertexPackedAttribute1.get(),                      // dstBuffer
		1,                                                       // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,sharedVertexPackedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		sharedVertexPackedAttribute2StagingBuffer.buffer.get(),  // srcBuffer
		sharedVertexPackedAttribute2.get(),                      // dstBuffer
		1,                                                       // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,sharedVertexPackedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		sameVertexPackedAttribute1StagingBuffer.buffer.get(),  // srcBuffer
		sameVertexPackedAttribute1.get(),                      // dstBuffer
		1,                                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,sameVertexPackedDataBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		sameVertexPackedAttribute2StagingBuffer.buffer.get(),  // srcBuffer
		sameVertexPackedAttribute2.get(),                      // dstBuffer
		1,                                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,sameVertexPackedDataBufferSize)  // pRegions
	);

	// submit command buffer
	submitNowCommandBuffer->end();
	vk::UniqueFence fence(device->createFenceUnique(vk::FenceCreateInfo{vk::FenceCreateFlags()}));
	graphicsQueue.submit(
		vk::SubmitInfo(  // submits (vk::ArrayProxy)
			0,nullptr,nullptr,       // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
			1,&submitNowCommandBuffer.get(),  // commandBufferCount,pCommandBuffers
			0,nullptr                // signalSemaphoreCount,pSignalSemaphores
		),
		fence.get()  // fence
	);

	// wait for work to complete
	vk::Result r=device->waitForFences(
		fence.get(),   // fences (vk::ArrayProxy)
		VK_TRUE,       // waitAll
		uint64_t(3e9)  // timeout (3s)
	);
	if(r==vk::Result::eTimeout)
		throw std::runtime_error("GPU timeout. Task is probably hanging.");

	// release memory (to avoid gpu out-of-memory error)
	coordinateStagingBuffer.reset();
	normalStagingBuffer.reset();
	colorStagingBuffer.reset();
	texCoordStagingBuffer.reset();
	vec4Attribute1StagingBuffer.reset();
	vec4Attribute2StagingBuffer.reset();
	vec4u8AttributeStagingBuffer.reset();
	packedAttribute1StagingBuffer.reset();
	packedAttribute2StagingBuffer.reset();
	singlePackedBufferStagingBuffer.reset();
	packedDAttribute1StagingBuffer.reset();
	packedDAttribute2StagingBuffer.reset();
	packedDAttribute3StagingBuffer.reset();
	indexStagingBuffer.reset();
	primitiveRestartIndexStagingBuffer.reset();
	stripIndexStagingBuffer.reset();
	stripPrimitiveRestartIndexStagingBuffer.reset();
	stripPrimitiveRestart3IndexStagingBuffer.reset();
	stripPrimitiveRestart4IndexStagingBuffer.reset();
	primitiveRestartMinusOne2IndexStagingBuffer.reset();
	primitiveRestartMinusOne5IndexStagingBuffer.reset();
	minusOneIndexStagingBuffer.reset();
	stripPackedAttribute1StagingBuffer.reset();
	stripPackedAttribute2StagingBuffer.reset();
	sharedVertexPackedAttribute1StagingBuffer.reset();
	sharedVertexPackedAttribute2StagingBuffer.reset();
	sameVertexPackedAttribute1StagingBuffer.reset();
	sameVertexPackedAttribute2StagingBuffer.reset();

	// start new command buffer
	submitNowCommandBuffer->reset(vk::CommandBufferResetFlags());
	submitNowCommandBuffer->begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,  // flags
			nullptr  // pInheritanceInfo
		)
	);

	// matrix attributes, buffers and uniforms
	size_t transformationMatrix4x4BufferSize=size_t(numTriangles)*16*sizeof(float); // 64MB
	size_t transformationMatrix4x3BufferSize=size_t(numTriangles)*12*sizeof(float);
	size_t transformationDMatrix4x4BufferSize=size_t(numTriangles)*16*sizeof(double); // 128MB
	size_t normalMatrix4x3BufferSize=size_t(numTriangles)*16*sizeof(float); // 64MB
	size_t transformationPATBufferSize=size_t(numTriangles)*8*sizeof(float);
	constexpr size_t viewAndProjectionMatricesBufferSize=(16+16+12)*sizeof(float);
	constexpr size_t viewAndProjectionDMatricesBufferSize=16*sizeof(double)+(16+12)*sizeof(float);
	constexpr size_t materialUniformBufferSize=4*12+4+4;
	constexpr size_t materialNotPackedUniformBufferSize=4*16+4+4;
	constexpr size_t globalLightUniformBufferSize=12;
	constexpr size_t lightUniformBufferSize=16+4*12;
	constexpr size_t lightNotPackedUniformBufferSize=5*16;
	constexpr size_t allInOneLightingUniformBufferSize=6*16;
	startTime=chrono::high_resolution_clock::now();
	createBuffer(singleMatrixUniformBuffer,    singleMatrixUniformMemory,          16*sizeof(float),                  false,vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(singlePATBuffer,              singlePATMemory,                    8*sizeof(float),                   false,vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(sameMatrixAttribute,          sameMatrixAttributeMemory,          transformationMatrix4x4BufferSize, true, vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(sameMatrixBuffer,             sameMatrixBufferMemory,             transformationMatrix4x4BufferSize, true, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(sameMatrixRowMajorBuffer,     sameMatrixRowMajorBufferMemory,     transformationMatrix4x4BufferSize, true, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(sameMatrix4x3Buffer,          sameMatrix4x3BufferMemory,          transformationMatrix4x3BufferSize, true, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(sameMatrix4x3RowMajorBuffer,  sameMatrix4x3RowMajorBufferMemory,  transformationMatrix4x3BufferSize, true, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(sameDMatrixBuffer,            sameDMatrixBufferMemory,            transformationDMatrix4x4BufferSize,true, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(samePATBuffer,                samePATBufferMemory,                transformationPATBufferSize,       true, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(transformationMatrixAttribute,transformationMatrixAttributeMemory,transformationMatrix4x4BufferSize, true, vk::BufferUsageFlagBits::eVertexBuffer |vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(transformationMatrixBuffer,   transformationMatrixBufferMemory,   transformationMatrix4x4BufferSize, true, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(normalMatrix4x3Buffer,        normalMatrix4x3Memory,              normalMatrix4x3BufferSize,         true, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(viewAndProjectionMatricesUniformBuffer,viewAndProjectionMatricesMemory,viewAndProjectionMatricesBufferSize,false,vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(viewAndProjectionDMatricesUniformBuffer,viewAndProjectionDMatricesMemory,viewAndProjectionDMatricesBufferSize,false,vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(materialUniformBuffer,        materialUniformBufferMemory,        materialUniformBufferSize,         false,vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(materialNotPackedUniformBuffer,materialNotPackedUniformBufferMemory,materialNotPackedUniformBufferSize,false,vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(globalLightUniformBuffer,     globalLightUniformBufferMemory,     globalLightUniformBufferSize,      false,vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(lightUniformBuffer,           lightUniformBufferMemory,           lightUniformBufferSize,            false,vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(lightNotPackedUniformBuffer,  lightNotPackedUniformBufferMemory,  lightNotPackedUniformBufferSize,   false,vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(allInOneLightingUniformBuffer,allInOneLightingUniformBufferMemory,allInOneLightingUniformBufferSize, false,vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	createBuffer(indirectBuffer,               indirectBufferMemory,               (size_t(numTriangles)+1)*sizeof(vk::DrawIndirectCommand),true,vk::BufferUsageFlagBits::eIndirectBuffer|vk::BufferUsageFlagBits::eTransferDst,bindInfoList);
	totalMeasurementTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();
	cout<<"Second buffer and memory set"<<endl;
	cout<<"   Creation time: "<<totalMeasurementTime*1000<<"ms"<<endl;
#if 0
	cout<<"Second buffer set memory requirements: "<<(transformationMatrix4x4BufferSize+
			transformationMatrix4x4BufferSize+transformationMatrix4x4BufferSize+transformationMatrix4x3BufferSize+
			transformationMatrix4x3BufferSize+transformationDMatrix4x4BufferSize+transformationMatrix4x4BufferSize+
			transformationMatrix4x4BufferSize+normalMatrix4x3BufferSize+transformationPATBufferSize+
			viewAndProjectionMatricesBufferSize+viewAndProjectionDMatricesBufferSize+materialUniformBufferSize+
			materialNotPackedUniformBufferSize+globalLightUniformBufferSize+lightUniformBufferSize+
			lightNotPackedUniformBufferSize+allInOneLightingUniformBufferSize)/1024/1024<<"MiB"<<endl;
#endif

	if(sparseMode==SPARSE_NONE)
	{
		size_t totalMemorySize=0;
		auto startTime=chrono::high_resolution_clock::now();
		for(auto &bindInfo : bindInfoList) {
			device->bindBufferMemory(
				bindInfo.buffer,  // buffer
				bindInfo.memory,  // memory
				0  // memoryOffset
			);
			totalMemorySize+=bindInfo.size;
		}
		double totalMeasurementTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();
		cout<<"   Standard binding: "<<totalMeasurementTime*1000<<"ms"<<endl;
		cout<<"   (total amount of memory: "<<totalMemorySize/1024/1024<<"MiB, number of memory allocations: "<<bindInfoList.size()<<")"<<endl;
		bindInfoList.clear();
	}
	else
	{
		vector<vk::SparseBufferMemoryBindInfo> bufferBinds;
		vector<vk::SparseMemoryBind> memoryBinds;
		bufferBinds.reserve(bindInfoList.size());
		memoryBinds.reserve(bindInfoList.size());
		size_t numBlocks=0;
		size_t numMemObjs=0;
		for(auto &bindInfo : bindInfoList) {
			if(!bindInfo.sparseAllowed) {
				device->bindBufferMemory(
					bindInfo.buffer,  // buffer
					bindInfo.memory,  // memory
					0  // memoryOffset
				);
				continue;
			}
			auto& r=memoryBinds.emplace_back(
				0,  // resourceOffset
				bindInfo.size,  // size
				bindInfo.memory,  // memory
				0,  // memoryOffset
				vk::SparseMemoryBindFlags()  // flags
			);
			bufferBinds.emplace_back(
				bindInfo.buffer,  // buffer
				1,  // bindCount
				&r  // pBinds
			);
			numBlocks+=bindInfo.size/sparseBlockSize;
			numMemObjs++;
		}
		bindInfoList.clear();
		auto startTime=chrono::high_resolution_clock::now();
		sparseQueue.bindSparse(
			vk::BindSparseInfo(
				nullptr,  // waitSemaphores
				bufferBinds,  // bufferBinds
				nullptr,  // imageOpaqueBinds
				nullptr,  // imageBinds
				nullptr  // signalSemaphores
			),
			vk::Fence()
		);
		sparseQueue.waitIdle();
		double totalMeasurementTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();
		cout<<"   Sparse binding: "<<totalMeasurementTime*1000<<"ms\n"
		    <<"   Binding time of a memory block: "<<totalMeasurementTime/numBlocks*1e6<<"us"<<endl;
		cout<<"   (number of blocks: "<<numBlocks<<", number of memory objects: "<<numMemObjs<<")"<<endl;
	}

	// single matrix uniform staging buffer
	const float singleMatrixData[]{
		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,0.f,
		0.f,0.f,1.f,0.f,
		2.f/currentSurfaceExtent.width*64.f,0.f,0.f,1.f
	};
	StagingBuffer singleMatrixStagingBuffer(sizeof(singleMatrixData));
	memcpy(singleMatrixStagingBuffer.map(),singleMatrixData,sizeof(singleMatrixData));
	singleMatrixStagingBuffer.unmap();

	// single PAT uniform staging buffer
	const float singlePATData[]{
		0.f,0.f,0.f,1.f,  // zero rotation quaternion
		2.f/currentSurfaceExtent.width*4.f,0.f,0.f,1.f,  // xyz translation + scale
	};
	StagingBuffer singlePATStagingBuffer(sizeof(singlePATData));
	memcpy(singlePATStagingBuffer.map(),singlePATData,sizeof(singlePATData));
	singlePATStagingBuffer.unmap();

	// same matrix staging buffers
	const float matrixData[16]{
		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,0.f,
		0.f,0.f,1.f,0.f,
		0.f,2.f/currentSurfaceExtent.height*64.f,0.f,1.f,
	};
	const float matrixRowMajorData[16]{
		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,2.f/currentSurfaceExtent.height*64.f,
		0.f,0.f,1.f,0.f,
		0.f,0.f,0.f,1.f,
	};
	const float matrix4x3Data[12]{
		1.f,0.f,0.f,
		0.f,1.f,0.f,
		0.f,0.f,1.f,
		0.f,2.f/currentSurfaceExtent.height*64.f,0.f
	};
	const float matrix4x3RowMajorData[12]{
		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,2.f/currentSurfaceExtent.height*64.f,
		0.f,0.f,1.f,0.f,
	};
	const double dmatrixData[16]{
		1.,0.,0.,0.,
		0.,1.,0.,0.,
		0.,0.,1.,0.,
		0.,2./currentSurfaceExtent.height*64.,0.,1.
	};
	const float patData[8]{
		0.f,0.f,0.f,1.f,  // zero rotation quaternion
		0.f,2.f/currentSurfaceExtent.height*64.f,0.f,1.f,  // xyz translation + scale
	};
	StagingBuffer sameMatrixStagingBuffer(transformationMatrix4x4BufferSize);
	StagingBuffer sameMatrixRowMajorStagingBuffer(transformationMatrix4x4BufferSize);
	StagingBuffer sameMatrix4x3StagingBuffer(transformationMatrix4x3BufferSize);
	StagingBuffer sameMatrix4x3RowMajorStagingBuffer(transformationMatrix4x3BufferSize);
	StagingBuffer sameDMatrixStagingBuffer(transformationDMatrix4x4BufferSize);
	StagingBuffer samePATStagingBuffer(transformationPATBufferSize);
	sameMatrixStagingBuffer.map();
	sameMatrixRowMajorStagingBuffer.map();
	sameMatrix4x3StagingBuffer.map();
	sameMatrix4x3RowMajorStagingBuffer.map();
	sameDMatrixStagingBuffer.map();
	samePATStagingBuffer.map();
	for(size_t i=0; i<size_t(numTriangles); i++)
		memcpy(reinterpret_cast<char*>(sameMatrixStagingBuffer.ptr)+(i*sizeof(matrixData)),matrixData,sizeof(matrixData));
	for(size_t i=0; i<size_t(numTriangles); i++)
		memcpy(reinterpret_cast<char*>(sameMatrixRowMajorStagingBuffer.ptr)+(i*sizeof(matrixRowMajorData)),matrixRowMajorData,sizeof(matrixRowMajorData));
	for(size_t i=0; i<size_t(numTriangles); i++)
		memcpy(reinterpret_cast<char*>(sameMatrix4x3StagingBuffer.ptr)+(i*sizeof(matrix4x3Data)),matrix4x3Data,sizeof(matrix4x3Data));
	for(size_t i=0; i<size_t(numTriangles); i++)
		memcpy(reinterpret_cast<char*>(sameMatrix4x3RowMajorStagingBuffer.ptr)+(i*sizeof(matrix4x3RowMajorData)),matrix4x3RowMajorData,sizeof(matrix4x3RowMajorData));
	for(size_t i=0; i<size_t(numTriangles); i++)
		memcpy(reinterpret_cast<char*>(sameDMatrixStagingBuffer.ptr)+(i*sizeof(dmatrixData)),dmatrixData,sizeof(dmatrixData));
	for(size_t i=0; i<size_t(numTriangles); i++)
		memcpy(reinterpret_cast<char*>(samePATStagingBuffer.ptr)+(i*sizeof(patData)),patData,sizeof(patData));
	sameMatrixStagingBuffer.unmap();
	sameMatrixRowMajorStagingBuffer.unmap();
	sameMatrix4x3StagingBuffer.unmap();
	sameMatrix4x3RowMajorStagingBuffer.unmap();
	sameDMatrixStagingBuffer.unmap();
	samePATStagingBuffer.unmap();

	// transformation matrix staging buffer
	StagingBuffer transformationMatrixStagingBuffer(transformationMatrix4x4BufferSize);
	generateMatrices(
		reinterpret_cast<float*>(transformationMatrixStagingBuffer.map()),numTriangles/2,triangleSize,
		renderingExtent.width,renderingExtent.height,
		2./currentSurfaceExtent.width,2./currentSurfaceExtent.height,0.,0.);
	transformationMatrixStagingBuffer.unmap();

	// normal matrix staging buffer
	const float normalMatrix4x3Data[]{
		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,0.f,
		0.f,0.f,1.f,0.f,
	};
	StagingBuffer normalMatrix4x3StagingBuffer(normalMatrix4x3BufferSize);
	normalMatrix4x3StagingBuffer.map();
	for(size_t i=0; i<size_t(numTriangles); i++)
		memcpy(reinterpret_cast<char*>(normalMatrix4x3StagingBuffer.ptr)+(i*sizeof(normalMatrix4x3Data)),normalMatrix4x3Data,sizeof(normalMatrix4x3Data));
	normalMatrix4x3StagingBuffer.unmap();

	// projection matrix staging buffers
	constexpr float viewAndProjectionMatricesData[]{
		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,0.f,
		0.f,0.f,1.f,0.f,
		0.f,0.f,0.f,1.f,

		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,0.f,
		0.f,0.f,1.f,0.f,
		0.f,0.f,0.f,1.f,

		1.f,0.f,0.f,0.f,
		0.f,1.f,0.f,0.f,
		0.f,0.f,1.f,0.f,
	};
	constexpr struct {
		double view[16] = {
			1.,0.,0.,0.,
			0.,1.,0.,0.,
			0.,0.,1.,0.,
			0.,0.,0.,1.,
		};
		float projection[16] = {
			1.,0.,0.,0.,
			0.,1.,0.,0.,
			0.,0.,1.,0.,
			0.,0.,0.,1.,
		};
		float normal[12] = {
			1.,0.,0.,0.,
			0.,1.,0.,0.,
			0.,0.,1.,0.,
		};
	} viewAndProjectionDMatricesData;
	static_assert(viewAndProjectionMatricesBufferSize==sizeof(viewAndProjectionMatricesData),"viewAndProjectionMatricesBufferSize must match size of viewAndProjectionMatricesData");
	static_assert(viewAndProjectionDMatricesBufferSize==sizeof(viewAndProjectionDMatricesData),"viewAndProjectionDMatricesBufferSize must match size of viewAndProjectionDMatricesData");
	StagingBuffer viewAndProjectionMatricesStagingBuffer(viewAndProjectionMatricesBufferSize);
	StagingBuffer viewAndProjectionDMatricesStagingBuffer(viewAndProjectionDMatricesBufferSize);
	memcpy(viewAndProjectionMatricesStagingBuffer.map(),viewAndProjectionMatricesData,viewAndProjectionMatricesBufferSize);
	memcpy(viewAndProjectionDMatricesStagingBuffer.map(),&viewAndProjectionDMatricesData,viewAndProjectionDMatricesBufferSize);
	viewAndProjectionMatricesStagingBuffer.unmap();
	viewAndProjectionDMatricesStagingBuffer.unmap();

	// material staging buffer
	constexpr struct {
		float f[13] = {
			0.8f,0.8f,0.8f,  // ambientColor
			0.8f,0.8f,0.8f,  // diffuseColor
			0.8f,0.8f,0.8f,  // specularColor
			0.0f,0.0f,0.0f,  // emissiveColor
			5.f,  // shininess
		};
		int32_t i[1] = {
			0x2100, // 0 - no texturing, 0x2100 - modulate, 0x1e01 - replace, 0x2101 - decal
		};
	} materialUniformData;
	static_assert(materialUniformBufferSize==sizeof(materialUniformData),"materialUniformBufferSize must match size of materialUniformData");
	StagingBuffer materialUniformStagingBuffer(materialUniformBufferSize);
	materialUniformStagingBuffer.map();
	memcpy(materialUniformStagingBuffer.ptr,&materialUniformData,sizeof(materialUniformData));
	materialUniformStagingBuffer.unmap();

	// material not packed staging buffer
	constexpr struct {
		float f[17] = {
			0.8f,0.8f,0.8f,1.f,  // ambientColor
			0.8f,0.8f,0.8f,1.f,  // diffuseColor
			0.8f,0.8f,0.8f,1.f,  // specularColor
			0.0f,0.0f,0.0f,1.f,  // emissiveColor
			5.f,  // shininess
		};
		int32_t i[1] = {
			0x2100, // 0 - no texturing, 0x2100 - modulate, 0x1e01 - replace, 0x2101 - decal
		};
	} materialNotPackedUniformData;
	static_assert(materialNotPackedUniformBufferSize==sizeof(materialNotPackedUniformData),"materialNotPackedUniformBufferSize must match size of materialNotPackedUniformData");
	StagingBuffer materialNotPackedUniformStagingBuffer(materialNotPackedUniformBufferSize);
	materialNotPackedUniformStagingBuffer.map();
	memcpy(materialNotPackedUniformStagingBuffer.ptr,&materialNotPackedUniformData,sizeof(materialNotPackedUniformData));
	materialNotPackedUniformStagingBuffer.unmap();

	// global light staging buffer
	const float globalLightUniformData[]{
		0.2f,0.2f,0.2f,  // globalAmbientLight
	};
	static_assert(globalLightUniformBufferSize==sizeof(globalLightUniformData),"globalLightUniformBufferSize must match size of globalLightUniformData");
	StagingBuffer globalLightUniformStagingBuffer(globalLightUniformBufferSize);
	globalLightUniformStagingBuffer.map();
	memcpy(globalLightUniformStagingBuffer.ptr,globalLightUniformData,sizeof(globalLightUniformData));
	globalLightUniformStagingBuffer.unmap();

	// light source staging buffer
	const float lightUniformData[]{
		-0.4f,0.4f,0.2f,1.0f,  // lightPosition
		1.0f,0.0f,0.0f,  // lightAttenuation
		0.2f,0.2f,0.2f,  // ambientLight
		0.6f,0.6f,0.6f,  // diffuseLight
		0.6f,0.6f,0.6f,  // specularLight
	};
	static_assert(lightUniformBufferSize==sizeof(lightUniformData),"lightUniformBufferSize must match size of lightUniformData");
	StagingBuffer lightUniformStagingBuffer(lightUniformBufferSize);
	lightUniformStagingBuffer.map();
	memcpy(lightUniformStagingBuffer.ptr,lightUniformData,sizeof(lightUniformData));
	lightUniformStagingBuffer.unmap();

	// light source staging buffer
	const float lightNotPackedUniformData[]{
		-0.4f,0.4f,0.2f,1.f,  // lightPosition
		1.0f,0.0f,0.0f,0.f,  // lightAttenuation
		0.2f,0.2f,0.2f,1.f,  // ambientLight
		0.6f,0.6f,0.6f,1.f,  // diffuseLight
		0.6f,0.6f,0.6f,1.f,  // specularLight
	};
	static_assert(lightNotPackedUniformBufferSize==sizeof(lightNotPackedUniformData),"lightNotPackedUniformBufferSize must match size of lightNotPackedUniformData");
	StagingBuffer lightNotPackedUniformStagingBuffer(lightNotPackedUniformBufferSize);
	lightNotPackedUniformStagingBuffer.map();
	memcpy(lightNotPackedUniformStagingBuffer.ptr,lightNotPackedUniformData,sizeof(lightNotPackedUniformData));
	lightNotPackedUniformStagingBuffer.unmap();

	// all-in-one lighting staging buffer
	const float allInOneLightingUniformData[]{
		0.8f,0.8f,0.8f,1.f,  // ambientColor
		0.8f,0.8f,0.8f,1.f,  // diffuseColor
		0.2f,0.2f,0.2f,0.f,  // globalAmbientLight
		-0.4f,0.4f,0.2f,  // lightPosition
		1.0f,0.0f,0.0f,  // lightAttenuation
		0.2f,0.2f,0.2f,  // ambientLight
		0.6f,0.6f,0.6f,  // diffuseLight
	};
	static_assert(allInOneLightingUniformBufferSize==sizeof(allInOneLightingUniformData),"allInOneLightingUniformBufferSize must match size of allInOneLightingUniformData");
	StagingBuffer allInOneLightingUniformStagingBuffer(allInOneLightingUniformBufferSize);
	allInOneLightingUniformStagingBuffer.map();
	memcpy(allInOneLightingUniformStagingBuffer.ptr,allInOneLightingUniformData,sizeof(allInOneLightingUniformData));
	allInOneLightingUniformStagingBuffer.unmap();

	// copy data from staging to attributes, buffers and uniforms
	submitNowCommandBuffer->copyBuffer(
		singleMatrixStagingBuffer.buffer.get(),  // srcBuffer
		singleMatrixUniformBuffer.get(),         // dstBuffer
		1,                                       // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,sizeof(singleMatrixData))  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		singlePATStagingBuffer.buffer.get(),  // srcBuffer
		singlePATBuffer.get(),                // dstBuffer
		1,                                    // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,sizeof(singlePATBuffer))  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		sameMatrixStagingBuffer.buffer.get(),  // srcBuffer
		sameMatrixAttribute.get(),             // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,transformationMatrix4x4BufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		sameMatrixStagingBuffer.buffer.get(),  // srcBuffer
		sameMatrixBuffer.get(),                // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,transformationMatrix4x4BufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		sameMatrixRowMajorStagingBuffer.buffer.get(),  // srcBuffer
		sameMatrixRowMajorBuffer.get(),                // dstBuffer
		1,                                             // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,transformationMatrix4x4BufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		sameMatrix4x3StagingBuffer.buffer.get(),  // srcBuffer
		sameMatrix4x3Buffer.get(),                // dstBuffer
		1,                                        // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,transformationMatrix4x3BufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		sameMatrix4x3RowMajorStagingBuffer.buffer.get(),  // srcBuffer
		sameMatrix4x3RowMajorBuffer.get(),                // dstBuffer
		1,                                                // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,transformationMatrix4x3BufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		sameDMatrixStagingBuffer.buffer.get(),  // srcBuffer
		sameDMatrixBuffer.get(),                // dstBuffer
		1,                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,transformationDMatrix4x4BufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		samePATStagingBuffer.buffer.get(),  // srcBuffer
		samePATBuffer.get(),                // dstBuffer
		1,                                  // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,transformationPATBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		transformationMatrixStagingBuffer.buffer.get(),  // srcBuffer
		transformationMatrixAttribute.get(),             // dstBuffer
		1,                                               // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,transformationMatrix4x4BufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		transformationMatrixStagingBuffer.buffer.get(),  // srcBuffer
		transformationMatrixBuffer.get(),                // dstBuffer
		1,                                               // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,transformationMatrix4x4BufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		normalMatrix4x3StagingBuffer.buffer.get(),  // srcBuffer
		normalMatrix4x3Buffer.get(),                // dstBuffer
		1,                                          // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,normalMatrix4x3BufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		viewAndProjectionMatricesStagingBuffer.buffer.get(),  // srcBuffer
		viewAndProjectionMatricesUniformBuffer.get(),         // dstBuffer
		1,                                                    // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,viewAndProjectionMatricesBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		viewAndProjectionDMatricesStagingBuffer.buffer.get(),  // srcBuffer
		viewAndProjectionDMatricesUniformBuffer.get(),         // dstBuffer
		1,                                                     // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,viewAndProjectionDMatricesBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		materialUniformStagingBuffer.buffer.get(),  // srcBuffer
		materialUniformBuffer.get(),                // dstBuffer
		1,                                          // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,materialUniformBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		materialNotPackedUniformStagingBuffer.buffer.get(),  // srcBuffer
		materialNotPackedUniformBuffer.get(),                // dstBuffer
		1,                                                   // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,materialNotPackedUniformBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		globalLightUniformStagingBuffer.buffer.get(),  // srcBuffer
		globalLightUniformBuffer.get(),                // dstBuffer
		1,                                             // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,globalLightUniformBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		lightUniformStagingBuffer.buffer.get(),  // srcBuffer
		lightUniformBuffer.get(),                // dstBuffer
		1,                                       // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,lightUniformBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		lightNotPackedUniformStagingBuffer.buffer.get(),  // srcBuffer
		lightNotPackedUniformBuffer.get(),                // dstBuffer
		1,                                                // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,lightNotPackedUniformBufferSize)  // pRegions
	);
	submitNowCommandBuffer->copyBuffer(
		allInOneLightingUniformStagingBuffer.buffer.get(),  // srcBuffer
		allInOneLightingUniformBuffer.get(),                // dstBuffer
		1,                                                  // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,allInOneLightingUniformBufferSize)  // pRegions
	);

	// indirect staging buffer
	StagingBuffer indirectStagingBuffer((size_t(numTriangles)+1)*sizeof(vk::DrawIndirectCommand));
	auto indirectBufferPtr=reinterpret_cast<vk::DrawIndirectCommand*>(indirectStagingBuffer.map());
	for(size_t i=0; i<size_t(numTriangles); i++) {
		indirectBufferPtr[i].vertexCount=3;
		indirectBufferPtr[i].instanceCount=1;
		indirectBufferPtr[i].firstVertex=uint32_t(i)*3;
		indirectBufferPtr[i].firstInstance=0;
	}
	indirectBufferPtr[numTriangles].vertexCount=3;
	indirectBufferPtr[numTriangles].instanceCount=numTriangles;
	indirectBufferPtr[numTriangles].firstVertex=0;
	indirectBufferPtr[numTriangles].firstInstance=0;
	indirectStagingBuffer.unmap();

	// copy data from staging to uniform buffer
	submitNowCommandBuffer->copyBuffer(
		indirectStagingBuffer.buffer.get(),  // srcBuffer
		indirectBuffer.get(),                // dstBuffer
		1,                                   // regionCount
		&(const vk::BufferCopy&)vk::BufferCopy(0,0,(size_t(numTriangles)+1)*sizeof(vk::DrawIndirectCommand))  // pRegions
	);

	// single texel image
	StagingBuffer singleTexelStagingBuffer(4);
	singleTexelStagingBuffer.map();
	reinterpret_cast<uint32_t*>(singleTexelStagingBuffer.ptr)[0]=0xffffffff;
	singleTexelStagingBuffer.unmap();
	submitNowCommandBuffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,  // srcStageMask
		vk::PipelineStageFlagBits::eTransfer,  // dstStageMask
		vk::DependencyFlags(),  // dependencyFlags
		nullptr,  // memoryBarriers
		nullptr,  // bufferMemoryBarriers
		vk::ImageMemoryBarrier(  // imageMemoryBarriers
			vk::AccessFlags(),  // srcAccessMask
			vk::AccessFlagBits::eTransferWrite,  // dstAccessMask
			vk::ImageLayout::eUndefined,  // oldLayout
			vk::ImageLayout::eTransferDstOptimal,  // newLayout
			VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
			VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
			singleTexelImage.get(),  // image
			vk::ImageSubresourceRange(  // subresourceRange
				vk::ImageAspectFlagBits::eColor,  // aspectMask
				0,  // baseMipLevel
				1,  // levelCount
				0,  // baseArrayLayer
				1   // layerCount
			)
		)
	);
	submitNowCommandBuffer->copyBufferToImage(
		singleTexelStagingBuffer.buffer.get(),  // srcBuffer
		singleTexelImage.get(),                 // dstImage
		vk::ImageLayout::eTransferDstOptimal,   // dstImageLayout
		vk::BufferImageCopy(  // regions
			0,  // bufferOffset
			0,  // bufferRowLength
			0,  // bufferImageHeight
			vk::ImageSubresourceLayers(  // imageSubresource
				vk::ImageAspectFlagBits::eColor,  // aspectMask
				0,  // mipLevel
				0,  // baseArrayLayer
				1   // layerCount
			),
			vk::Offset3D(0,0,0),  // imageOffset
			vk::Extent3D(1,1,1)   // imageExtent
		)
	);
	submitNowCommandBuffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,  // srcStageMask
		vk::PipelineStageFlagBits::eFragmentShader,  // dstStageMask
		vk::DependencyFlags(),  // dependencyFlags
		nullptr,  // memoryBarriers
		nullptr,  // bufferMemoryBarriers
		vk::ImageMemoryBarrier(  // imageMemoryBarriers
			vk::AccessFlagBits::eTransferWrite,  // srcAccessMask
			vk::AccessFlagBits::eShaderRead,  // dstAccessMask
			vk::ImageLayout::eTransferDstOptimal,  // oldLayout
			vk::ImageLayout::eShaderReadOnlyOptimal,  // newLayout
			VK_QUEUE_FAMILY_IGNORED,  // srcQueueFamilyIndex
			VK_QUEUE_FAMILY_IGNORED,  // dstQueueFamilyIndex
			singleTexelImage.get(),  // image
			vk::ImageSubresourceRange(  // subresourceRange
				vk::ImageAspectFlagBits::eColor,  // aspectMask
				0,  // baseMipLevel
				1,  // levelCount
				0,  // baseArrayLayer
				1   // layerCount
			)
		)
	);

	// submit command buffer
	submitNowCommandBuffer->end();
	device->resetFences(fence.get());
	graphicsQueue.submit(
		vk::SubmitInfo(  // submits (vk::ArrayProxy)
			0,nullptr,nullptr,       // waitSemaphoreCount,pWaitSemaphores,pWaitDstStageMask
			1,&submitNowCommandBuffer.get(),  // commandBufferCount,pCommandBuffers
			0,nullptr                // signalSemaphoreCount,pSignalSemaphores
		),
		fence.get()  // fence
	);

	// wait for work to complete
	r=device->waitForFences(
		fence.get(),   // fences (vk::ArrayProxy)
		VK_TRUE,       // waitAll
		uint64_t(3e9)  // timeout (3s)
	);
	if(r==vk::Result::eTimeout)
		throw std::runtime_error("GPU timeout. Task is probably hanging.");


	// descriptor sets
	descriptorPool=
		device->createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),  // flags
				25,  // maxSets
				3,  // poolSizeCount
				array<vk::DescriptorPoolSize,3>{  // pPoolSizes
					vk::DescriptorPoolSize(
						vk::DescriptorType::eUniformBuffer,  // type
						24  // descriptorCount
					),
					vk::DescriptorPoolSize(
						vk::DescriptorType::eStorageBuffer,  // type
						28  // descriptorCount
					),
					vk::DescriptorPoolSize(
						vk::DescriptorType::eCombinedImageSampler,  // type
						2  // descriptorCount
					),
				}.data()
			)
		);
	oneUniformVSDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&oneUniformVSDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	one4fUniformFSDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&oneUniformFSDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	one4bUniformFSDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&oneUniformFSDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	coordinateBufferDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&oneBufferDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	sameMatrixBufferDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&oneBufferDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationMatrixBufferDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&oneBufferDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	singlePackedBufferDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&oneBufferDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	twoBuffersDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&twoBuffersDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	threeBuffersDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&threeBuffersDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	threeBuffersInGSDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&threeBuffersInGSDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	threeUniformFSDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&threeUniformFSDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationThreeMatricesDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&bufferAndUniformDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationThreeMatricesRowMajorDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&bufferAndUniformDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationThreeMatrices4x3DescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&bufferAndUniformDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationThreeMatrices4x3RowMajorDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&bufferAndUniformDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationThreeDMatricesDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&bufferAndUniformDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationTwoMatricesAndPATDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&bufferAndUniformDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationTwoMatricesAndSinglePATDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&bufferAndUniformDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationFiveMatricesDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&twoBuffersAndUniformDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationFiveMatricesUsingGSDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&fourBuffersAndUniformInGSDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	transformationFiveMatricesUsingGSAndAttributesDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&twoBuffersAndUniformInGSDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	phongTexturedThreeDMatricesUsingGSAndAttributesDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&bufferAndUniformInGSDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	phongTexturedDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&phongTexturedDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	phongTexturedNotPackedDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&phongTexturedDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	allInOneLightingUniformDescriptorSet=
		device->allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool.get(),  // descriptorPool
				1,  // descriptorSetCount
				&oneUniformFSDescriptorSetLayout.get()  // pSetLayouts
			)
		)[0];
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			oneUniformVSDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eUniformBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					singleMatrixUniformBuffer.get(),  // buffer
					0,  // offset
					16*sizeof(float)  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			one4fUniformFSDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eUniformBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					materialUniformBuffer.get(),  // buffer
					0,  // offset
					4*sizeof(float)  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			one4bUniformFSDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eUniformBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					materialUniformBuffer.get(),  // buffer
					0*sizeof(float),  // offset
					4  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			coordinateBufferDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					coordinateBuffer.get(),  // buffer
					0,  // offset
					coordinateBufferSize  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			sameMatrixBufferDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					sameMatrixBuffer.get(),  // buffer
					0,  // offset
					size_t(numTriangles)*16*sizeof(float)  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			transformationMatrixBufferDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					transformationMatrixBuffer.get(),  // buffer
					0,  // offset
					size_t(numTriangles)*16*sizeof(float)  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			singlePackedBufferDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			1,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					singlePackedBuffer.get(),  // buffer
					0,  // offset
					size_t(numTriangles)*3*32  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			twoBuffersDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			2,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,2>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					packedBuffer1.get(),  // buffer
					0,  // offset
					packedDataBufferSize  // range
				),
				vk::DescriptorBufferInfo(
					packedBuffer2.get(),  // buffer
					0,  // offset
					packedDataBufferSize  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			threeBuffersDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			3,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,3>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					packedBuffer1.get(),  // buffer
					0,  // offset
					packedDataBufferSize  // range
				),
				vk::DescriptorBufferInfo(
					packedBuffer2.get(),  // buffer
					0,  // offset
					packedDataBufferSize  // range
				),
				vk::DescriptorBufferInfo(
					sameMatrixBuffer.get(),  // buffer
					0,  // offset
					size_t(numTriangles)*16*sizeof(float)  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		vk::WriteDescriptorSet(  // descriptorWrites
			threeBuffersInGSDescriptorSet,  // dstSet
			0,  // dstBinding
			0,  // dstArrayElement
			3,  // descriptorCount
			vk::DescriptorType::eStorageBuffer,  // descriptorType
			nullptr,  // pImageInfo
			array<vk::DescriptorBufferInfo,3>{  // pBufferInfo
				vk::DescriptorBufferInfo(
					packedBuffer1.get(),  // buffer
					0,  // offset
					packedDataBufferSize  // range
				),
				vk::DescriptorBufferInfo(
					packedBuffer2.get(),  // buffer
					0,  // offset
					packedDataBufferSize  // range
				),
				vk::DescriptorBufferInfo(
					sameMatrixBuffer.get(),  // buffer
					0,  // offset
					size_t(numTriangles)*16*sizeof(float)  // range
				),
			}.data(),
			nullptr  // pTexelBufferView
		),
		nullptr  // descriptorCopies
	);
	device->updateDescriptorSets(
		array<vk::WriteDescriptorSet,28>{{  // descriptorWrites
			{
				transformationThreeMatricesDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						sameMatrixBuffer.get(),  // buffer
						0,  // offset
						transformationMatrix4x4BufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationThreeMatricesRowMajorDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						sameMatrixRowMajorBuffer.get(),  // buffer
						0,  // offset
						transformationMatrix4x4BufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationThreeMatrices4x3DescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						sameMatrix4x3Buffer.get(),  // buffer
						0,  // offset
						transformationMatrix4x3BufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationThreeMatrices4x3RowMajorDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						sameMatrix4x3RowMajorBuffer.get(),  // buffer
						0,  // offset
						transformationMatrix4x3BufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationThreeDMatricesDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						sameDMatrixBuffer.get(),  // buffer
						0,  // offset
						transformationDMatrix4x4BufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationTwoMatricesAndPATDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						samePATBuffer.get(),  // buffer
						0,  // offset
						transformationPATBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationTwoMatricesAndSinglePATDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						singlePATBuffer.get(),  // buffer
						0,  // offset
						8*sizeof(float)  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationThreeMatricesDescriptorSet,  // dstSet
				1,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationThreeMatricesRowMajorDescriptorSet,  // dstSet
				1,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationThreeMatrices4x3DescriptorSet,  // dstSet
				1,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationThreeMatrices4x3RowMajorDescriptorSet,  // dstSet
				1,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationThreeDMatricesDescriptorSet,  // dstSet
				1,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionDMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionDMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationTwoMatricesAndPATDescriptorSet,  // dstSet
				1,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationTwoMatricesAndSinglePATDescriptorSet,  // dstSet
				1,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationFiveMatricesDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				2,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,2>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						sameMatrixBuffer.get(),  // buffer
						0,  // offset
						transformationMatrix4x4BufferSize  // range
					),
					vk::DescriptorBufferInfo(
						normalMatrix4x3Buffer.get(),  // buffer
						0,  // offset
						normalMatrix4x3BufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationFiveMatricesDescriptorSet,  // dstSet
				2,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationFiveMatricesUsingGSDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				4,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,4>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						packedBuffer1.get(),  // buffer
						0,  // offset
						packedDataBufferSize  // range
					),
					vk::DescriptorBufferInfo(
						packedBuffer2.get(),  // buffer
						0,  // offset
						packedDataBufferSize  // range
					),
					vk::DescriptorBufferInfo(
						sameMatrixBuffer.get(),  // buffer
						0,  // offset
						transformationMatrix4x4BufferSize  // range
					),
					vk::DescriptorBufferInfo(
						normalMatrix4x3Buffer.get(),  // buffer
						0,  // offset
						normalMatrix4x3BufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationFiveMatricesUsingGSDescriptorSet,  // dstSet
				4,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationFiveMatricesUsingGSAndAttributesDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				2,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,2>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						sameMatrixBuffer.get(),  // buffer
						0,  // offset
						transformationMatrix4x4BufferSize  // range
					),
					vk::DescriptorBufferInfo(
						normalMatrix4x3Buffer.get(),  // buffer
						0,  // offset
						normalMatrix4x3BufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				transformationFiveMatricesUsingGSAndAttributesDescriptorSet,  // dstSet
				2,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				phongTexturedThreeDMatricesUsingGSAndAttributesDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eStorageBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						sameDMatrixBuffer.get(),  // buffer
						0,  // offset
						transformationDMatrix4x4BufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				phongTexturedThreeDMatricesUsingGSAndAttributesDescriptorSet,  // dstSet
				1,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						viewAndProjectionDMatricesUniformBuffer.get(),  // buffer
						0,  // offset
						viewAndProjectionDMatricesBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				phongTexturedDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				3,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,3>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						materialUniformBuffer.get(),  // buffer
						0,  // offset
						materialUniformBufferSize  // range
					),
					vk::DescriptorBufferInfo(
						globalLightUniformBuffer.get(),  // buffer
						0,  // offset
						globalLightUniformBufferSize  // range
					),
					vk::DescriptorBufferInfo(
						lightUniformBuffer.get(),  // buffer
						0,  // offset
						lightUniformBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				phongTexturedNotPackedDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				3,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,3>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						materialNotPackedUniformBuffer.get(),  // buffer
						0,  // offset
						materialNotPackedUniformBufferSize  // range
					),
					vk::DescriptorBufferInfo(
						globalLightUniformBuffer.get(),  // buffer
						0,  // offset
						globalLightUniformBufferSize  // range
					),
					vk::DescriptorBufferInfo(
						lightNotPackedUniformBuffer.get(),  // buffer
						0,  // offset
						lightNotPackedUniformBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				phongTexturedDescriptorSet,  // dstSet
				3,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eCombinedImageSampler,  // descriptorType
				array<vk::DescriptorImageInfo,1>{  // pImageInfo
					vk::DescriptorImageInfo(
						trilinearSampler.get(),      // sampler
						singleTexelImageView.get(),  // imageView
						vk::ImageLayout::eShaderReadOnlyOptimal  // imageLayout
					),
				}.data(),
				nullptr,  // pBufferInfo
				nullptr  // pTexelBufferView
			},
			{
				phongTexturedNotPackedDescriptorSet,  // dstSet
				3,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eCombinedImageSampler,  // descriptorType
				array<vk::DescriptorImageInfo,1>{  // pImageInfo
					vk::DescriptorImageInfo(
						trilinearSampler.get(),      // sampler
						singleTexelImageView.get(),  // imageView
						vk::ImageLayout::eShaderReadOnlyOptimal  // imageLayout
					),
				}.data(),
				nullptr,  // pBufferInfo
				nullptr  // pTexelBufferView
			},
			{
				threeUniformFSDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				3,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,3>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						materialUniformBuffer.get(),  // buffer
						0,  // offset
						materialUniformBufferSize  // range
					),
					vk::DescriptorBufferInfo(
						globalLightUniformBuffer.get(),  // buffer
						0,  // offset
						globalLightUniformBufferSize  // range
					),
					vk::DescriptorBufferInfo(
						lightUniformBuffer.get(),  // buffer
						0,  // offset
						lightUniformBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
			{
				allInOneLightingUniformDescriptorSet,  // dstSet
				0,  // dstBinding
				0,  // dstArrayElement
				1,  // descriptorCount
				vk::DescriptorType::eUniformBuffer,  // descriptorType
				nullptr,  // pImageInfo
				array<vk::DescriptorBufferInfo,1>{  // pBufferInfo
					vk::DescriptorBufferInfo(
						allInOneLightingUniformBuffer.get(),  // buffer
						0,  // offset
						allInOneLightingUniformBufferSize  // range
					),
				}.data(),
				nullptr  // pTexelBufferView
			},
		}},
		nullptr  // descriptorCopies
	);


	// timestamp properties
	timestampValidBits=
		physicalDevice.getQueueFamilyProperties()[graphicsQueueFamily].timestampValidBits;
	timestampPeriod_ns=
		physicalDevice.getProperties().limits.timestampPeriod;
	cout<<"Timestamp number of bits:  "<<timestampValidBits<<endl;
	cout<<"Timestamp period:          "<<timestampPeriod_ns<<" ns\n"<<endl;
	if(timestampValidBits==0)
		throw runtime_error("Timestamps are not supported.");

	// timestamp pool
	timestampPool=
		device->createQueryPoolUnique(
			vk::QueryPoolCreateInfo(
				vk::QueryPoolCreateFlags(),  // flags
				vk::QueryType::eTimestamp,   // queryType
				uint32_t(tests.size())*2,    // queryCount
				vk::QueryPipelineStatisticFlags()  // pipelineStatistics
			)
		);

	// reallocate command buffers
	if(commandBuffers.size()!=swapchainImages.size()) {
		commandBuffers=
			device->allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(
					commandPool.get(),                 // commandPool
					vk::CommandBufferLevel::ePrimary,  // level
					uint32_t(swapchainImages.size())   // commandBufferCount
				)
			);
	}

	// record command buffers
	for(size_t i=0,c=swapchainImages.size(); i<c; i++) {

		// begin command buffer
		vk::CommandBuffer cb=commandBuffers[i].get();
		cb.begin(
			vk::CommandBufferBeginInfo(
				vk::CommandBufferUsageFlagBits::eSimultaneousUse,  // flags
				nullptr  // pInheritanceInfo
			)
		);
		cb.resetQueryPool(timestampPool.get(),0,uint32_t(tests.size())*2);
		uint32_t timestampIndex=0;

		// begin test lambda
		auto beginTest=
			[](vk::CommandBuffer cb,vk::Framebuffer framebuffer,vk::Extent2D currentSurfaceExtent,
			   vk::Pipeline pipeline,vk::PipelineLayout pipelineLayout,
			   const vector<vk::Buffer>& attributes,const vector<vk::DescriptorSet>& descriptorSets)
			{
				cb.beginRenderPass(
					vk::RenderPassBeginInfo(
						renderPass.get(),         // renderPass
						framebuffer,              // framebuffer
						vk::Rect2D(vk::Offset2D(0,0),currentSurfaceExtent),  // renderArea
						2,                        // clearValueCount
						array<vk::ClearValue,2>{  // pClearValues
							vk::ClearColorValue(array<float,4>{0.f,0.f,0.f,1.f}),
							vk::ClearDepthStencilValue(1.f,0)
						}.data()
					),
					vk::SubpassContents::eInline
				);
				cb.bindPipeline(vk::PipelineBindPoint::eGraphics,pipeline);  // bind pipeline
				if(descriptorSets.size()>0)
					cb.bindDescriptorSets(
						vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
						pipelineLayout,  // layout
						0,  // firstSet
						descriptorSets,  // descriptorSets
						nullptr  // dynamicOffsets
					);
				if(attributes.size()>0)
					cb.bindVertexBuffers(
						0,  // firstBinding
						uint32_t(attributes.size()),  // bindingCount
						attributes.data(),  // pBuffers
						vector<vk::DeviceSize>(attributes.size(),0).data()  // pOffsets
					);
				cb.pipelineBarrier(
					vk::PipelineStageFlagBits::eBottomOfPipe,  // srcStageMask
					vk::PipelineStageFlagBits::eTopOfPipe,  // dstStageMask
					vk::DependencyFlags(),  // dependencyFlags
					0,nullptr,  // memoryBarrierCount+pMemoryBarriers
					0,nullptr,  // bufferMemoryBarrierCount+pBufferMemoryBarriers
					0,nullptr   // imageMemoryBarrierCount+pImageMemoryBarriers
				);
			};

		// render something to put GPU out of power saving states
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          coordinateAttributePipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>());
		cb.draw(3*numTriangles,1,0,0);
		cb.draw(3*numTriangles,1,0,0);
		cb.endRenderPass();

		// attributeless constant output test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessConstantOutputPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// attributeless input indices test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessInputIndicesPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// attributeless instancing test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessConstantOutputPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3,numTriangles,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// geometry shader constant output test
		if(enabledFeatures.geometryShader) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          geometryShaderConstantOutputPipeline.get(),simplePipelineLayout.get(),
			          vector<vk::Buffer>(),
			          vector<vk::DescriptorSet>());
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		}
		else {
			tests[timestampIndex/2].enabled=false;
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
		}

		// attributeless indirect buffer instancing
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessConstantOutputPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndirect(indirectBuffer.get(),  // buffer
		                size_t(numTriangles)*sizeof(vk::DrawIndirectCommand),  // offset
		                1,  // drawCount
		                sizeof(vk::DrawIndirectCommand));  // stride
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// attributeless indirect buffer per-triangle record
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessConstantOutputPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		if(enabledFeatures.multiDrawIndirect)
			cb.drawIndirect(indirectBuffer.get(),  // buffer
			                0,  // offset
			                numTriangles,  // drawCount
			                sizeof(vk::DrawIndirectCommand));  // stride
		else
			tests[timestampIndex/2].enabled=false;
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// indirect buffer per-triangle record
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          coordinateAttributePipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		if(enabledFeatures.multiDrawIndirect)
			cb.drawIndirect(indirectBuffer.get(),  // buffer
			                0,  // offset
			                numTriangles,  // drawCount
			                sizeof(vk::DrawIndirectCommand));  // stride
		else
			tests[timestampIndex/2].enabled=false;
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// per-triangle drawCmd in command buffer, attributeless constant output test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          attributelessConstantOutputPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0; i<numTriangles; i++)
			cb.draw(3,1,numTriangles*3,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// per-triangle drawCmd test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          coordinateAttributePipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0; i<numTriangles; i++)
			cb.draw(3,1,i*3,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// coordinate attribute test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          coordinateAttributePipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// coordinates in buffer test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          coordinateBufferPipeline.get(),oneBufferPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ coordinateBufferDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// singleUniformMatrix test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          singleMatrixUniformPipeline.get(),oneUniformVSPipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>{ oneUniformVSDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// matrixBuffer test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          matrixBufferPipeline.get(),oneBufferPipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get() },
		          vector<vk::DescriptorSet>{ sameMatrixBufferDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// matrixAttribute test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          matrixAttributePipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get(), transformationMatrixAttribute.get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3,numTriangles/2,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.draw(3,numTriangles/2,3,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// two attributes test, no transformation
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          twoAttributesPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get(),vec4Attributes[0].get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// two packed attributes test, no transformation
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          twoPackedAttributesPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// two packed buffers test, no transformation
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          twoPackedBuffersPipeline.get(),twoBuffersPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ twoBuffersDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// two packed buffers using struct test, no transformation
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          twoPackedBuffersUsingStructPipeline.get(),twoBuffersPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ twoBuffersDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// two packed buffers using struct slow test, no transformation
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          twoPackedBuffersUsingStructSlowPipeline.get(),twoBuffersPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ twoBuffersDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// single packed buffer test, no transformations
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          singlePackedBufferPipeline.get(),oneBufferPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ singlePackedBufferDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// two packed attributes test, single matrix
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          twoPackedAttributesAndSingleMatrixPipeline.get(),oneBufferPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ sameMatrixBufferDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// two packed attributes test, one matrix
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          twoPackedAttributesAndMatrixPipeline.get(),oneBufferPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ sameMatrixBufferDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// two packed buffers test, one matrix
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          twoPackedBuffersAndMatrixPipeline.get(),threeBuffersPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ threeBuffersDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// geometry shader two packed buffers test, one matrix
		if(enabledFeatures.geometryShader) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          geometryShaderPipeline.get(),threeBuffersInGSPipelineLayout.get(),
			          vector<vk::Buffer>(),
			          vector<vk::DescriptorSet>{ threeBuffersInGSDescriptorSet });
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		}
		else {
			tests[timestampIndex/2].enabled=false;
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
		}

		// four attributes (two4F32+two4U8) test, no transformation
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          two4F32Two4U8AttributesPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get(),vec4Attributes[0].get(),
		                              vec4u8Attributes[0].get(),vec4u8Attributes[1].get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// four attributes test, no transformation
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fourAttributesPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get(),vec4Attributes[0].get(),
		                              vec4Attributes[1].get(),vec4Attributes[2].get() },
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// four attributes test, per-triangle transformation
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fourAttributesAndMatrixPipeline.get(),oneBufferPipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get(),vec4Attributes[0].get(),
		                              vec4Attributes[1].get(),vec4Attributes[2].get() },
		          vector<vk::DescriptorSet>{ sameMatrixBufferDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// transformation 3 matrices test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          transformationThreeMatricesPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationThreeMatricesDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// transformation 5 matrices test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          transformationFiveMatricesPipeline.get(),twoBuffersAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationFiveMatricesDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// transformation 5 matrices test using geometry shader and attributes
		if(enabledFeatures.geometryShader) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          transformationFiveMatricesUsingGSAndAttributesPipeline.get(),twoBuffersAndUniformInGSPipelineLayout.get(),
			          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
			          vector<vk::DescriptorSet>{ transformationFiveMatricesUsingGSAndAttributesDescriptorSet });
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		} else {
			tests[timestampIndex/2].enabled=false;
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
		}

		// transformation 5 matrices test using geometry shader
		if(enabledFeatures.geometryShader) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          transformationFiveMatricesUsingGSPipeline.get(),fourBuffersAndUniformInGSPipelineLayout.get(),
			          vector<vk::Buffer>(),
			          vector<vk::DescriptorSet>{ transformationFiveMatricesUsingGSDescriptorSet });
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		} else {
			tests[timestampIndex/2].enabled=false;
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
		}

		// textured Phong four attribute five matrices test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedFourAttributesFiveMatricesPipeline.get(),twoBuffersAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get(),normalAttribute.get(),
		                              colorAttribute.get(),texCoordAttribute.get() },
		          vector<vk::DescriptorSet>{ transformationFiveMatricesDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// textured Phong four attribute test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedFourAttributesPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ coordinateAttribute.get(),normalAttribute.get(),
		                              colorAttribute.get(),texCoordAttribute.get() },
		          vector<vk::DescriptorSet>{ transformationThreeMatricesDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// textured Phong test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationThreeMatricesDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// textured Phong RowMajor matrix test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedRowMajorPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationThreeMatricesRowMajorDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// textured Phong Mat4x3 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedMat4x3Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationThreeMatrices4x3DescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// textured Phong Mat4x3 RowMajor test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedMat4x3RowMajorPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationThreeMatrices4x3RowMajorDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// textured Phong Quat1 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedQuat1Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndPATDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// textured Phong Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndPATDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// textured Phong Quat3 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedQuat3Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndPATDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// indexed textured Phong Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndPATDescriptorSet });
		cb.bindIndexBuffer(indexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(3*numTriangles,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// primitive restart indexed textured Phong Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndPATDescriptorSet });
		cb.bindIndexBuffer(primitiveRestartIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(4*numTriangles,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// textured Phong with double precision matrices on input but computation in standard floats test
		if(enabledFeatures.shaderFloat64) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          phongTexturedDMatricesOnlyInputPipeline.get(),bufferAndUniformPipelineLayout.get(),
			          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
			          vector<vk::DescriptorSet>{ transformationThreeDMatricesDescriptorSet });
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		} else {
			tests[timestampIndex/2].enabled=false;
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
		}

		// textured Phong with double precision matrices test
		if(enabledFeatures.shaderFloat64) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          phongTexturedDMatricesPipeline.get(),bufferAndUniformPipelineLayout.get(),
			          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
			          vector<vk::DescriptorSet>{ transformationThreeDMatricesDescriptorSet });
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		} else {
			tests[timestampIndex/2].enabled=false;
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
		}

		// textured Phong with double precision matrices and vertices test
		if(enabledFeatures.shaderFloat64) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          phongTexturedDMatricesDVerticesPipeline.get(),bufferAndUniformPipelineLayout.get(),
			          vector<vk::Buffer>{ packedDAttribute1.get(),packedDAttribute2.get(),packedDAttribute3.get() },
			          vector<vk::DescriptorSet>{ transformationThreeDMatricesDescriptorSet });
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		} else {
			tests[timestampIndex/2].enabled=false;
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
		}

		// textured Phong with double precision matrices and vertices in GS test
		if(enabledFeatures.shaderFloat64 && enabledFeatures.geometryShader) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
					  phongTexturedInGSDMatricesDVerticesPipeline.get(),bufferAndUniformInGSPipelineLayout.get(),
					  vector<vk::Buffer>{ packedDAttribute1.get(),packedDAttribute2.get(),packedDAttribute3.get() },
					  vector<vk::DescriptorSet>{ phongTexturedThreeDMatricesUsingGSAndAttributesDescriptorSet });
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		} else {
			tests[timestampIndex/2].enabled=false;
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
		}

		// textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// indexed textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(indexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(3*numTriangles,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// primitive restart indexed textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ packedAttribute1.get(),packedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(primitiveRestartIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(4*numTriangles,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// shared vertices textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ sharedVertexPackedAttribute1.get(),sharedVertexPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// 1 to 125 triangle call, shared vertices textured Phong single Quat2 test
		for(uint32_t n:array<uint32_t,11>{1,2,5,8,10,20,25,40,50,100,125}) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          phongTexturedSingleQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
			          vector<vk::Buffer>{ sharedVertexPackedAttribute1.get(),sharedVertexPackedAttribute2.get() },
			          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			for(uint32_t i=0,e=3*numTriangles; i<e; i+=n*3)
				cb.draw(min(n*3,e-i),1,i,0);  // vertexCount,instanceCount,firstVertex,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		}

		// indexed shared vertices textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(stripIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*3*triStripLength; i<e; i+=3*triStripLength)
			cb.drawIndexed(3*triStripLength,1,i,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// 1 to 125 triangle call, indexed shared vertices textured Phong single Quat2 test
		for(uint32_t n:array<uint32_t,12>{1,2,5,8,10,20,25,40,50,64,100,125}) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          phongTexturedSingleQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
			          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
			          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
			cb.bindIndexBuffer(stripIndexBuffer.get(),0,vk::IndexType::eUint32);
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			for(uint32_t i=0,e=3*numTriangles; i<e; i+=n*3)
				cb.drawIndexed(min(n*3,e-i),1,i,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		}

		// tri-strip no-index textured Phong Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2TriStripPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*(2+triStripLength); i<e; i+=2+triStripLength)
			cb.draw(2+triStripLength,1,i,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// 1 to 125 triangles tri-strip no-index textured Phong Quat2 test
		for(uint32_t n:array<uint32_t,11>{1,2,5,8,10,20,25,40,50,100,125}) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          phongTexturedSingleQuat2TriStripPipeline.get(),bufferAndUniformPipelineLayout.get(),
			          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
			          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			for(uint32_t i=0,e=(numTriangles/triStripLength)*(2+triStripLength); i<e; i+=2+triStripLength)
				for(uint32_t j=i,je=j+triStripLength; j<je; j+=n)
					cb.draw(n+2,1,j,0);  // vertexCount,instanceCount,firstVertex,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		}

		// tri-strip indexed textured Phong Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2TriStripPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(indexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*(2+triStripLength); i<e; i+=2+triStripLength)
			cb.drawIndexed(2+triStripLength,1,i,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// 1 to 125-triangles drawCmd indexed tri-strip, textured Phong Quat2 test
		for(uint32_t n:array<uint32_t,11>{1,2,5,8,10,20,25,40,50,100,125}) {
			beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
			          phongTexturedSingleQuat2TriStripPipeline.get(),bufferAndUniformPipelineLayout.get(),
			          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
			          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
			cb.bindIndexBuffer(indexBuffer.get(),0,vk::IndexType::eUint32);
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			for(uint32_t i=0,e=(numTriangles/triStripLength)*(2+triStripLength); i<e; i+=2+triStripLength)
				for(uint32_t j=i,je=j+triStripLength; j<je; j+=n)
					cb.drawIndexed(n+2,1,j,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
			cb.writeTimestamp(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
				timestampPool.get(),  // queryPool
				timestampIndex++      // query
			);
			cb.endRenderPass();
		}

		// 1-drawCmd, primitive restart after whole strip of 1000 indexed shared vertices, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(stripPrimitiveRestartIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed((triStripLength+3)*(numTriangles/triStripLength),1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// 1-drawCmd per strip, primitive restart after whole strip of 1000 indexed shared vertices, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(stripPrimitiveRestartIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*(triStripLength+3); i<e; i+=triStripLength+3)
			cb.drawIndexed(triStripLength+3,1,i,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// primitive restart after 1 triangle (3 indexed shared vertices), textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(stripPrimitiveRestart3IndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*4*triStripLength; i<e; i+=4*triStripLength)
			cb.drawIndexed(4*triStripLength,1,i,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// primitive restart after 2 triangles (4 indexed shared vertices), textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(stripPrimitiveRestart4IndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*5*triStripLength/2; i<e; i+=5*triStripLength/2)
			cb.drawIndexed(5*triStripLength/2,1,i,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// primitive restart after 5 triangles (7 indexed shared vertices), textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(stripPrimitiveRestart7IndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*8*triStripLength/5; i<e; i+=8*triStripLength/5)
			cb.drawIndexed(8*triStripLength/5,1,i,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// primitive restart after 8 triangles (10 indexed shared vertices), textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(stripPrimitiveRestart10IndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*11*triStripLength/8; i<e; i+=11*triStripLength/8)
			cb.drawIndexed(11*triStripLength/8,1,i,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// two -1 after each three triangle indices, primitive restart shared vertices, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(primitiveRestartMinusOne2IndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*5*triStripLength; i<e; i+=5*triStripLength)
			cb.drawIndexed(5*triStripLength,1,i,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// five -1 after each three triangle indices, primitive restart shared vertices, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(primitiveRestartMinusOne5IndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*8*triStripLength; i<e; i+=8*triStripLength)
			cb.drawIndexed(8*triStripLength,1,i,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// all -1 rendered as numTriangles+2 tri-strip, primitive restart, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(minusOneIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(numTriangles+2,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// all 0 rendered numTriangles+2 tri-strip, primitive restart, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(zeroIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(numTriangles+2,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// all 1 rendered numTriangles+2 tri-strip, primitive restart, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2PrimitiveRestartPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(plusOneIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(numTriangles+2,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// all 0 rendered as numTriangles+2 tri-strip, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2TriStripPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(zeroIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(numTriangles+2,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// all 1 rendered as numTriangles+2 tri-strip, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2TriStripPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(plusOneIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(numTriangles+2,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// all 0 rendered as tri-list, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(zeroIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(3*numTriangles,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// all 1 rendered as tri-list, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ stripPackedAttribute1.get(),stripPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.bindIndexBuffer(plusOneIndexBuffer.get(),0,vk::IndexType::eUint32);
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.drawIndexed(3*numTriangles,1,0,0,0);  // indexCount,instanceCount,firstIndex,vertexOffset,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// the same vertex rendered as numTriangles+2 tri-strip, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2TriStripPipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ sameVertexPackedAttribute1.get(),sameVertexPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		for(uint32_t i=0,e=(numTriangles/triStripLength)*(2+triStripLength); i<e; i+=2+triStripLength)
			cb.draw(2+triStripLength,1,i,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// the same vertex rendered as tri-list, textured Phong single Quat2 test
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongTexturedSingleQuat2Pipeline.get(),bufferAndUniformPipelineLayout.get(),
		          vector<vk::Buffer>{ sameVertexPackedAttribute1.get(),sameVertexPackedAttribute2.get() },
		          vector<vk::DescriptorSet>{ transformationTwoMatricesAndSinglePATDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(3*numTriangles,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate constant color test, 1x whole screen
		tests[timestampIndex/2].numRenderedItems=1.;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fillrateContantColorPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,1,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate constant color test, 10x whole screen
		tests[timestampIndex/2].numRenderedItems=numFullscreenQuads;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fillrateContantColorPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,numFullscreenQuads,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate four smooth interpolators test
		tests[timestampIndex/2].numRenderedItems=numFullscreenQuads;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fillrateFourSmoothInterpolatorsPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,numFullscreenQuads,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate four flat interpolators test
		tests[timestampIndex/2].numRenderedItems=numFullscreenQuads;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fillrateFourFlatInterpolatorsPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,numFullscreenQuads,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate textured Phong interpolators test
		tests[timestampIndex/2].numRenderedItems=numFullscreenQuads;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fillrateTexturedPhongInterpolatorsPipeline.get(),simplePipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>());
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,numFullscreenQuads,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate textured Phong test
		tests[timestampIndex/2].numRenderedItems=numFullscreenQuads;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fillrateTexturedPhongPipeline.get(),phongTexturedPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ phongTexturedDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,numFullscreenQuads,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate textured Phong not packed uniform test
		tests[timestampIndex/2].numRenderedItems=numFullscreenQuads;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fillrateTexturedPhongNotPackedPipeline.get(),phongTexturedPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ phongTexturedNotPackedDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,numFullscreenQuads,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate uniform color4f test
		tests[timestampIndex/2].numRenderedItems=numFullscreenQuads;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fillrateUniformColor4fPipeline.get(),oneUniformFSPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ one4fUniformFSDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,numFullscreenQuads,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate uniform color4b test
		tests[timestampIndex/2].numRenderedItems=numFullscreenQuads;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          fillrateUniformColor4bPipeline.get(),oneUniformFSPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ one4bUniformFSDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,numFullscreenQuads,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate phong without specular computations test
		tests[timestampIndex/2].numRenderedItems=numFullscreenQuads;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongNoSpecularPipeline.get(),threeUniformFSPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ threeUniformFSDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,numFullscreenQuads,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// fillrate phong without specular computations, single uniform test
		tests[timestampIndex/2].numRenderedItems=numFullscreenQuads;
		beginTest(cb,framebuffers[i].get(),currentSurfaceExtent,
		          phongNoSpecularSingleUniformPipeline.get(),oneUniformFSPipelineLayout.get(),
		          vector<vk::Buffer>(),
		          vector<vk::DescriptorSet>{ allInOneLightingUniformDescriptorSet });
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eTopOfPipe,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.draw(4,numFullscreenQuads,0,0);  // vertexCount,instanceCount,firstVertex,firstInstance
		cb.writeTimestamp(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,  // pipelineStage
			timestampPool.get(),  // queryPool
			timestampIndex++      // query
		);
		cb.endRenderPass();

		// end command buffer
		cb.end();
		assert(timestampIndex==tests.size()*2 && "Number of timestamps and number of tests mismatch.");
	}
}


/// Queue one frame for rendering
static bool queueFrame()
{
	// acquire next image
	uint32_t imageIndex;
	vk::Result r=
		device->acquireNextImageKHR(
			swapchain.get(),                  // swapchain
			numeric_limits<uint64_t>::max(),  // timeout
			imageAvailableSemaphore.get(),    // semaphore to signal
			vk::Fence(nullptr),               // fence to signal
			&imageIndex                       // pImageIndex
		);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR) { needResize=true; return false; }
		else  vk::throwResultException(r,VULKAN_HPP_NAMESPACE_STRING"::Device::acquireNextImageKHR");
	}

	// submit work
	graphicsQueue.submit(
		vk::SubmitInfo(
			1,&imageAvailableSemaphore.get(),     // waitSemaphoreCount+pWaitSemaphores
			&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // pWaitDstStageMask
			1,&commandBuffers[imageIndex].get(),  // commandBufferCount+pCommandBuffers
			1,&renderFinishedSemaphore.get()      // signalSemaphoreCount+pSignalSemaphores
		),
		vk::Fence(nullptr)
	);

	// submit image for presentation
	r=presentationQueue.presentKHR(
		&(const vk::PresentInfoKHR&)vk::PresentInfoKHR(
			1,&renderFinishedSemaphore.get(),  // waitSemaphoreCount+pWaitSemaphores
			1,&swapchain.get(),&imageIndex,    // swapchainCount+pSwapchains+pImageIndices
			nullptr                            // pResults
		)
	);
	if(r!=vk::Result::eSuccess) {
		if(r==vk::Result::eErrorOutOfDateKHR||r==vk::Result::eSuboptimalKHR) { needResize=true; return false; }
		else  vk::throwResultException(r,VULKAN_HPP_NAMESPACE_STRING"::Queue::presentKHR");
	}

	// return success
	return true;
}


static void testMemoryAllocationPerformance(vk::BufferCreateFlags bufferFlags,unsigned virtualSpaceMultiplier)
{
	auto test=
		[](size_t numBuffers,size_t numMemoryObjectsPerBuffer,size_t memoryObjectSize,unsigned virtualSpaceMultiplier,
		   vk::BufferCreateFlags bufferFlags)->tuple<double,double,double> {

			// create buffers
			vector<vk::UniqueBuffer> bufferList;
			bufferList.reserve(numBuffers);
			auto startTime=chrono::high_resolution_clock::now();
			for(size_t i=0; i<numBuffers; i++)
				bufferList.emplace_back(
					device->createBufferUnique(
						vk::BufferCreateInfo(
							bufferFlags,                  // flags
							numMemoryObjectsPerBuffer*memoryObjectSize*virtualSpaceMultiplier,  // size
							vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,  // usage
							vk::SharingMode::eExclusive,  // sharingMode
							0,                            // queueFamilyIndexCount
							nullptr                       // pQueueFamilyIndices
						)
					)
				);
			double bufferCreationTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();

			// allocate memory
			vector<vk::UniqueDeviceMemory> memoryList;
			memoryList.reserve(numBuffers*numMemoryObjectsPerBuffer);
			startTime=chrono::high_resolution_clock::now();
			for(size_t i=0; i<numBuffers; i++)
				for(size_t j=0; j<numMemoryObjectsPerBuffer; j++)
					memoryList.emplace_back(
						get<0>(
							allocateMemory(
								bufferList[i].get(),
								vk::MemoryPropertyFlagBits::eDeviceLocal,
								numMemoryObjectsPerBuffer*virtualSpaceMultiplier
							)
						)
					);
			double memoryCreationTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();

			double bindTime;
			if(!(bufferFlags&vk::BufferCreateFlagBits::eSparseBinding)) {

				// bind non-sparse memory
				assert(numMemoryObjectsPerBuffer==1 && "Non-sparse buffers support only one attached DeviceMemory object.");

				startTime=chrono::high_resolution_clock::now();
				for(size_t i=0; i<numBuffers; i++)
					device->bindBufferMemory(
						bufferList[i].get(),  // buffer
						memoryList[i].get(),  // memory
						0  // memoryOffset
					);
				bindTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();
			}
			else {

				// fill SparseMemoryBind structures
				vector<vk::SparseMemoryBind> memoryBinds;
				memoryBinds.reserve(numBuffers*numMemoryObjectsPerBuffer);
				for(size_t i=0,idx=0; i<numBuffers; i++)
					for(size_t j=0; j<numMemoryObjectsPerBuffer; j++,idx++)
						memoryBinds.emplace_back(
							j*memoryObjectSize,  // resourceOffset
							memoryObjectSize,  // size
							memoryList[idx].get(),  // memory
							0,  // memoryOffset
							vk::SparseMemoryBindFlags()  // flags
						);

				// fill SparseBufferMemoryBindInfo structures
				vector<vk::SparseBufferMemoryBindInfo> bufferBinds;
				bufferBinds.reserve(numBuffers);
				for(size_t i=0; i<numBuffers; i++)
					bufferBinds.emplace_back(
						bufferList[i].get(),  // buffer
						uint32_t(numMemoryObjectsPerBuffer),  // bindCount
						memoryBinds.data()+(i*numMemoryObjectsPerBuffer)  // pBinds
					);

				// call bindSparse
				startTime=chrono::high_resolution_clock::now();
				sparseQueue.bindSparse(
					vk::BindSparseInfo(
						nullptr,  // waitSemaphores
						bufferBinds,  // bufferBinds
						nullptr,  // imageOpaqueBinds
						nullptr,  // imageBinds
						nullptr  // signalSemaphores
					),
					vk::Fence()
				);
				sparseQueue.waitIdle();
				bindTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();
			}

			// return time in seconds as double
			return make_tuple(bufferCreationTime,memoryCreationTime,bindTime);
		};

	// perform test
	constexpr unsigned repeatCount=3;
	auto testData=
		(bufferFlags&vk::BufferCreateFlagBits::eSparseBinding)
			? vector{
				// numBuffers, memObjPerBuffer, numMemBlocksPerMemObj
				tuple{1,1,   1}, // to warm up caches, will be not included in results
				tuple{1,1,   1}, // to warm up caches, will be not included in results
				tuple{1,1,   1},
				tuple{1,1,   2},
				tuple{1,1,  32},
				tuple{1,1, 128},
				tuple{1,1, 512},
				tuple{1,1,1024},
				tuple{1,2, 512},
				tuple{1,32, 32},
				tuple{1,512, 2},
				tuple{1,1024,1},
				tuple{2,1, 512},
				tuple{32,1, 32},
				tuple{512,1, 2},
				tuple{1024,1,1},
				tuple{512,2, 1},
				tuple{32,32, 1},
				tuple{8,8,  16},
			}
			: vector{
				tuple{1,1,   1}, // to warm up caches, will be not included in results
				tuple{1,1,   1}, // to warm up caches, will be not included in results
				tuple{1,1,   1},
				tuple{1,1,   2},
				tuple{1,1,  32},
				tuple{1,1, 512},
				tuple{1,1,1024},
				tuple{32,1, 32},
				tuple{1024,1,1},
			};
	vector<double> bufferCreationResults(testData.size(),numeric_limits<double>::max());
	vector<double> memoryCreationResults(testData.size(),numeric_limits<double>::max());
	vector<double> bindResults(testData.size(),numeric_limits<double>::max());
	auto startTime=chrono::high_resolution_clock::now();
	for(size_t j=0; j<repeatCount; j++)
		for(size_t i=0; i<testData.size(); i++) {
			auto& d=testData[i];
			auto [bufferCreationTime,memoryCreationTime,bindTime]=
				test(get<0>(d),get<1>(d),get<2>(d)*sparseBlockSize,virtualSpaceMultiplier,bufferFlags);
			bufferCreationResults[i]=min(bufferCreationTime,bufferCreationResults[i]);
			memoryCreationResults[i]=min(memoryCreationTime,memoryCreationResults[i]);
			bindResults[i]=min(bindTime,bindResults[i]);
		}
	double totalMeasurementTime=chrono::duration<double>(chrono::high_resolution_clock::now()-startTime).count();

	auto printTime=
		[](double t) -> string {
			stringstream ss;
			ss<<setprecision(3);
			if(t>=1)
				ss<<t<<"s";
			else if(t>=1e-3)
				ss<<t*1e3<<"ms";
			else if(t>=1e-6)
				ss<<t*1e6<<"us";
			else
				ss<<t*1e9<<"ns";
			return ss.str();
		};
	auto printAmountOfMemory=
		[](size_t value) -> string {
			stringstream ss;
			ss<<setprecision(3);
			if(value<1000)
				ss<<value<<"B";
			else {
				float f=float(value)/1024;
				if(f<1000.f)
					ss<<f<<"KiB";
				else {
					f/=1024;
					if(f<1000.f)
						ss<<f<<"MiB";
					else {
						f/=1024;
						if(f<1000.f)
							ss<<f<<"GiB";
						else
							ss<<f/1024<<"TiB";
					}
				}
			}
			return ss.str();
		};


	// print results
#if 1
	cout<<"  number of      total   one   costOfMem  one   costOfMem  one    memory  total"<<endl;
	cout<<"  buffers/       memory buffer  block on memObj  block on buffer  block   cost"<<endl;
	cout<<"  memObjPerBuf/  alloc- create  buffer   create  memObj    bind   bind    ofMem"<<endl;
	cout<<"  memBlocksPerObj ated   time  creation   time  creation   time   time    block"<<endl;
	for(size_t i=2; i<testData.size(); i++) {
		size_t numBuffers=get<0>(testData[i]);
		size_t numMemObjPerBuffer=get<1>(testData[i]);
		size_t numMemBlocksPerMemObj=get<2>(testData[i]);
		cout<<setw(4)<<numBuffers<<" "
		    <<setw(4)<<numMemObjPerBuffer<<" "
			<<setw(4)<<numMemBlocksPerMemObj<<" "
			<<setw(7)<<printAmountOfMemory(numBuffers*numMemObjPerBuffer*numMemBlocksPerMemObj*sparseBlockSize)<<" "
			<<setw(7)<<printTime(bufferCreationResults[i]/numBuffers)<<" "
			<<setw(7)<<printTime(bufferCreationResults[i]/(numBuffers*numMemObjPerBuffer*numMemBlocksPerMemObj))<<"  "
			<<setw(7)<<printTime(memoryCreationResults[i]/(numBuffers*numMemObjPerBuffer))<<" "
			<<setw(7)<<printTime(memoryCreationResults[i]/(numBuffers*numMemObjPerBuffer*numMemBlocksPerMemObj))<<" "
			<<setw(7)<<printTime(bindResults[i]/numBuffers)<<" "
			<<setw(7)<<printTime(bindResults[i]/(numBuffers*numMemObjPerBuffer*numMemBlocksPerMemObj))<<" "
			<<setw(7)<<printTime((bufferCreationResults[i]+memoryCreationResults[i]+bindResults[i])/
			                     (numBuffers*numMemObjPerBuffer*numMemBlocksPerMemObj))<<endl;
	}
#else
	cout<<"  num  numMem  mem    time of   costOfMem  time of  costOfMem  buffer  memory"<<endl;
	cout<<"   of  ObjPer blocks  a buffer   block on   memObj  block on    bind   block"<<endl;
	cout<<"  bufs buffer perObj  creation  bufCreate  creation memCreate   time  bindTime"<<endl;
	for(size_t i=2; i<testData.size(); i++) {
		size_t numBuffers=get<0>(testData[i]);
		size_t numMemObjPerBuffer=get<1>(testData[i]);
		size_t numMemBlocksPerMemObj=get<2>(testData[i]);
		cout<<setw(5)<<numBuffers<<"  "
		    <<setw(5)<<numMemObjPerBuffer<<"  "
			<<setw(5)<<numMemBlocksPerMemObj<<"   "
			<<setw(7)<<printTime(bufferCreationResults[i]/numBuffers)<<"  "
			<<setw(8)<<printTime(bufferCreationResults[i]/(numBuffers*numMemObjPerBuffer*numMemBlocksPerMemObj))<<"    "
			<<setw(7)<<printTime(memoryCreationResults[i]/(numBuffers*numMemObjPerBuffer))<<"  "
			<<setw(7)<<printTime(memoryCreationResults[i]/(numBuffers*numMemObjPerBuffer*numMemBlocksPerMemObj))<<"    "
			<<setw(7)<<printTime(bindResults[i]/numBuffers)<<"  "
			<<setw(7)<<printTime(bindResults[i]/(numBuffers*numMemObjPerBuffer*numMemBlocksPerMemObj))<<endl;
	}
#endif
	cout<<"Total measurement time: "<<printTime(totalMeasurementTime)<<endl;
}


/// main function of the application
int main(int argc,char** argv)
{
	// catch exceptions
	// (vulkan.hpp fuctions throws if they fail)
	try {

		// parse command line
		size_t physicalDeviceIndex=0;
		for(int i=1; i<argc; i++) {
			if(argv[i]==nullptr || argv[i][0]==0)
				continue;
			if(argv[i][0]=='-') {
				if(strcmp(argv[i],"--sparse-none")==0)  sparseMode=SPARSE_NONE; else
				if(strcmp(argv[i],"--sparse-binding")==0)  sparseMode=SPARSE_BINDING; else
				if(strcmp(argv[i],"--sparse-residency")==0)  sparseMode=SPARSE_RESIDENCY; else
				if(strcmp(argv[i],"--sparse-residency-aliased")==0)  sparseMode=SPARSE_RESIDENCY_ALIASED;
				else {
					cout<<"Invalid argument: "<<argv[i]<<endl;
					exit(99);
				}
			}
			else
				physicalDeviceIndex=size_t(max(atoi(argv[1]),0));
		}

		// init Vulkan and open window,
		// give physical device index as parameter
		init(physicalDeviceIndex);

		auto startTime=chrono::steady_clock::now();

#ifdef _WIN32

		// run Win32 event loop
		MSG msg;
		while(true){

			// process messages
			while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)>0) {
				if(msg.message==WM_QUIT)
					goto ExitMainLoop;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

#else

		// run Xlib event loop
		XEvent e;
		while(true) {

			// process messages
			while(XPending(display)>0) {
				XNextEvent(display,&e);
				if(e.type==ConfigureNotify && e.xconfigure.window==window) {
					vk::Extent2D newSize(e.xconfigure.width,e.xconfigure.height);
					if(newSize!=windowSize) {
						needResize=true;
						windowSize=newSize;
					}
					continue;
				}
				if(e.type==ClientMessage && ulong(e.xclient.data.l[0])==wmDeleteMessage)
					goto ExitMainLoop;
			}

#endif

			// recreate swapchain if necessary
			if(needResize) {

				// recreate only upon surface extent change
				vk::SurfaceCapabilitiesKHR surfaceCapabilities=physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
				if(surfaceCapabilities.currentExtent!=currentSurfaceExtent) {

					// avoid 0,0 surface extent as creation of swapchain would fail
					// (0,0 is returned on some platforms (particularly Windows) when window is minimized)
					if(surfaceCapabilities.currentExtent.width==0 || surfaceCapabilities.currentExtent.height==0)
						continue;

					// new currentSurfaceExtent
					currentSurfaceExtent=
						(surfaceCapabilities.currentExtent.width!=std::numeric_limits<uint32_t>::max())
							?surfaceCapabilities.currentExtent
							:vk::Extent2D{max(min(windowSize.width,surfaceCapabilities.maxImageExtent.width),surfaceCapabilities.minImageExtent.width),
										  max(min(windowSize.height,surfaceCapabilities.maxImageExtent.height),surfaceCapabilities.minImageExtent.height)};

					// recreate swapchain
					recreateSwapchainAndPipeline();
				}

				// restart measurement
				startTime=chrono::steady_clock::now();

				needResize=false;
			}

			// queue frame
			if(!queueFrame())
				continue;

			// wait for rendering to complete
			presentationQueue.waitIdle();
			graphicsQueue.waitIdle();

			// read timestamps
			vector<uint64_t> timestamps(tests.size()*2);
			vk::Result r=device->getQueryPoolResults(
				timestampPool.get(),  // queryPool
				0,                    // firstQuery
				uint32_t(tests.size())*2,  // queryCount
				tests.size()*2*sizeof(uint64_t),  // dataSize
				timestamps.data(),    // pData
				sizeof(uint64_t),     // stride
				vk::QueryResultFlagBits::e64|vk::QueryResultFlagBits::eWait  // flags
			);
			if(r!=vk::Result::eSuccess)
				throw std::runtime_error("vkGetQueryPoolResults() did not finish with VK_SUCCESS result.");
			size_t i=0;
			for(Test& t : tests) {
				if(t.enabled)
					t.renderingTimes.emplace_back(timestamps[i+1]-timestamps[i]);
				else
					// neutralize invalid timestamps of disabled tests
					// e.g. avoid "run in parallel" error
					if(i==0)  timestamps[0]=timestamps[1]=0;
					else  timestamps[i]=timestamps[i+1]=timestamps[i-1];
				i+=2;
			}
			if(!is_sorted(timestamps.begin(),timestamps.end()))
				throw std::runtime_error("Tests ran in parallel.");

			// print the result at the end
			double totalMeasurementTime=chrono::duration<double>(chrono::steady_clock::now()-startTime).count();
			if(totalMeasurementTime>2.) {
				cout<<"Triangle throughput:"<<endl;
				for(Test& t : tests)
					if(t.type==Test::Type::VertexThroughput) {
						if(t.enabled) {
							sort(t.renderingTimes.begin(),t.renderingTimes.end());
							double time_ns=t.renderingTimes[(t.renderingTimes.size()-1)/2]*timestampPeriod_ns;
							cout<<"   "<<t.resultString<<": "<<double(numTriangles)/time_ns*1e9/1e6<<" millions per second"<<endl;
						}
						else
							cout<<"   "<<t.resultString<<": not supported"<<endl;
					}
				cout<<"Fragment throughput:"<<endl;
				size_t numScreenFragments=size_t(currentSurfaceExtent.width)*currentSurfaceExtent.height;
				for(Test& t : tests)
					if(t.type==Test::Type::FragmentThroughput) {
						if(t.enabled) {
							sort(t.renderingTimes.begin(),t.renderingTimes.end());
							double time_ns=t.renderingTimes[(t.renderingTimes.size()-1)/2]*timestampPeriod_ns;
							cout<<"   "<<t.resultString<<": "<<double(numScreenFragments)*t.numRenderedItems/time_ns*1e9/1e9<<" * 1e9 per second"<<endl;
						#if 0 // tuning of tests to not take too long
							cout<<"      measurement time: "<<time_ns/1e6<<"ms"<<endl;
						#endif
						}
						else
							cout<<"   "<<t.resultString<<": not supported"<<endl;
					}
				cout<<"Number of measurements of each test: "<<tests.front().renderingTimes.size()<<endl;
				cout<<"Total time of all measurements: "<<totalMeasurementTime<<" seconds"<<endl;
				cout<<endl;

				// device with sparse memory support
				vk::UniqueDevice sparseDevice;
				vk::PhysicalDeviceFeatures physicalFeatures=physicalDevice.getFeatures();
				if(physicalFeatures.sparseBinding) {
					vk::PhysicalDeviceFeatures enabledFeatures;
					enabledFeatures.setSparseBinding(true);
					sparseDevice=
						physicalDevice.createDeviceUnique(
							vk::DeviceCreateInfo{
								vk::DeviceCreateFlags(),  // flags
								(presentationQueueFamily==graphicsQueueFamily && sparseQueueFamily==graphicsQueueFamily)  // queueCreateInfoCount
									?uint32_t(1):uint32_t(3),
								array<const vk::DeviceQueueCreateInfo,3>{  // pQueueCreateInfos
									vk::DeviceQueueCreateInfo{
										vk::DeviceQueueCreateFlags(),
										graphicsQueueFamily,
										1,
										&(const float&)1.f,
									},
									vk::DeviceQueueCreateInfo{
										vk::DeviceQueueCreateFlags(),
										presentationQueueFamily,
										1,
										&(const float&)1.f,
									},
									vk::DeviceQueueCreateInfo{
										vk::DeviceQueueCreateFlags(),
										sparseQueueFamily,
										1,
										&(const float&)1.f,
									},
								}.data(),
								0,nullptr,  // no layers
								1,          // number of enabled extensions
								array<const char*,1>{"VK_KHR_swapchain"}.data(),  // enabled extension names
								&enabledFeatures,  // enabled features
							}
						);
				}

				// print memory alignment
				cout<<"Graphics memory allocation properties"<<endl;
				cout<<"Standard buffer alignment:  "
					<<getMemoryAlignment(device.get(),1,vk::BufferCreateFlags())<<endl;

				// set sparse block variables
				if(sparseDevice) {
					sparseBlockSize=getMemoryAlignment(sparseDevice.get(),1,vk::BufferCreateFlagBits::eSparseBinding);
					memoryBlockSize=sparseBlockSize;
					memoryBlockMask=~(sparseBlockSize-1);
					cout<<"Graphics memory page size:  "<<sparseBlockSize<<endl;
				}
				else {
					cout<<"Graphics memory page size: Could not be retrieved."<<endl;
					cout<<"   No sparse memory support. Further measurements will report\n"
					      "   results like the graphics memory page size would be 64KiB."<<endl;
					sparseBlockSize=65536;
					memoryBlockSize=65536;
					memoryBlockSize=~0xffff;
				}

				// sparse properties
				if(sparseDevice) {
					vk::PhysicalDeviceFeatures physicalFeatures=physicalDevice.getFeatures();
					cout<<"Sparse address space:       0x"<<std::hex<<physicalDevice.getProperties().limits.sparseAddressSpaceSize<<std::dec<<
						" ("<<physicalDevice.getProperties().limits.sparseAddressSpaceSize/1024/1024/1024<<" GiB)"<<endl;
					cout<<"Sparse binding:             "<<physicalFeatures.sparseBinding<<endl;
					cout<<"Sparse residency buffer:    "<<physicalFeatures.sparseResidencyBuffer<<endl;
					cout<<"Sparse residency image2D:   "<<physicalFeatures.sparseResidencyImage2D<<endl;
					cout<<"Sparse residency 4samples:  "<<physicalFeatures.sparseResidency4Samples<<endl;
					cout<<"Sparse residency aliased:   "<<physicalFeatures.sparseResidencyAliased<<endl;
					cout<<"Sparse buffer page size:    "<<getMemoryAlignment(sparseDevice.get(),1,vk::BufferCreateFlagBits::eSparseBinding|vk::BufferCreateFlagBits::eSparseResidency|vk::BufferCreateFlagBits::eSparseAliased)<<endl;
					vk::SparseImageFormatProperties imageFormatProperties=
						[]() {
							auto l=physicalDevice.getSparseImageFormatProperties(vk::Format::eR8G8B8A8Uint,vk::ImageType::e2D,vk::SampleCountFlagBits::e1,vk::ImageUsageFlagBits::eSampled,vk::ImageTiling::eOptimal);
							for(vk::SparseImageFormatProperties& p : l)
								if(p.aspectMask&vk::ImageAspectFlagBits::eColor)
									return p;
							return vk::SparseImageFormatProperties{ {},{0,0,0},{} };
						}();
					cout<<"Sparse image (R8G8B8A8) block: "<<imageFormatProperties.imageGranularity.width<<"x"<<imageFormatProperties.imageGranularity.height<<endl;
				}

				// perform non-sparse test
				cout<<endl;
				cout<<"Standard memory allocation performance"<<endl;
				testMemoryAllocationPerformance(vk::BufferCreateFlags(),1);

				cout<<endl;
				cout<<"Sparse memory allocation performance"<<endl;
				if(!sparseDevice)
					cout<<"   No sparse memory support."<<endl;
				else {

					// replace device by sparseDevice
					device.swap(sparseDevice);

					// get queues
					graphicsQueue=device->getQueue(graphicsQueueFamily,0);
					presentationQueue=device->getQueue(presentationQueueFamily,0);
					sparseQueue=device->getQueue(sparseQueueFamily,0);

					// perform test
					testMemoryAllocationPerformance(vk::BufferCreateFlagBits::eSparseBinding,1);

					// return original device back
					device.swap(sparseDevice);
				}

				break;
			}

		}
	ExitMainLoop:
		device->waitIdle();

	// catch exceptions
	} catch(vk::Error& e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception& e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	// wait device idle, particularly, if there was an exception and device is busy
	// (device must be idle before destructors of buffers and other stuff are called)
	if(device) {
		try {
			device->waitIdle();
		} catch(vk::Error&) {
			// ignore all Vulkan exceptions (especially vk::DeviceLostError)
		}
	}

	return 0;
}
