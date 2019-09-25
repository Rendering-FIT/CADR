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
#else
#include <experimental/filesystem>
using namespace std::experimental;
#endif
#include <fstream>
#include <iostream>
#include <memory>

using namespace std;

using json=nlohmann::json;

typedef logic_error gltfError;


template<typename T,typename Map,typename Key>
T mapGetWithDefault(const Map& m,const Key& k,T d)
{
	auto it=m.find(k);
	if(it!=m.end())
		return *it;
	else
		return d;
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
		CadR::VulkanDevice vulkanDevice(vulkanInstance,deviceAndQueueFamilies,
		                                nullptr,"VK_KHR_swapchain",nullptr);
		vk::PhysicalDevice physicalDevice=std::get<0>(deviceAndQueueFamilies);
		uint32_t graphicsQueueFamily=std::get<1>(deviceAndQueueFamilies);
		uint32_t presentationQueueFamily=std::get<2>(deviceAndQueueFamilies);
		CadR::Renderer renderer(&vulkanDevice,&vulkanInstance,physicalDevice,graphicsQueueFamily);
		CadR::Renderer::set(&renderer);

		// get queues
		vk::Queue graphicsQueue=vulkanDevice->getQueue(graphicsQueueFamily,0,vulkanDevice);
		vk::Queue presentationQueue=vulkanDevice->getQueue(presentationQueueFamily,0,vulkanDevice);

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
		vk::UniqueHandle<vk::RenderPass,CadR::VulkanDevice> renderPass=
			vulkanDevice->createRenderPassUnique(
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
				nullptr,vulkanDevice
			);

		// setup window
		window.initVulkan(physicalDevice,vulkanDevice,surfaceFormat,graphicsQueueFamily,
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

		auto m=make_unique<CadR::Mesh>(CadR::AttribConfig{12},count,count,0);
		m->uploadAttrib(0,data);
		renderer.executeCopyOperations();

		// command buffers and semaphores
		vk::UniqueHandle<vk::CommandPool,CadR::VulkanDevice> commandPool;
		vector<vk::CommandBuffer> commandBuffers;
		auto imageAvailableSemaphore=
			vulkanDevice->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()  // flags
				),
				nullptr,  // allocator
				vulkanDevice  // dispatch
			);
		auto renderingFinishedSemaphore=
			vulkanDevice->createSemaphoreUnique(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()  // flags
				),
				nullptr,  // allocator
				vulkanDevice  // dispatch
			);

		// resize callback
		window.resizeCallbacks.append(
				[&vulkanDevice,&window,&commandPool,&commandBuffers,graphicsQueueFamily]() {
					cout<<"Resize happened."<<endl;

					// recreate command pool
					commandPool=
						vulkanDevice->createCommandPoolUnique(
							vk::CommandPoolCreateInfo(
								vk::CommandPoolCreateFlags(),  // flags
								graphicsQueueFamily  // queueFamilyIndex
							),
							nullptr,  // allocator
							vulkanDevice  // dispatch
						);

					// recreate command buffers
					// (we do not need command buffers to be properly freed as they are freed during command pool re-creation)
					commandBuffers.clear();
					commandBuffers=
						vulkanDevice->allocateCommandBuffers(
							vk::CommandBufferAllocateInfo(
								commandPool.get(),                 // commandPool
								vk::CommandBufferLevel::ePrimary,  // level
								window.imageCount()                // commandBufferCount
							),
							vulkanDevice  // dispatch
						);

					// record command buffers
					for(size_t i=0,c=window.imageCount(); i<c; i++) {
						vk::CommandBuffer& cb=commandBuffers[i];
						cb.begin(
							vk::CommandBufferBeginInfo(
								vk::CommandBufferUsageFlagBits::eSimultaneousUse,  // flags
								nullptr  // pInheritanceInfo
							),
							vulkanDevice  // dispatch
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
							vulkanDevice  // dispatch
						);
						//cb.bindPipeline(vk::PipelineBindPoint::eGraphics,pipeline.get());  // bind pipeline
						//cb.draw(3,1,0,0);  // draw single triangle
						cb.endRenderPass(vulkanDevice);
						cb.end(vulkanDevice);
					}

				},
				nullptr
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
				vulkanDevice  // dispatch
			);
			window.present(presentationQueue,renderingFinishedSemaphore.get(),imageIndex);

			// wait for rendering to complete
			presentationQueue.waitIdle(vulkanDevice);
		}

		// finish all pending work on device
		vulkanDevice->waitIdle(vulkanDevice);

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
