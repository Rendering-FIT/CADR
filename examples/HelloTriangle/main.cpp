// SPDX-FileCopyrightText: 2026 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT-0

#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/Exceptions.h>
#include <CadR/Pipeline.h>
#include <CadR/Renderer.h>
#include <CadR/StateSet.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#include <CadR/VulkanLibrary.h>
#include <CadR/VulkanSupport.h>
#include "VulkanWindow.h"
#include <vulkan/vulkan.hpp>
#include <glm/mat4x4.hpp>
#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <string_view>
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN  // reduce amount of included files by windows.h
# include <windows.h>  // needed for SetConsoleOutputCP()
# ifdef MemoryBarrier  // came through windows.h and collides with vk::MemoryBarrier
#  undef MemoryBarrier
# endif
#endif

using namespace std;


// global variables
static const string_view executableName = "HelloTriangle";  // name of the executable (used in command line help, for instance)
static const string_view fancyAppName = "HelloTriangle";  // name of the application (used for window title and vk::ApplicationInfo, for instance)
static const uint32_t vulkanAppVersion = VK_MAKE_VERSION(0, 0, 0);
static const string_view engineName = "CADR";
static const uint32_t engineVersion = VK_MAKE_VERSION(0, 0, 0);
static const uint32_t vulkanApiVersion = VK_API_VERSION_1_4;


// shader code in SPIR-V binary
static const uint32_t vertexShaderSpirv[]={
#include "shader.vert.spv"
};
static const uint32_t fragmentShaderSpirv[]={
#include "shader.frag.spv"
};


// application class
class App {
public:

	App(int argc, char* argv[]);
	~App();

	void init();
	void resize(VulkanWindow& window,
		const vk::SurfaceCapabilitiesKHR& surfaceCapabilities, vk::Extent2D newSurfaceExtent);
	void frame(VulkanWindow& window);

	// command line options
	string_view deviceNameFilter;
	int deviceIndex = -1;

	// Vulkan core objects
	// (The order of members is not arbitrary but defines construction and destruction order.)
	// (If App object is on the stack, do not call std::exit() as the App destructor might not be called.)
	// (It is probably good idea to destroy vulkanLib and vulkanInstance after the display connection.)
	CadR::VulkanLibrary vulkanLib;
	CadR::VulkanInstance vulkanInstance;

	// window needs to be destroyed after the swapchain
	// (this is required especially on Wayland)
	VulkanWindow window;

	// Vulkan variables, handles and objects
	// (they need to be destructed in non-arbitrary order in the destructor)
	vk::PhysicalDevice physicalDevice;
	uint32_t graphicsQueueFamily;
	uint32_t presentationQueueFamily;
	CadR::VulkanDevice device;
	vk::Queue graphicsQueue;
	vk::Queue presentationQueue;
	vk::SurfaceFormatKHR surfaceFormat;

	CadR::Renderer renderer;
	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;
	vk::RenderPass renderPass;
	vector<vk::Framebuffer> framebuffers;
	vector<vk::Semaphore> renderingFinishedSemaphores;
	vk::Semaphore imageAvailableSemaphore;
	vk::Fence renderingFinishedFence;
	vk::SwapchainKHR swapchain;
	vector<vk::Image> swapchainImages;
	vector<vk::ImageView> swapchainImageViews;

	CadR::StateSet stateSet;
	vk::ShaderModule vertexShader;
	vk::ShaderModule fragmentShader;
	CadR::Pipeline pipeline;
	CadR::Geometry geometry;
	CadR::Drawable drawable;
	CadR::MatrixList matrixList;

};


class ExitWithMessage {
protected:
	int _exitCode;
	string _what;
public:
	ExitWithMessage(int exitCode, const string& msg) : _exitCode(exitCode), _what(msg) {}
	ExitWithMessage(int exitCode, const char* msg) : _exitCode(exitCode), _what(msg) {}
	const char* what() const noexcept  { return _what.c_str(); }
	int exitCode() const noexcept  { return _exitCode; }
};


