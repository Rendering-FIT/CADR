#include <CadR/BufferData.h>
#include <CadR/CadR.h>
#include <CadR/Mesh.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#include <CadR/VulkanLibrary.h>
#include "Window.h"
#include <vulkan/vulkan.hpp>
#include <nlohmann/json.hpp>
#if _WIN32
#include <filesystem>
#else // gcc 7.4.0 (Ubuntu 18.04) does support path only as experimental
#include <experimental/filesystem>
using namespace std::experimental;
#endif
#include <fstream>
#include <iostream>
#include <memory>

using namespace std;

using json=nlohmann::json;

typedef logic_error gltfError;

// shader code in SPIR-V binary
static const uint32_t coordinateShaderSpirv[]={
#include "shaders/coordinates.vert.spv"
};
static const uint32_t unspecifiedMaterialShaderSpirv[]={
#include "shaders/unspecifiedMaterial.frag.spv"
};


template<typename T,typename Map,typename Key>
T mapGetWithDefault(const Map& m,const Key& k,T d)
{
	auto it=m.find(k);
	if(it!=m.end())
		return *it;
	else
		return d;
}


uint32_t getStride(int componentType,const string& type)
{
	uint32_t componentSize;
	switch(componentType) {
	case 5120:                          // BYTE
	case 5121: componentSize=1; break;  // UNSIGNED_BYTE
	case 5122:                          // SHORT
	case 5123: componentSize=2; break;  // UNSIGNED_SHORT
	case 5125:                          // UNSIGNED_INT
	case 5126: componentSize=4; break;  // FLOAT
	default: throw gltfError("Invalid accessor's componentType value.");
	}
	if(type=="VEC3")  return 3*componentSize;
	else if(type=="VEC4")  return 4*componentSize;
	else if(type=="VEC2")  return 2*componentSize;
	else if(type=="SCALAR")  return componentSize;
	else if(type=="MAT4")  return 16*componentSize;
	else if(type=="MAT3")  return 9*componentSize;
	else if(type=="MAT2")  return 4*componentSize;
	else throw gltfError("Invalid accessor's type.");
}


vk::Format getFormat(int componentType,const string& type,bool normalize,bool wantInt=false)
{
	if(componentType>=5127)
		throw gltfError("Invalid accessor's componentType.");

	// FLOAT component type
	if(componentType==5126) { 
		if(normalize)
			throw gltfError("Normalize set while accessor's componentType is FLOAT (5126).");
		if(wantInt)
			throw gltfError("Integer format asked while accessor's componentType is FLOAT (5126).");
		if(type=="VEC3")       return vk::Format::eR32G32B32Sfloat;
		else if(type=="VEC4")  return vk::Format::eR32G32B32A32Sfloat;
		else if(type=="VEC2")  return vk::Format::eR32G32Sfloat;
		else if(type=="SCALAR")  return vk::Format::eR32Sfloat;
		else if(type=="MAT4")  return vk::Format::eR32G32B32A32Sfloat;
		else if(type=="MAT3")  return vk::Format::eR32G32B32Sfloat;
		else if(type=="MAT2")  return vk::Format::eR32G32Sfloat;
		else throw gltfError("Invalid accessor's type.");
	}

	// UNSIGNED_INT component type
	else if(componentType==5125)
		throw gltfError("UNSIGNED_INT componentType shall be used only for accessors containing indices. No attribute format is supported.");

	// INT component type
	else if(componentType==5124)
		throw gltfError("Invalid componentType. INT is not valid value for glTF 2.0.");

	// SHORT and UNSIGNED_SHORT component type
	else if(componentType>=5122) {
		int base;
		if(type=="VEC3")       base=84;  // VK_FORMAT_R16G16B16_UNORM
		else if(type=="VEC4")  base=91;  // VK_FORMAT_R16G16B16A16_UNORM
		else if(type=="VEC2")  base=77;  // VK_FORMAT_R16G16_UNORM
		else if(type=="SCALAR")  base=70;  // VK_FORMAT_R16_UNORM
		else if(type=="MAT4")  base=91;
		else if(type=="MAT3")  base=84;
		else if(type=="MAT2")  base=77;
		else throw gltfError("Invalid accessor's type.");
		if(componentType==5122)  // signed SHORT
			base+=1;  // VK_FORMAT_R16*_S*
		if(wantInt)    return vk::Format(base+4);  // VK_FORMAT_R16*_[U|S]INT
		if(normalize)  return vk::Format(base);    // VK_FORMAT_R16*_[U|S]NORM
		else           return vk::Format(base+2);  // VK_FORMAT_R16*_[U|S]SCALED
	}

	// BYTE and UNSIGNED_BYTE component type
	else if(componentType>=5120) {
	int base;
		if(type=="VEC3")       base=23;  // VK_FORMAT_R8G8B8_UNORM
		else if(type=="VEC4")  base=37;  // VK_FORMAT_R8G8B8A8_UNORM
		else if(type=="VEC2")  base=16;  // VK_FORMAT_R8G8_UNORM
		else if(type=="SCALAR")  base=9;  // VK_FORMAT_R8_UNORM
		else if(type=="MAT4")  base=37;
		else if(type=="MAT3")  base=23;
		else if(type=="MAT2")  base=16;
		else throw gltfError("Invalid accessor's type.");
		if(componentType==5120)  // signed BYTE
			base+=1;  // VK_FORMAT_R8*_S*
		if(wantInt)    return vk::Format(base+4);  // VK_FORMAT_R16*_[U|S]INT
		if(normalize)  return vk::Format(base);    // VK_FORMAT_R16*_[U|S]NORM
		else           return vk::Format(base+2);  // VK_FORMAT_R16*_[U|S]SCALED
	}

	// componentType bellow 5120
	throw gltfError("Invalid accessor's componentType.");
	return vk::Format(0);
}