/// Construct application object
App::App(int argc, char** argv)
	: stateSet(renderer)
	, geometry(renderer, CadR::Geometry::noInit)
	, matrixList(renderer, CadR::MatrixList::noInit)
{
	auto printHelp =
		[](string_view msg = "") {
			throw
				ExitWithMessage(99,
					string(msg) +
					string(executableName) + " [-h] [--help] [deviceNameFilter] [-<deviceIndex>]\n"
					"   -h --help  prints this screen\n"
					"   deviceNameFilter  considers only Vulkan devices whose name\n"
					"                     contains <deviceNameFilter> string\n"
					"   -<deviceIndex>  use device on particular index");
		};

	// process command-line arguments
	for(int i=1; i<argc; i++) {

		// parse options starting with '-'
		if(argv[i][0] == '-') {

			// filter Vulkan devices
			if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
				printHelp();
			else if(argv[i][1] >= '0' && argv[i][1] <= '9') {
				char* endp = nullptr;
				deviceIndex = strtol(&argv[i][1], &endp, 10);
				if(deviceIndex == 0 || endp == &argv[i][1] || (endp && *endp != 0))
					printHelp(string("Wrong device index parameter \"") + argv[i] + "\". Usage:\n");
			}
			else
				printHelp(string("Wrong parameter \"") + argv[i] + "\". Usage:\n");
		}
		else
			deviceNameFilter = argv[i];
	}
}


App::~App()
{
	if(device) {

		// wait for device idle state
		// (to prevent errors during destruction of Vulkan resources);
		// we ignore any returned error codes here
		// because the device might be in the lost state already, etc.
		device.vkDeviceWaitIdle(device.handle());

		matrixList.finalize();
		geometry.finalize();
		stateSet.destroy();
		pipeline.destroyPipeline(device);
		pipeline.destroyPipelineLayout(device);
		device.destroy(vertexShader);
		device.destroy(fragmentShader);
		device.destroy(commandPool);
		renderer.finalize();
		device.destroy(renderingFinishedFence);
		device.destroy(imageAvailableSemaphore);
		for(auto f : framebuffers)  device.destroy(f);
		for(auto v : swapchainImageViews)  device.destroy(v);
		for(auto s : renderingFinishedSemaphores)  device.destroy(s);
		device.destroy(swapchain);
		device.destroy(renderPass);
		device.destroy();

	}

	window.destroy();
#if defined(USE_PLATFORM_XLIB) || defined(USE_PLATFORM_QT)
	// On Xlib, VulkanWindow::finalize() needs to be called before instance destroy to avoid crash.
	// It is workaround for the known bug in libXext: https://gitlab.freedesktop.org/xorg/lib/libxext/-/issues/3,
	// that crashes the application inside XCloseDisplay(). The problem seems to be present
	// especially on Nvidia drivers (reproduced on versions 470.129.06 and 515.65.01, for example).
	//
	// On Qt, VulkanWindow::finalize() needs to be called not too late after leaving main() because
	// crash might follow. Probably Qt gets partly finalized. Seen on Linux with Qt 5.15.13 and Qt 6.4.2 on 2024-05-03.
	// Calling VulkanWindow::finalize() before leaving main() seems to be a safe solution. For simplicity, we are doing it here.
	VulkanWindow::finalize();
#endif
	vulkanInstance.destroy();
}


void App::init()
{
	// init Vulkan and window
	VulkanWindow::init();
	vulkanLib.load(CadR::VulkanLibrary::defaultName());
	vulkanInstance.create(
		vulkanLib,
		fancyAppName.data(),  // applicationName
		vulkanAppVersion,  // applicationVersion
		engineName.data(),  // engineName
		engineVersion,  // engineVersion
		vulkanApiVersion,  // apiVersion - maximum version that gltfReader might use
		nullptr,  // enabledLayers
		VulkanWindow::requiredExtensions()  // enabledExtensions
	);
	window.create(vulkanInstance.handle(), {1024,768}, string(fancyAppName),
	              vulkanLib.vkGetInstanceProcAddr);

	// choose device
	vk::PhysicalDeviceProperties deviceProperties;
	CadR::DeviceFeatures deviceFeatures;
	tie(physicalDevice, graphicsQueueFamily, presentationQueueFamily, std::ignore,
	    deviceProperties, deviceFeatures, std::ignore) =
		[&]() {

			// find compatible devices
			vector<vk::PhysicalDevice> deviceList = vulkanInstance.enumeratePhysicalDevices();
			vector<tuple<vk::PhysicalDevice, uint32_t, uint32_t, bool,
				vk::PhysicalDeviceProperties, CadR::DeviceFeatures, int>> compatibleDevices;
			compatibleDevices.reserve(deviceList.size());
			vector<tuple<vk::PhysicalDevice, vk::PhysicalDeviceProperties, string>>
				incompatibleDevices;
			vk::SurfaceKHR presentationSurface = window.surface();
			vk::PhysicalDeviceProperties deviceProperties;
			CadR::DeviceFeatures deviceFeatures;
			bool noDeviceNameMatch = true;
			string incompatibilityReasons;
			for(vk::PhysicalDevice pd : deviceList)
			{
				deviceProperties = vulkanInstance.getPhysicalDeviceProperties(pd);
				incompatibilityReasons.clear();

				// deviceNameFilter
				if(!deviceNameFilter.empty()) {
					string_view deviceName(deviceProperties.deviceName);
					if(deviceName.find(deviceNameFilter) == string_view::npos)
						incompatibilityReasons += ", device name does not match user request";
					else
						noDeviceNameMatch = false;
				}

				// Vulkan 1.2 is required
				if(deviceProperties.apiVersion < VK_API_VERSION_1_2)
					incompatibilityReasons += ", device Vulkan version 1.2 or higher is required";

				// device features
				// (consider only devices with Vulkan 1.2 and later)
				if(deviceProperties.apiVersion >= VK_API_VERSION_1_2) {
					deviceFeatures.link(deviceProperties.apiVersion);
					vulkanInstance.getPhysicalDeviceFeatures2(pd, &deviceFeatures.vulkan10);
					string missingFeatures = CadR::missingRequiredFeatures(deviceFeatures);
					if(!missingFeatures.empty())
						incompatibilityReasons += ", missing Vulkan features: " + missingFeatures;
				}

				// VK_KHR_swapchain is required
				vector<vk::ExtensionProperties> extensionList = vulkanInstance.enumerateDeviceExtensionProperties(pd);
				for(vk::ExtensionProperties& e : extensionList)
					if(strcmp(e.extensionName, "VK_KHR_swapchain") == 0)
						goto swapchainSupported;
				incompatibilityReasons += ", VK_KHR_swapchain extension is required";
			swapchainSupported:;

				// select queues for submitting operations and for presentation
				uint32_t operationsQueueFamily = UINT32_MAX;
				uint32_t presentationQueueFamily = UINT32_MAX;
				vector<vk::QueueFamilyProperties> queueFamilyList = vulkanInstance.getPhysicalDeviceQueueFamilyProperties(pd);
				for(uint32_t i=0, c=uint32_t(queueFamilyList.size()); i<c; i++) {

					// test for graphics and compute operations support
					// and timestamp support (transfer operations are included automatically)
					if((queueFamilyList[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
					   (queueFamilyList[i].queueFlags & vk::QueueFlagBits::eCompute) && 
					   queueFamilyList[i].timestampValidBits != 0)
					{
						// test for presentation support
						if(vulkanInstance.getPhysicalDeviceSurfaceSupportKHR(pd, i, presentationSurface)) {

							// if operations and presentation are supported on the same queue,
							// we will use just one queue
							operationsQueueFamily = i;
							presentationQueueFamily = i;
							goto queueFamiliesFound;
						}
						else
							// if only operations and timestamps are supported,
							// we store the first such queue
							if(operationsQueueFamily == UINT32_MAX)
								operationsQueueFamily = i;
					}
					else {
						// test for presentation support
						if(vulkanInstance.getPhysicalDeviceSurfaceSupportKHR(pd, i, presentationSurface))
							// if only presentation is supported on the queue,
							// we store the first such queue
							if(presentationQueueFamily == UINT32_MAX)
								presentationQueueFamily = i;
					}
				}

				if(operationsQueueFamily == UINT32_MAX)
					incompatibilityReasons += ", no queue with required functionalities";
				if(presentationQueueFamily == UINT32_MAX)
					incompatibilityReasons += ", no presentation queue";

			queueFamiliesFound:;

				if(incompatibilityReasons.empty()) {

					// features required for dynamic rendering
					bool dynamicRenderingCapable;
					if(deviceProperties.apiVersion < VK_API_VERSION_1_4)  // Vulkan 1.4 is here since 2024-12-03, supported by Nvidia since Maxwell (GTX 9xx), by AMD since Radeon RX 5000 series on Windows and GCN3 (many cards of RX 3xx series) on Linux, on Intel since Gen11 (on desktop since Rocket Lake, e.g. Core 11xxx) and since Skylake on Linux
						dynamicRenderingCapable = false;
					else {
						dynamicRenderingCapable =
							deviceFeatures.vulkan10.features.sampleRateShading &&
							deviceFeatures.vulkan13.dynamicRendering &&  // dynamic rendering as introduced in Vulkan 1.3
							deviceFeatures.vulkan14.dynamicRenderingLocalRead &&  // allows local reads by subsequent fragment shaders
							deviceProperties.limits.maxColorAttachments >= 5;  // Vulkan 1.4 guarantees 8, Vulkan 1.0 only 4; this test is superfluous for 1.4
					}

					// compute device score considering:
					// - device type
					// - capability to use dynamic rendering in the context of CADR
					// - having operations queue the same as present queue
					int score;
					constexpr const array deviceTypeScore = {
						 10, // vk::PhysicalDeviceType::eOther         - lowest score
						140, // vk::PhysicalDeviceType::eIntegratedGpu - high score
						150, // vk::PhysicalDeviceType::eDiscreteGpu   - highest score
						130, // vk::PhysicalDeviceType::eVirtualGpu    - normal score
						 20, // vk::PhysicalDeviceType::eCpu           - low score
					};
					int deviceType = int(deviceProperties.deviceType);
					if(deviceType >= 0 || deviceType < int(deviceTypeScore.size()))
						score = deviceTypeScore[deviceType];
					else
						score = 0;
					if(dynamicRenderingCapable)
						score += 100;
					if(operationsQueueFamily == presentationQueueFamily)
						score++;

					compatibleDevices.emplace_back(pd, operationsQueueFamily, presentationQueueFamily,
					                               dynamicRenderingCapable, deviceProperties, deviceFeatures, score);

				} else {
					incompatibleDevices.emplace_back(pd, deviceProperties, move(incompatibilityReasons));
					incompatibilityReasons.clear();  // reinit string from undefined state after move operation
				}
			}

			// handle no compatible devices
			if(compatibleDevices.empty())
				if(!deviceNameFilter.empty() && noDeviceNameMatch)
					throw ExitWithMessage(2, string("No Vulkan device found that matches \"") + string(deviceNameFilter) + "\" in its name.");
				else
					throw ExitWithMessage(2, "No compatible Vulkan device found.");

			// sort devices
			stable_sort(compatibleDevices.begin(), compatibleDevices.end(),
				[](const decltype(compatibleDevices)::value_type& d1,
				   const decltype(compatibleDevices)::value_type& d2) { return get<6>(d1) > get<6>(d2); });

			// print device list
			cout << "List of devices:" << endl;
			for(size_t i=0, c=compatibleDevices.size(); i<c; i++) {
				const auto& d = compatibleDevices[i];
				cout << "   " << i+1 << ": " << get<4>(d).deviceName << endl;
			}
			for(size_t i=0, c=incompatibleDevices.size(); i<c; i++) {
				const auto& d = incompatibleDevices[i];
				cout << "   incompatible: " << get<1>(d).deviceName
				     << " (incompatibility reason: " << get<2>(d) << ")" << endl;
			}

			// return results
			return compatibleDevices.front();

		}();

	// print used device
	cout << "\nUsing device:\n"
	        "   " << deviceProperties.deviceName << endl;

	// init device and renderer
	device.create(
		vulkanInstance, physicalDevice, graphicsQueueFamily, presentationQueueFamily,
#if 0 // enable validation extensions and features
		{"VK_KHR_swapchain", "VK_KHR_shader_non_semantic_info"},
		[&]() {
			CadR::DeviceFeatures features(deviceProperties.apiVersion);
			features.vulkan12.uniformAndStorageBuffer8BitAccess = true;
#else
		"VK_KHR_swapchain",
		[&]() {
			CadR::DeviceFeatures features(deviceProperties.apiVersion);
#endif
			CadR::setRequiredFeatures(features);
			return features;
		}().vulkan10
	);
	window.setDevice(device.handle(), physicalDevice);
	renderer.init(device, vulkanInstance, physicalDevice, graphicsQueueFamily);

	// get queues
	graphicsQueue = device.getQueue(graphicsQueueFamily, 0);
	presentationQueue = device.getQueue(presentationQueueFamily, 0);

	// choose surface format
	surfaceFormat =
		[](vk::PhysicalDevice physicalDevice, CadR::VulkanInstance& vulkanInstance, vk::SurfaceKHR surface)
		{
			constexpr const array candidateFormats{
				vk::SurfaceFormatKHR{ vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
				vk::SurfaceFormatKHR{ vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
				vk::SurfaceFormatKHR{ vk::Format::eA8B8G8R8SrgbPack32, vk::ColorSpaceKHR::eSrgbNonlinear },
			};
			vector<vk::SurfaceFormatKHR> availableFormats =
				physicalDevice.getSurfaceFormatsKHR(surface, vulkanInstance);
			if(availableFormats.size()==1 && availableFormats[0].format==vk::Format::eUndefined)
				// Vulkan spec allowed single eUndefined value until 1.1.111 (2019-06-10)
				// with the meaning you can use any valid vk::Format value.
				// Now, it is forbidden, but let's handle any old driver.
				return candidateFormats[0];
			else {
				for(vk::SurfaceFormatKHR sf : availableFormats) {
					auto it = std::find(candidateFormats.begin(), candidateFormats.end(), sf);
					if(it != candidateFormats.end())
						return *it;
				}
				if(availableFormats.size() == 0)  // Vulkan must return at least one format (this is mandated since Vulkan 1.0.37 (2016-10-10), but was missing in the spec before probably because of omission)
					throw CadR::LogicError("Vulkan error: getSurfaceFormatsKHR() returned empty list.");
				return availableFormats[0];
			}
		}(physicalDevice, vulkanInstance, window.surface());

		// render pass
		renderPass =
			device.createRenderPass(
				vk::RenderPassCreateInfo(
					vk::RenderPassCreateFlags(),  // flags
					1,                            // attachmentCount
					array<const vk::AttachmentDescription, 1>{  // pAttachments
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
						},
					}.data(),
					1,  // subpassCount
					array<vk::SubpassDescription, 1>{  // pSubpasses
						vk::SubpassDescription{
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
							nullptr,  // pDepthStencilAttachment
							0,        // preserveAttachmentCount
							nullptr   // pPreserveAttachments
						},
					}.data(),
					2,  // dependencyCount
					array{  // pDependencies
						vk::SubpassDependency(
							VK_SUBPASS_EXTERNAL,   // srcSubpass
							0,                     // dstSubpass
							vk::PipelineStageFlags(  // srcStageMask
								vk::PipelineStageFlagBits::eColorAttachmentOutput |
								vk::PipelineStageFlagBits::eComputeShader |
								vk::PipelineStageFlagBits::eTransfer),
							vk::PipelineStageFlags(  // dstStageMask
								vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eVertexInput |
								vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eColorAttachmentOutput),
							vk::AccessFlags(vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eTransferWrite),  // srcAccessMask
							vk::AccessFlags(  // dstAccessMask
								vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead |
								vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentWrite),
							vk::DependencyFlags()  // dependencyFlags
						),
						vk::SubpassDependency(
							0,                    // srcSubpass
							VK_SUBPASS_EXTERNAL,  // dstSubpass
							vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),
							vk::PipelineStageFlags(vk::PipelineStageFlagBits::eBottomOfPipe),  // dstStageMask
							vk::AccessFlags(vk::AccessFlagBits::eColorAttachmentWrite),
							vk::AccessFlags(),     // dstAccessMask
							vk::DependencyFlags()  // dependencyFlags
						),
					}.data()

				)
			);

	// rendering semaphore and fence
	imageAvailableSemaphore =
		device.createSemaphore(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()  // flags
			)
		);
	renderingFinishedFence =
		device.createFence(
			vk::FenceCreateInfo(
				vk::FenceCreateFlagBits::eSignaled  // flags
			)
		);

	vertexShader =
		device.createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(vertexShaderSpirv),  // codeSize
				vertexShaderSpirv  // pCode
			)
		);
	fragmentShader =
		device.createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(fragmentShaderSpirv),  // codeSize
				fragmentShaderSpirv  // pCode
			)
		);
	pipeline.init(
		nullptr,  // pipeline - will be supplied during resize event
		device.createPipelineLayout(  // pipelineLayout
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				0,        // setLayoutCount
				nullptr,  // pSetLayouts
				1,  // pushConstantRangeCount
				array{
					vk::PushConstantRange{  // pPushConstantRanges
						vk::ShaderStageFlagBits::eAllGraphics,  // stageFlags
						8,  // offset
						8  // size
					},
				}.data()
			}
		),
		nullptr  // descriptorSetLayoutList
	);

	// command buffer
	commandPool =
		device.createCommandPool(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
				graphicsQueueFamily  // queueFamilyIndex
			)
		);
	commandBuffer =
		device.allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				commandPool,                       // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1                                  // commandBufferCount
			)
		)[0];

	// scene
	stateSet.pipeline = &pipeline;
	geometry.init();
	CadR::StagingData sd = geometry.createVertexStagingData(16*3);
	struct Vertex {
		glm::vec3 position;
		uint32_t color;
	};
	Vertex* v = sd.data<Vertex>();
	v[0].position = {  0.433f,  0.250f, 0.f };
	v[0].color = 0xff0000ff;
	v[1].position = {  0.000f, -0.500f, 0.f };
	v[1].color = 0xff00ff00;
	v[2].position = { -0.433f,  0.250f, 0.f };
	v[2].color = 0xffff0000;
	sd = geometry.createIndexStagingData(12);
	uint32_t* indices = sd.data<uint32_t>();
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	sd = geometry.createPrimitiveSetStagingData(8);
	uint32_t* ps = sd.data<uint32_t>();
	ps[0] = 3;  // count
	ps[1] = 0;  // first
	matrixList.init();
	matrixList.setMatrices(glm::mat4(1.f));  // set identity matrix
	drawable.create(
		geometry,  // geometry
		0,  // primitiveSetOffset
		matrixList,  // matrixList
		stateSet  // stateSet
	);
}