int main(int argc,char** argv) {

	try {

		// get file to load
		if(argc<2) {
			cout<<"Please, specify glTF file to load."<<endl;
			return 1;
		}
		filesystem::path filePath(argv[1]);

		// open file
		ifstream f(filePath);
		if(!f.is_open()) {
			cout<<"Can not open file "<<filePath<<"."<<endl;
			return 1;
		}
		f.exceptions(ifstream::badbit|ifstream::failbit);

		// init Vulkan and open window
		CadR::VulkanLibrary vulkanLib;
		CadR::VulkanInstance vulkanInstance(vulkanLib,"glTF reader",0,"CADR",0,VK_API_VERSION_1_0,nullptr,
#ifdef _WIN32
		                                    {"VK_KHR_surface","VK_KHR_win32_surface"});  // enabled extension names
#else
		                                    {"VK_KHR_surface","VK_KHR_xlib_surface"});  // enabled extension names
#endif
		CadUI::Window window(vulkanInstance);
		tuple<vk::PhysicalDevice,uint32_t,uint32_t> deviceAndQueueFamilies=
				vulkanInstance.chooseDeviceAndQueueFamilies(window.surface());
		CadR::VulkanDevice device(vulkanInstance,deviceAndQueueFamilies,
		                          nullptr,"VK_KHR_swapchain",nullptr);
		vk::PhysicalDevice physicalDevice=std::get<0>(deviceAndQueueFamilies);
		uint32_t graphicsQueueFamily=std::get<1>(deviceAndQueueFamilies);
		uint32_t presentationQueueFamily=std::get<2>(deviceAndQueueFamilies);
		CadR::Renderer renderer(&device,&vulkanInstance,physicalDevice,graphicsQueueFamily);
		CadR::Renderer::set(&renderer);

		// get queues
		vk::Queue graphicsQueue=device->getQueue(graphicsQueueFamily,0,device);
		vk::Queue presentationQueue=device->getQueue(presentationQueueFamily,0,device);

		// choose surface formats
		vk::SurfaceFormatKHR surfaceFormat;
		{
			vector<vk::SurfaceFormatKHR> sfList=physicalDevice.getSurfaceFormatsKHR(window.surface(),vulkanInstance);
			const vk::SurfaceFormatKHR wantedSurfaceFormat{vk::Format::eB8G8R8A8Unorm,vk::ColorSpaceKHR::eSrgbNonlinear};
			surfaceFormat=
				sfList.size()==1&&sfList[0].format==vk::Format::eUndefined
					?wantedSurfaceFormat
					:std::find(sfList.begin(),sfList.end(),
					           wantedSurfaceFormat)!=sfList.end()
						           ?wantedSurfaceFormat
						           :sfList[0];
		}
		vk::Format depthFormat=[](vk::PhysicalDevice physicalDevice,CadR::VulkanInstance& vulkanInstance){
			for(vk::Format f:array<vk::Format,3>{vk::Format::eD32Sfloat,vk::Format::eD32SfloatS8Uint,vk::Format::eD24UnormS8Uint}) {
				vk::FormatProperties p=physicalDevice.getFormatProperties(f,vulkanInstance);
				if(p.optimalTilingFeatures&vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
					return f;
				}
			}
			throw std::runtime_error("No suitable depth buffer format.");
		}(physicalDevice,vulkanInstance);

		// choose presentation mode
		vk::PresentModeKHR presentMode=
			[](vector<vk::PresentModeKHR>&& modes){  // presentMode
					return find(modes.begin(),modes.end(),vk::PresentModeKHR::eMailbox)!=modes.end()
						?vk::PresentModeKHR::eMailbox
						:vk::PresentModeKHR::eFifo; // fifo is guaranteed to be supported
				}(physicalDevice.getSurfacePresentModesKHR(window.surface(),window));

		// render pass
		auto renderPass=
			device->createRenderPassUnique(
				vk::RenderPassCreateInfo(
					vk::RenderPassCreateFlags(),  // flags
					1,                            // attachmentCount
					array<const vk::AttachmentDescription,1>{  // pAttachments
						vk::AttachmentDescription{  // color attachment
							vk::AttachmentDescriptionFlags(),  // flags
							surfaceFormat.format,              // format
							vk::SampleCountFlagBits::e1,       // samples
							vk::AttachmentLoadOp::eClear,      // loadOp
							vk::AttachmentStoreOp::eStore,     // storeOp
							vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
							vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
							vk::ImageLayout::eUndefined,       // initialLayout
							vk::ImageLayout::ePresentSrcKHR    // finalLayout
						}/*,
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
						}*/
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
						/*&(const vk::AttachmentReference&)vk::AttachmentReference(  // pDepthStencilAttachment
							1,  // attachment
							vk::ImageLayout::eDepthStencilAttachmentOptimal  // layout
						)*/nullptr,
						0,        // preserveAttachmentCount
						nullptr   // pPreserveAttachments
					),
					1,  // dependencyCount
					&(const vk::SubpassDependency&)vk::SubpassDependency(  // pDependencies
						VK_SUBPASS_EXTERNAL,   // srcSubpass
						0,                     // dstSubpass
						vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // srcStageMask
						vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // dstStageMask
						vk::AccessFlags(),     // srcAccessMask
						vk::AccessFlags(vk::AccessFlagBits::eColorAttachmentRead|vk::AccessFlagBits::eColorAttachmentWrite),  // dstAccessMask
						vk::DependencyFlags()  // dependencyFlags
					)
				),
				nullptr,device
			);

		// setup window
		window.initVulkan(physicalDevice,device,surfaceFormat,graphicsQueueFamily,
		                  presentationQueueFamily,renderPass.get(),presentMode);

		// parse json
		cout<<"Processing file "<<filePath<<"..."<<endl;
		json j3;
		f>>j3;

		// print glTF info
		cout<<endl;
		cout<<"glTF info:"<<endl;
		cout<<"   Version:     "<<j3["asset"]["version"]<<endl;
		cout<<"   MinVersion:  "<<j3["asset"]["minVersion"]<<endl;
		cout<<"   Generator:   "<<j3["asset"]["generator"]<<endl;
		cout<<"   Copyright:   "<<j3["asset"]["copyright"]<<endl;
		cout<<endl;

		// read object lists
		auto scenes=j3["scenes"];
		auto nodes=j3["nodes"];
		auto meshes=j3["meshes"];
		auto accessors=j3["accessors"];
		auto buffers=j3["buffers"];
		auto bufferViews=j3["bufferViews"];

		// print stats
		cout<<"Stats:"<<endl;
		cout<<"   Scenes:  "<<scenes.size()<<endl;
		cout<<"   Nodes:   "<<nodes.size()<<endl;
		cout<<"   Meshes:  "<<meshes.size()<<endl;
		cout<<endl;

		// mesh
		if(meshes.size()!=1)
			throw gltfError("Only files with one mesh are supported for now.");
		auto& mesh=meshes[0];

		// primitive
		auto primitives=mesh["primitives"];
		if(primitives.size()!=1)
			throw gltfError("Only meshes with one primitive are supported for now.");
		auto primitive=primitives[0];

		// primitive data
		auto attributes=primitive["attributes"];
		if(primitive.find("indices")!=primitive.end())
			throw gltfError("Unsupported functionality: indices.");
		auto mode=mapGetWithDefault(primitive,"mode",4);
		if(mode!=4)
			throw gltfError("Unsupported functionality: mode is not 4 (TRIANGLES).");

		// accessor
		if(attributes.find("POSITION")==attributes.end() || attributes.size()!=1)
			throw gltfError("Unsupported attribute configuration.");
		size_t positionAccessorIndex=attributes["POSITION"];
		auto positionAccessor=accessors.at(positionAccessorIndex);
		if(positionAccessor.find("bufferView")==positionAccessor.end())
			throw gltfError("Unsupported functionality: Omitted bufferView.");
		size_t bufferViewIndex=positionAccessor["bufferView"];
		size_t accessorOffset=mapGetWithDefault(positionAccessor,"byteOffset",0);
		int componentType=positionAccessor["componentType"];
		if(componentType!=5126) // float
			throw gltfError("Invalid component type. Must be float.");
		bool normalized=mapGetWithDefault(positionAccessor,"normalized",false);
		size_t count=positionAccessor["count"];
		if(count==0)
			throw gltfError("Attribute count in accessor is 0.");
		string type=positionAccessor["type"];
		if(type!="VEC3")
			throw gltfError("Invalid attribute type. Must be VEC3.");
		if(positionAccessor.find("sparse")!=positionAccessor.end())
			throw gltfError("Unsupported functionality: Property sparse.");

		// bufferView
		auto bufferView=bufferViews.at(bufferViewIndex);
		size_t bufferIndex=bufferView["buffer"];
		size_t bufferViewOffset=mapGetWithDefault(bufferView,"byteOffset",0);
		size_t bufferViewLength=bufferView["byteLength"];
		size_t stride=mapGetWithDefault(bufferView,"byteStride",0);
		if(stride!=0)
			throw gltfError("Unsupported functionality: Stride not zero.");

		// buffer
		auto buffer=buffers.at(bufferIndex);
		filesystem::path bufferUri=mapGetWithDefault<string>(buffer,"uri","");
		size_t bufferLength=buffer["byteLength"];

		// open buffer file
		filesystem::path bufferPath=
			bufferUri.is_absolute()
				?bufferUri
				:filePath.parent_path()/bufferUri;
		cout<<"Opening buffer "<<bufferPath<<"..."<<endl;
		ifstream b(bufferPath);
		if(!b.is_open()) {
			cout<<"Can not open file "<<bufferPath<<"."<<endl;
			return 1;
		}
		b.exceptions(ifstream::badbit|ifstream::failbit);

		// read buffer file
		vector<uint8_t> data(count*12);
		b.seekg(bufferViewOffset+accessorOffset);
		b.read(reinterpret_cast<istream::char_type*>(data.data()),data.size());
		b.close();

		auto m=
			make_unique<CadR::Mesh>(
				CadR::AttribConfig{12},  // attribConfig
				count,  // numVertices
				count,  // numIndices
				0  // numDrawCommands
			);
		m->uploadAttrib(0,data);
		renderer.executeCopyOperations();

		// shaders
		auto coordinateShader=
			device->createShaderModuleUnique(
				vk::ShaderModuleCreateInfo(
					vk::ShaderModuleCreateFlags(),  // flags
					sizeof(coordinateShaderSpirv),  // codeSize
					coordinateShaderSpirv  // pCode
				),
				nullptr,  // allocator
				device  // dispatch
			);
		auto unknownMaterialShader=
			device->createShaderModuleUnique(
				vk::ShaderModuleCreateInfo(
					vk::ShaderModuleCreateFlags(),  // flags
					sizeof(unspecifiedMaterialShaderSpirv),  // codeSize
					unspecifiedMaterialShaderSpirv  // pCode
				),
				nullptr,  // allocator
				device  // dispatch
			);

		// command buffers and pipelines
		vk::UniqueHandle<vk::CommandPool,CadR::VulkanDevice> commandPool;
		vector<vk::CommandBuffer> commandBuffers;
		auto pipelineCache=
			device->createPipelineCacheUnique(
				vk::PipelineCacheCreateInfo(
					vk::PipelineCacheCreateFlags(),  // flags
					0,       // initialDataSize
					nullptr  // pInitialData
				),
				nullptr,  // allocator
				device  // dispatch
			);
		auto pipelineLayout=
			device->createPipelineLayoutUnique(
				vk::PipelineLayoutCreateInfo{
					vk::PipelineLayoutCreateFlags(),  // flags
					0,       // setLayoutCount
					nullptr, // pSetLayouts
					0,       // pushConstantRangeCount
					nullptr  // pPushConstantRanges
				},
				nullptr,  // allocator
				device  // dispatch
			);
		vk::UniqueHandle<vk::Pipeline,CadR::VulkanDevice> coordinatePipeline;

		// resize callback
		window.resizeCallbacks.append(
				[&device,&window,&commandPool,&commandBuffers,graphicsQueueFamily,
				 &coordinateShader,&unknownMaterialShader,
				 &pipelineCache,&pipelineLayout,&coordinatePipeline,
				 &m,stride=getStride(componentType,type),
				 coordinateFormat=getFormat(componentType,type,normalized,false)]() {

					cout<<"Resize happened."<<endl;

					// recreate command pool
					commandPool=
						device->createCommandPoolUnique(
							vk::CommandPoolCreateInfo(
								vk::CommandPoolCreateFlags(),  // flags
								graphicsQueueFamily  // queueFamilyIndex
							),
							nullptr,  // allocator
							device  // dispatch
						);

					// recreate command buffers
					// (we do not need command buffers to be properly freed as they are freed during command pool re-creation)
					commandBuffers.clear();
					commandBuffers=
						device->allocateCommandBuffers(
							vk::CommandBufferAllocateInfo(
								commandPool.get(),                 // commandPool
								vk::CommandBufferLevel::ePrimary,  // level
								window.imageCount()                // commandBufferCount
							),
							device  // dispatch
						);

					// coordinate pipeline
					coordinatePipeline=
						device->createGraphicsPipelineUnique(
							pipelineCache.get(),
							vk::GraphicsPipelineCreateInfo(
								vk::PipelineCreateFlags(),  // flags
								2,  // stageCount
								array<const vk::PipelineShaderStageCreateInfo,2>{  // pStages
									vk::PipelineShaderStageCreateInfo{
										vk::PipelineShaderStageCreateFlags(),  // flags
										vk::ShaderStageFlagBits::eVertex,      // stage
										coordinateShader.get(),  // module
										"main",  // pName
										nullptr  // pSpecializationInfo
									},
									vk::PipelineShaderStageCreateInfo{
										vk::PipelineShaderStageCreateFlags(),  // flags
										vk::ShaderStageFlagBits::eFragment,    // stage
										unknownMaterialShader.get(),  // module
										"main",  // pName
										nullptr  // pSpecializationInfo
									}
								}.data(),
								&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{  // pVertexInputState
									vk::PipelineVertexInputStateCreateFlags(),  // flags
									1,        // vertexBindingDescriptionCount
									array<const vk::VertexInputBindingDescription,1>{  // pVertexBindingDescriptions
										vk::VertexInputBindingDescription(
											0,  // binding
											stride,  // stride
											vk::VertexInputRate::eVertex  // inputRate
										),
									}.data(),
									1,        // vertexAttributeDescriptionCount
									array<const vk::VertexInputAttributeDescription,1>{  // pVertexAttributeDescriptions
										vk::VertexInputAttributeDescription(
											0,  // location
											0,  // binding
											coordinateFormat,  // format
											0   // offset
										),
									}.data()
								},
								&(const vk::PipelineInputAssemblyStateCreateInfo&)vk::PipelineInputAssemblyStateCreateInfo{  // pInputAssemblyState
									vk::PipelineInputAssemblyStateCreateFlags(),  // flags
									vk::PrimitiveTopology::eTriangleList,  // topology
									VK_FALSE  // primitiveRestartEnable
								},
								nullptr, // pTessellationState
								&(const vk::PipelineViewportStateCreateInfo&)vk::PipelineViewportStateCreateInfo{  // pViewportState
									vk::PipelineViewportStateCreateFlags(),  // flags
									1,  // viewportCount
									&(const vk::Viewport&)vk::Viewport(0.f,0.f,float(window.surfaceExtent().width),float(window.surfaceExtent().height),0.f,1.f),  // pViewports
									1,  // scissorCount
									&(const vk::Rect2D&)vk::Rect2D(vk::Offset2D(0,0),window.surfaceExtent())  // pScissors
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
								pipelineLayout.get(),  // layout
								window.renderPass(),  // renderPass
								0,  // subpass
								vk::Pipeline(nullptr),  // basePipelineHandle
								-1 // basePipelineIndex
							),
							nullptr,  // allocator
							device  // dispatch
						);

					// record command buffers
					for(size_t i=0,c=window.imageCount(); i<c; i++) {
						vk::CommandBuffer& cb=commandBuffers[i];
						cb.begin(
							vk::CommandBufferBeginInfo(
								vk::CommandBufferUsageFlagBits::eSimultaneousUse,  // flags
								nullptr  // pInheritanceInfo
							),
							device  // dispatch
						);
						cb.beginRenderPass(
							vk::RenderPassBeginInfo(
								window.renderPass(),       // renderPass
								window.framebuffers()[i],  // framebuffer
								vk::Rect2D(vk::Offset2D(0,0),window.surfaceExtent()),  // renderArea
								1,                         // clearValueCount
								&(const vk::ClearValue&)vk::ClearValue(vk::ClearColorValue(array<float,4>{0.f,0.f,1.f,1.f}))  // pClearValues
							),
							vk::SubpassContents::eInline,  // contents
							device  // dispatch
						);
						cb.bindPipeline(vk::PipelineBindPoint::eGraphics,coordinatePipeline.get(),device);
						cb.bindVertexBuffers(
							0,  // firstBinding
							1,  // bindingCount
							m->attribStorage()->bufferList().data(),  // pBuffers
							array<const vk::DeviceSize,1>{0}.data(),  // pOffsets
							device  // dispatch
						);
						cb.draw(3,1,0,0,device);  // draw a single triangle
						cb.endRenderPass(device);
						cb.end(device);
					}

				},
				nullptr
			);

		// semaphores
		auto imageAvailableSemaphore=
			device->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()  // flags
				),
				nullptr,  // allocator
				device  // dispatch
			);
		auto renderingFinishedSemaphore=
			device->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()  // flags
				),
				nullptr,  // allocator
				device  // dispatch
			);

		// main loop
		while(window.processEvents()) {
			if(!window.updateSize())
				continue;

			// render
			auto [imageIndex,success]=window.acquireNextImage(imageAvailableSemaphore.get());
			if(!success)
				continue;
			graphicsQueue.submit(
				vk::SubmitInfo(
					1,&imageAvailableSemaphore.get(),    // waitSemaphoreCount+pWaitSemaphores
					&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // pWaitDstStageMask
					1,&commandBuffers[imageIndex],       // commandBufferCount+pCommandBuffers
					1,&renderingFinishedSemaphore.get()  // signalSemaphoreCount+pSignalSemaphores
				),
				vk::Fence(nullptr),  // fence
				device  // dispatch
			);
			window.present(presentationQueue,renderingFinishedSemaphore.get(),imageIndex);

			// wait for rendering to complete
			presentationQueue.waitIdle(device);
		}

		// finish all pending work on device
		device->waitIdle(device);

	// catch exceptions
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	return 0;
}