void App::resize(VulkanWindow& window,
	const vk::SurfaceCapabilitiesKHR& surfaceCapabilities, vk::Extent2D newSurfaceExtent)
{
	// clear resources
	for(auto v : swapchainImageViews)  device.destroy(v);
	swapchainImageViews.clear();
	for(auto f : framebuffers)  device.destroy(f);
	framebuffers.clear();
	pipeline.destroyPipeline(device);
	pipeline.set(nullptr);

	// create new swapchain
	constexpr const uint32_t requestedImageCount = 2;
	vk::UniqueHandle<vk::SwapchainKHR, CadR::VulkanDevice> newSwapchain =
		device.createSwapchainKHRUnique(
			vk::SwapchainCreateInfoKHR(
				vk::SwapchainCreateFlagsKHR(),  // flags
				window.surface(),               // surface
				surfaceCapabilities.maxImageCount==0  // minImageCount
					? max(requestedImageCount, surfaceCapabilities.minImageCount)
					: clamp(requestedImageCount, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount),
				surfaceFormat.format,           // imageFormat
				surfaceFormat.colorSpace,       // imageColorSpace
				newSurfaceExtent,               // imageExtent
				1,                              // imageArrayLayers
				vk::ImageUsageFlagBits::eColorAttachment,  // imageUsage
				(graphicsQueueFamily==presentationQueueFamily) ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent, // imageSharingMode
				uint32_t(2),  // queueFamilyIndexCount
				array<uint32_t, 2>{graphicsQueueFamily, presentationQueueFamily}.data(),  // pQueueFamilyIndices
				surfaceCapabilities.currentTransform,    // preTransform
				vk::CompositeAlphaFlagBitsKHR::eOpaque,  // compositeAlpha
				vk::PresentModeKHR::eFifo,  // presentMode
				VK_TRUE,  // clipped
				swapchain  // oldSwapchain
			)
		);
	device.destroy(swapchain);
	swapchain = newSwapchain.release();

	// swapchain images and image views
	swapchainImages = device.getSwapchainImagesKHR(swapchain);
	swapchainImageViews.reserve(swapchainImages.size());
	for(vk::Image image : swapchainImages)
		swapchainImageViews.emplace_back(
			device.createImageView(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),  // flags
					image,                       // image
					vk::ImageViewType::e2D,      // viewType
					surfaceFormat.format,        // format
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

	// framebuffers
	framebuffers.reserve(swapchainImages.size());
	for(size_t i=0, c=swapchainImages.size(); i<c; i++)
		framebuffers.emplace_back(
			device.createFramebuffer(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),  // flags
					renderPass,  // renderPass
					1,  // attachmentCount
					array{  // pAttachments
						swapchainImageViews[i],
					}.data(),
					newSurfaceExtent.width,  // width
					newSurfaceExtent.height,  // height
					1  // layers
				)
			)
		);

	// rendering finished semaphores
	if(renderingFinishedSemaphores.size() != swapchainImages.size())
	{
		for(auto s : renderingFinishedSemaphores)  device.destroy(s);
		renderingFinishedSemaphores.clear();
		renderingFinishedSemaphores.reserve(swapchainImages.size());
		vk::SemaphoreCreateInfo semaphoreCreateInfo{
			vk::SemaphoreCreateFlags()  // flags
		};
		for(size_t i=0,c=swapchainImages.size(); i<c; i++)
			renderingFinishedSemaphores.emplace_back(
				device.createSemaphore(semaphoreCreateInfo));
	}

	// pipeline
	pipeline.set(
		device.createGraphicsPipeline(
			nullptr,  // pipelineCache
			vk::GraphicsPipelineCreateInfo(
				vk::PipelineCreateFlags(),  // flags

				// shader stages
				2,  // stageCount
				array{  // pStages
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eVertex,  // stage
						vertexShader,  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					},
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eFragment,  // stage
						fragmentShader,  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					},
				}.data(),

				// vertex input
				&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{  // pVertexInputState
					vk::PipelineVertexInputStateCreateFlags(),  // flags
					0,        // vertexBindingDescriptionCount
					nullptr,  // pVertexBindingDescriptions
					0,        // vertexAttributeDescriptionCount
					nullptr   // pVertexAttributeDescriptions
				},

				// input assembly
				&(const vk::PipelineInputAssemblyStateCreateInfo&)vk::PipelineInputAssemblyStateCreateInfo{  // pInputAssemblyState
					vk::PipelineInputAssemblyStateCreateFlags(),  // flags
					vk::PrimitiveTopology::eTriangleList,  // topology
					VK_FALSE  // primitiveRestartEnable
				},

				// tessellation
				nullptr, // pTessellationState

				// viewport
				&(const vk::PipelineViewportStateCreateInfo&)vk::PipelineViewportStateCreateInfo{  // pViewportState
					vk::PipelineViewportStateCreateFlags(),  // flags
					1,  // viewportCount
					array{  // pViewports
						vk::Viewport(0.f, 0.f, float(newSurfaceExtent.width), float(newSurfaceExtent.height), 0.f, 1.f),
					}.data(),
					1,  // scissorCount
					array{  // pScissors
						vk::Rect2D(vk::Offset2D(0,0), newSurfaceExtent)
					}.data(),
				},

				// rasterization
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

				// multisampling
				&(const vk::PipelineMultisampleStateCreateInfo&)vk::PipelineMultisampleStateCreateInfo{  // pMultisampleState
					vk::PipelineMultisampleStateCreateFlags(),  // flags
					vk::SampleCountFlagBits::e1,  // rasterizationSamples
					VK_FALSE,  // sampleShadingEnable
					0.f,       // minSampleShading
					nullptr,   // pSampleMask
					VK_FALSE,  // alphaToCoverageEnable
					VK_FALSE   // alphaToOneEnable
				},

				// depth and stencil
				nullptr,  // pDepthStencilState

				// blending
				&(const vk::PipelineColorBlendStateCreateInfo&)vk::PipelineColorBlendStateCreateInfo{  // pColorBlendState
					vk::PipelineColorBlendStateCreateFlags(),  // flags
					VK_FALSE,  // logicOpEnable
					vk::LogicOp::eClear,  // logicOp
					1,  // attachmentCount
					array{  // pAttachments
						vk::PipelineColorBlendAttachmentState{
							VK_FALSE,  // blendEnable
							vk::BlendFactor::eZero,  // srcColorBlendFactor
							vk::BlendFactor::eZero,  // dstColorBlendFactor
							vk::BlendOp::eAdd,       // colorBlendOp
							vk::BlendFactor::eZero,  // srcAlphaBlendFactor
							vk::BlendFactor::eZero,  // dstAlphaBlendFactor
							vk::BlendOp::eAdd,       // alphaBlendOp
							vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
								vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA  // colorWriteMask
						},
					}.data(),
					array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
				},

				nullptr,  // pDynamicState
				pipeline.layout(),  // layout
				renderPass,  // renderPass
				0,  // subpass
				vk::Pipeline(nullptr),  // basePipelineHandle
				-1 // basePipelineIndex
			)
		)
	);
}


void App::frame(VulkanWindow&)
{
	// wait for previous frame rendering work
	// if still not finished
	// (we might start copy operations before, but we need to exclude TableHandles that must stay intact
	// until the rendering is finished)
	vk::Result r =
		device.waitForFences(
			renderingFinishedFence,  // fences
			VK_TRUE,  // waitAll
			uint64_t(1.5e9)  // timeout
		);
	if(r != vk::Result::eSuccess) {
		if(r == vk::Result::eTimeout)
			throw CadR::Timeout("GPU timeout. Task is probably hanging on GPU.");
		throw runtime_error("Vulkan error: vkWaitForFences failed with error " + to_string(r) + ".");
	}
	device.resetFences(renderingFinishedFence);

	// begin the frame
	renderer.beginFrame();

	// submit all copy operations that were not submitted yet
	renderer.executeCopyOperations();

	// acquire image
	uint32_t imageIndex;
	r =
		device.acquireNextImageKHR(
			swapchain,                // swapchain
			uint64_t(3e9),            // timeout (3s)
			imageAvailableSemaphore,  // semaphore to signal
			vk::Fence(nullptr),       // fence to signal
			&imageIndex               // pImageIndex
		);
	if(r != vk::Result::eSuccess) {
		renderer.endFrame();
		if(r == vk::Result::eSuboptimalKHR) {
			window.scheduleResize();
			return;
		} else if(r == vk::Result::eErrorOutOfDateKHR) {
			window.scheduleResize();
			return;
		} else
			throw runtime_error("Vulkan error: vkAcquireNextImageKHR failed with error " + to_string(r) + ".");
	}

	// begin command buffer recording
	renderer.beginRecording(commandBuffer);

	// prepare recording
	size_t numDrawables = renderer.prepareSceneRendering(stateSet);

	// record compute shader preprocessing
	renderer.recordDrawableProcessing(commandBuffer, numDrawables);

	renderer.recordSceneRendering(
		commandBuffer,  // commandBuffer
		stateSet,  // stateSetRoot
		vk::RenderPassBeginInfo{
			renderPass,  // renderPass
			framebuffers[imageIndex],  // framebuffer
			vk::Rect2D(vk::Offset2D(0, 0), window.surfaceExtent()),  // renderArea
			2,  // clearValueCount
			array<vk::ClearValue,2>{  // pClearValues
				vk::ClearColorValue(array<float,4>{0.f, 0.f, 0.f, 1.f}),
				vk::ClearDepthStencilValue(1.f, 0),
			}.data()
		}
	);

	// end command buffer recording
	renderer.endRecording(commandBuffer);

	// submit all copy operations that were not submitted yet
	renderer.executeCopyOperations();

	// submit frame
	vk::Semaphore renderingFinishedSemaphore = renderingFinishedSemaphores[imageIndex];
	device.queueSubmit(
		graphicsQueue,  // queue
		vk::SubmitInfo(
			1, &imageAvailableSemaphore,  // waitSemaphoreCount + pWaitSemaphores +
			&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(  // pWaitDstStageMask
				vk::PipelineStageFlagBits::eColorAttachmentOutput),
			1, &commandBuffer,  // commandBufferCount + pCommandBuffers
			1, &renderingFinishedSemaphore  // signalSemaphoreCount + pSignalSemaphores
		),
		renderingFinishedFence  // fence
	);

	// present
	r =
		device.presentKHR(
			presentationQueue,  // queue
			&(const vk::PresentInfoKHR&)vk::PresentInfoKHR(  // presentInfo
				1, &renderingFinishedSemaphore,  // waitSemaphoreCount + pWaitSemaphores
				1, &swapchain, &imageIndex,  // swapchainCount + pSwapchains + pImageIndices
				nullptr  // pResults
			)
		);
	if(r != vk::Result::eSuccess) {
		if(r == vk::Result::eSuboptimalKHR) {
			window.scheduleResize();
			cout << "present result: Suboptimal" << endl;
		} else if(r == vk::Result::eErrorOutOfDateKHR) {
			window.scheduleResize();
			cout << "present error: OutOfDate" << endl;
		} else
			throw runtime_error("Vulkan error: vkQueuePresentKHR() failed with error " + to_string(r) + ".");
	}

	// end of the frame
	// (gpu computations might be running asynchronously now
	// and presentation might be waiting for the rendering to finish)
	renderer.endFrame();
}


int main(int argc, char* argv[])
try {

	// set console code page to utf-8 to print non-ASCII characters correctly
#ifdef _WIN32
	if(!SetConsoleOutputCP(CP_UTF8))
		cout << "Failed to set console code page to utf-8." << endl;
#endif

	// init application
	App app(argc, argv);
	app.init();
	app.window.setResizeCallback(
		bind(
			&App::resize,
			&app,
			placeholders::_1,
			placeholders::_2,
			placeholders::_3
		)
	);
	app.window.setFrameCallback(
		bind(&App::frame, &app, placeholders::_1)
	);

	// show window and run main loop
	app.window.show();
	VulkanWindow::mainLoop();

	// finish all pending work on device
	app.device.waitIdle();
	return 0;

// catch exceptions
} catch(CadR::Error &e) {
	cout << "Failed because of CadR exception: " << e.what() << endl;
	return 9;
} catch(vk::Error &e) {
	cout << "Failed because of Vulkan exception: " << e.what() << endl;
	return 9;
} catch(ExitWithMessage &e) {
	cout << e.what() << endl;
	return e.exitCode();
} catch(exception &e) {
	cout << "Failed because of exception: " << e.what() << endl;
	return 9;
} catch(...) {
	cout << "Failed because of unspecified exception." << endl;
	return 9;
}
