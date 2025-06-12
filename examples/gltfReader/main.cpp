#include <CadR/BoundingBox.h>
#include <CadR/BoundingSphere.h>
#include <CadR/Geometry.h>
#include <CadR/Exceptions.h>
#include <CadR/ImageAllocation.h>
#include <CadR/Pipeline.h>
#include <CadR/Sampler.h>
#include <CadR/StagingBuffer.h>
#include <CadR/StagingData.h>
#include <CadR/StateSet.h>
#include <CadR/Texture.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#include <CadR/VulkanLibrary.h>
#include "PipelineLibrary.h"
#include "VulkanWindow.h"
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include "../../3rdParty/stb/stb_image.h"
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN  // reduce amount of included files by windows.h
# include <windows.h>  // needed for SetConsoleOutputCP()
# include <shellapi.h>  // needed for CommandLineToArgvW()
#endif

using namespace std;

using json = nlohmann::json;

typedef logic_error GltfError;


// Shader data structures
struct LightGpuData {
	// we use vec4 instead of vec3 for the purpose of memory alignment; alpha component for light intensities is unused
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;
	glm::vec4 eyePosition;  ///< Light position in eye coordinates.
	glm::vec3 eyePositionDir;  ///< Normalized eyePosition, e.g. direction to light source in eye coordinates.
	uint32_t dummy;  ///< Alignment to 16 byte.
};
constexpr const uint32_t maxLights = 1;
struct SceneGpuData {
	glm::mat4 viewMatrix;    // current camera view matrix
	float p11,p22,p33,p43;   // projectionMatrix - members that depend on zNear and zFar clipping planes
	glm::vec3 ambientLight;  // we use vec4 instead of vec3 for the purpose of memory alignment; alpha component for light intensities is unused
	uint32_t numLights;
	LightGpuData lights[maxLights];
};
static_assert(sizeof(SceneGpuData) == 96+(80*maxLights), "Wrong SceneGpuData data size");


// application class
class App {
public:

	App(int argc, char* argv[]);
	~App();

	void init();
	void resize(VulkanWindow& window,
		const vk::SurfaceCapabilitiesKHR& surfaceCapabilities, vk::Extent2D newSurfaceExtent);
	void frame(VulkanWindow& window);
	void mouseMove(VulkanWindow& window, const VulkanWindow::MouseState& mouseState);
	void mouseButton(VulkanWindow&, size_t button, VulkanWindow::ButtonState buttonState, const VulkanWindow::MouseState& mouseState);
	void mouseWheel(VulkanWindow& window, float wheelX, float wheelY, const VulkanWindow::MouseState& mouseState);
	void key(VulkanWindow& window, VulkanWindow::KeyState keyState, VulkanWindow::ScanCode scanCode);

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
	vk::Format depthFormat;
	float maxSamplerAnisotropy;
	vk::RenderPass renderPass;
	vk::SwapchainKHR swapchain;
	vector<vk::ImageView> swapchainImageViews;
	vector<vk::Framebuffer> framebuffers;
	vk::Image depthImage;
	vk::DeviceMemory depthImageMemory;
	vk::ImageView depthImageView;
	vk::Semaphore imageAvailableSemaphore;
	vk::Semaphore renderFinishedSemaphore;
	vk::Fence renderFinishedFence;

	CadR::Renderer renderer;
	PipelineLibrary pipelineLibrary;
	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;

	// camera control
	static constexpr const float maxZNearZFarRatio = 1000.f;  //< Maximum z-near distance and z-far distance ratio. High ratio might lead to low z-buffer precision.
	static constexpr const float zoomStepRatio = 0.9f;  //< Camera zoom speed. Camera distance from the model center is multiplied by this value on each mouse wheel up event and divided by it on each wheel down event.
	float fovy = 80.f / 180.f * glm::pi<float>();  //< Initial field-of-view in y-axis (in vertical direction) is 80 degrees.
	float cameraHeading = 0.f;
	float cameraElevation = 0.f;
	float cameraDistance;
	float startMouseX, startMouseY;
	float startCameraHeading, startCameraElevation;
	CadR::BoundingSphere sceneBoundingSphere;

	filesystem::path filePath;

	CadR::HandlelessAllocation sceneDataAllocation;
	CadR::StateSet stateSetRoot;
	static constexpr size_t numPipelines = PipelineLibrary::numPipelines();
	struct TextureStateSet {
		CadR::StateSet stateSet;
		CadR::Texture* texture;
	};
	struct PipelineStateSet {
		CadR::Pipeline pipeline;
		CadR::StateSet stateSet;
		list<TextureStateSet> textureStateSetList;
		vector<TextureStateSet*> textureIndexToStateSet;
		PipelineStateSet(CadR::Renderer& r) : pipeline(r), stateSet(r) {}
		void destroy() noexcept  { textureStateSetList.clear(); stateSet.destroy(); }
	};
	array<PipelineStateSet,numPipelines> pipelineStateSetList;
	vector<CadR::Geometry> geometryDB;
	vector<CadR::Drawable> drawableDB;
	vector<CadR::ImageAllocation> imageDB;
	vector<CadR::Sampler> samplerDB;
	vector<CadR::Texture> textureDB;
	CadR::Sampler defaultSampler;

};


class ExitWithMessage {
protected:
	int _exitCode;
	std::string _what;
public:
	ExitWithMessage(int exitCode, const std::string& msg) : _exitCode(exitCode), _what(msg) {}
	ExitWithMessage(int exitCode, const char* msg) : _exitCode(exitCode), _what(msg) {}
	const char* what() const noexcept  { return _what.c_str(); }
	int exitCode() const noexcept  { return _exitCode; }
};


// create_array<T,N>() allows for initialization of an std::array when passing the same value to all the constructors is needed
// (for passing references, use std::ref() and std::cref() to pass them into create_array())
template<typename T, size_t N, size_t index = N, typename T2, typename... Ts>
constexpr array<T, N> createPipelineStateSetList(T2& t, Ts&... ts)
{
	if constexpr (index <= 1)
		return array<T, N>{ t, ts... };
	else
		return createPipelineStateSetList<T, N, index-1>(t, t, ts...);
}


#ifdef _WIN32
static string utf16toUtf8(const wchar_t* ws)
{
	if(ws == nullptr)
		return {};

	// alloc buffer
	unique_ptr<char[]> buffer(new char[wcslen(ws)*3+1]);

	// perform conversion
	size_t pos = 0;
	size_t i = 0;
	char16_t c = ws[0];
	while(c != 0) {
		if((c & 0xfc00) != 0xd800 || (ws[i+1] & 0xfc00) != 0xfc00) {
			if(c < 128)
				// 1 byte utf-8 sequence
				buffer[pos++] = char(c);  // bits 0x7f
			else if(c < 2048) {
				// 2 bytes utf-8 sequence
				buffer[pos++] = 0xc0 | (c >> 6);  // bits 0x07c0
				buffer[pos++] = 0x80 | (c & 0x3f);  // bits 0x3f
			}
			else {
				// 3 bytes utf-8 sequence
				buffer[pos++] = 0xe0 | (c >> 12);  // bits 0xf000
				buffer[pos++] = 0x80 | ((c >> 6) & 0x3f);  // bits 0x0fc0
				buffer[pos++] = 0x80 | (c & 0x3f);  // bits 0x3f
			}
			i++;
			c = ws[i];
		}
		else {
			char16_t cNext = ws[i+1];
			uint32_t codePoint = (uint32_t(c & 0x03ff) << 10) | (cNext & 0x03ff);
			if(codePoint >= 65536) {
				// 4 bytes utf-8 sequence
				buffer[pos++] = 0xf0 | (codePoint >> 18);  // bits 0x1c0000
				buffer[pos++] = 0x80 | ((codePoint >> 12) & 0x3f);  // bits 0x3f000
				buffer[pos++] = 0x80 | ((codePoint >> 6) & 0x3f);  // bits 0x0fc0
				buffer[pos++] = 0x80 | (codePoint & 0x3f);  // bits 0x3f
			}
			else if(codePoint >= 2048) {
				// 3 bytes utf-8 sequence
				buffer[pos++] = 0xe0 | (c >> 12);  // bits 0xf000
				buffer[pos++] = 0x80 | ((c >> 6) & 0x3f);  // bits 0x0fc0
				buffer[pos++] = 0x80 | (c & 0x3f);  // bits 0x3f
			}
			else if(codePoint >= 128) {
				// 2 bytes utf-8 sequence
				buffer[pos++] = 0xc0 | (c >> 6);  // bits 0x07c0
				buffer[pos++] = 0x80 | (c & 0x3f);  // bits 0x3f
			}
			else {
				// 1 byte utf-8 sequence
				buffer[pos++] = char(c);  // bits 0x7f
			}
			i += 2;
			c = ws[i];
		}
	}

	return string(buffer.get(), pos);
}
#endif


/// Construct application object
App::App(int argc, char** argv)
	// none of the following initializators shall be allowed to throw,
	// otherwise a special care must be given to avoid memory leaks in the case of exception
	: sceneDataAllocation(renderer.dataStorage())  // HandlelessAllocation(DataStorage&) does not throw, so it can be here
	, stateSetRoot(renderer)
	, pipelineStateSetList{ createPipelineStateSetList<PipelineStateSet, numPipelines>(renderer) }
	, defaultSampler(renderer)
{
#ifdef _WIN32
	// get wchar_t command line
	LPWSTR commandLine = GetCommandLineW();
	unique_ptr<wchar_t*, void(*)(wchar_t**)> wargv(
		CommandLineToArgvW(commandLine, &argc),
		[](wchar_t** p) { if(LocalFree(p) != 0) assert(0 && "LocalFree() failed."); }
	);
#endif

	// process command-line arguments
	if(argc < 2)
		throw ExitWithMessage(99, "Please, specify glTF file to load.");

#ifdef _WIN32
	filePath = wargv.get()[1];
#else
	filePath = argv[1];
#endif
}


App::~App()
{
	if(device) {

		// wait for device idle state
		// (to prevent errors during destruction of Vulkan resources)
		try {
			device.waitIdle();
		} catch(vk::Error& e) {
			cout << "Failed because of Vulkan exception: " << e.what() << endl;
		}

		// destroy handles
		// (the handles are destructed in certain (not arbitrary) order)
		for(PipelineStateSet& pss : pipelineStateSetList)
			pss.destroy();
		textureDB.clear();
		imageDB.clear();
		samplerDB.clear();
		defaultSampler.destroy();
		drawableDB.clear();
		geometryDB.clear();
		sceneDataAllocation.free();
		pipelineLibrary.destroy();
		device.destroy(commandPool);
		renderer.finalize();
		device.destroy(renderFinishedFence);
		device.destroy(renderFinishedSemaphore);
		device.destroy(imageAvailableSemaphore);
		for(auto f : framebuffers)  device.destroy(f);
		for(auto v : swapchainImageViews)  device.destroy(v);
		device.destroy(depthImage);
		device.freeMemory(depthImageMemory);
		device.destroy(depthImageView);
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
	// open file
	ifstream f(filePath);
	if(!f.is_open()) {
		string msg("Cannot open file ");
		msg.append(filePath.string());
		msg.append(".");
		throw ExitWithMessage(1, msg);
	}
	f.exceptions(ifstream::badbit | ifstream::failbit);

	// init Vulkan and window
	VulkanWindow::init();
	vulkanLib.load(CadR::VulkanLibrary::defaultName());
	vulkanInstance.create(vulkanLib, "glTF reader", 0, "CADR", 0, VK_API_VERSION_1_2, nullptr,
	                      VulkanWindow::requiredExtensions());
	window.create(vulkanInstance, {1024,768}, "glTF reader", vulkanLib.vkGetInstanceProcAddr);

	// init device and renderer
	tuple<vk::PhysicalDevice, uint32_t, uint32_t> deviceAndQueueFamilies =
			vulkanInstance.chooseDevice(
				vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute,  // queueOperations
				window.surface(),  // presentationSurface
				[](CadR::VulkanInstance& instance, vk::PhysicalDevice pd) -> bool {  // filterCallback
					if(instance.getPhysicalDeviceProperties(pd).apiVersion < VK_API_VERSION_1_2)
						return false;
					auto features =
						instance.getPhysicalDeviceFeatures2<
							vk::PhysicalDeviceFeatures2,
							vk::PhysicalDeviceVulkan11Features,
							vk::PhysicalDeviceVulkan12Features>(pd);
					return
						features.get<vk::PhysicalDeviceFeatures2>().features.multiDrawIndirect &&
						features.get<vk::PhysicalDeviceFeatures2>().features.shaderInt64 &&
						features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
						features.get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress;
				});
	physicalDevice = std::get<0>(deviceAndQueueFamilies);
	if(!physicalDevice)
		throw ExitWithMessage(2, "No compatible Vulkan device found.");
	device.create(
		vulkanInstance, deviceAndQueueFamilies,
#if 1 // enable or disable validation extensions
      // (0 enables validation extensions and features for debugging purposes)
		"VK_KHR_swapchain",
		[]() {
			CadR::Renderer::RequiredFeaturesStructChain f = CadR::Renderer::requiredFeaturesStructChain();
			f.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;
			return f;
		}().get<vk::PhysicalDeviceFeatures2>()
#else
		{"VK_KHR_swapchain", "VK_KHR_shader_non_semantic_info"},
		[]() {
			CadR::Renderer::RequiredFeaturesStructChain f = CadR::Renderer::requiredFeaturesStructChain();
			f.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;
			f.get<vk::PhysicalDeviceVulkan12Features>().uniformAndStorageBuffer8BitAccess = true;
			return f;
		}().get<vk::PhysicalDeviceFeatures2>()
#endif
	);
	graphicsQueueFamily = std::get<1>(deviceAndQueueFamilies);
	presentationQueueFamily = std::get<2>(deviceAndQueueFamilies);
	window.setDevice(device, physicalDevice);
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
					throw std::runtime_error("Vulkan error: getSurfaceFormatsKHR() returned empty list.");
				return availableFormats[0];
			}
		}(physicalDevice, vulkanInstance, window.surface());

	// choose depth format
	depthFormat =
		[](vk::PhysicalDevice physicalDevice, CadR::VulkanInstance& vulkanInstance)
		{
			constexpr const array<vk::Format, 3> candidateFormats {
				vk::Format::eD32Sfloat,
				vk::Format::eD32SfloatS8Uint,
				vk::Format::eD24UnormS8Uint,
			};
			for(vk::Format f : candidateFormats) {
				vk::FormatProperties p = physicalDevice.getFormatProperties(f, vulkanInstance);
				if(p.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
					return f;
			}
			throw std::runtime_error("No suitable depth buffer format.");
		}(physicalDevice, vulkanInstance);

	// maxSamplerAnisotropy
	maxSamplerAnisotropy = vulkanInstance.getPhysicalDeviceProperties(physicalDevice).limits.maxSamplerAnisotropy;

	// render pass
	renderPass =
		device.createRenderPass(
			vk::RenderPassCreateInfo(
				vk::RenderPassCreateFlags(),  // flags
				2,                            // attachmentCount
				array<const vk::AttachmentDescription, 2>{  // pAttachments
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
					},
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
							vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader |
							vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests |
							vk::PipelineStageFlagBits::eColorAttachmentOutput),
						vk::AccessFlags(vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eTransferWrite),  // srcAccessMask
						vk::AccessFlags(  // dstAccessMask
							vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead |
							vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentWrite |
							vk::AccessFlagBits::eDepthStencilAttachmentWrite),
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

	// rendering semaphores and fences
	imageAvailableSemaphore =
		device.createSemaphore(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()  // flags
			)
		);
	renderFinishedSemaphore =
		device.createSemaphore(
			vk::SemaphoreCreateInfo(
				vk::SemaphoreCreateFlags()  // flags
			)
		);
	renderFinishedFence =
		device.createFence(
			vk::FenceCreateInfo(
				vk::FenceCreateFlagBits::eSignaled  // flags
			)
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

	// pipelineStateSetList
	pipelineLibrary.create(device);
	for(size_t i=0; i<pipelineStateSetList.size(); i++) {
		PipelineStateSet& pss = pipelineStateSetList[i];
		CadR::Pipeline& p = pss.pipeline;
		vk::PipelineLayout pipelineLayout =
			(i & 0x02) ? pipelineLibrary.texturePipelineLayout() : pipelineLibrary.pipelineLayout();
		p.init(nullptr, pipelineLayout, nullptr);
		CadR::StateSet& s = pss.stateSet;
		s.pipeline = &p;
		s.pipelineLayout = pipelineLayout;
		stateSetRoot.childList.append(s);
	}

	// parse json
#ifdef _WIN32
	cout << "Processing file " << utf16toUtf8(filePath.c_str()) << "..." << endl;
#else
	cout << "Processing file " << filePath.native() << "..." << endl;
#endif
	json glTF, newGltfItems;
	f >> glTF;
	f.close();

	// read root objects
	// (asset item is mandatory, the rest is optional)
	auto getRootItem =
		[](json& glTF, json& newGltfItems, const string& key) -> json::array_t&
		{
			auto it = glTF.find(key);
			if(it != glTF.end())
				return it->get_ref<json::array_t&>();
			auto& ref = newGltfItems[key];
			if(ref.is_null())
				ref = json::array();
			return ref.get_ref<json::array_t&>();
		};
	auto& asset = glTF.at("asset");
	auto& scenes = getRootItem(glTF, newGltfItems, "scenes");
	auto& nodes = getRootItem(glTF, newGltfItems, "nodes");
	auto& meshes = getRootItem(glTF, newGltfItems, "meshes");
	auto& accessors = getRootItem(glTF, newGltfItems, "accessors");
	auto& buffers = getRootItem(glTF, newGltfItems, "buffers");
	auto& bufferViews = getRootItem(glTF, newGltfItems, "bufferViews");
	auto& materials = getRootItem(glTF, newGltfItems, "materials");
	auto& textures = getRootItem(glTF, newGltfItems, "textures");
	auto& images = getRootItem(glTF, newGltfItems, "images");
	auto& samplers = getRootItem(glTF, newGltfItems, "samplers");

	// print glTF info
	// (version item is mandatory, the rest is optional)
	auto getStringWithDefault =
		[](json& node, const string& key, const string& defaultVal)
		{
			auto it = node.find(key);
			return (it != node.end())
				? it->get_ref<json::string_t&>()
				: defaultVal;
		};
	cout << endl;
	cout << "glTF info:" << endl;
	cout << "   Version:     " << asset.at("version").get_ref<json::string_t&>() << endl;
	cout << "   MinVersion:  " << getStringWithDefault(asset, "minVersion", "< none >") << endl;
	cout << "   Generator:   " << getStringWithDefault(asset, "generator", "< none >") << endl;
	cout << "   Copyright:   " << getStringWithDefault(asset, "copyright", "< none >") << endl;
	cout << endl;

	// print stats
	cout << "Stats:" << endl;
	cout << "   Scenes:    " << scenes.size() << endl;
	cout << "   Nodes:     " << nodes.size() << endl;
	cout << "   Meshes:    " << meshes.size() << endl;
	cout << "   Textures:  " << textures.size() << endl;
	cout << endl;

	// read buffers
	vector<vector<uint8_t>> bufferDataList;
	bufferDataList.reserve(buffers.size());
	for(auto& b : buffers) {

		// get path
		auto uriIt = b.find("uri");
		if(uriIt == b.end())
			throw GltfError("Unsupported functionality: Undefined buffer.uri.");
		const string& s = uriIt->get_ref<json::string_t&>();
		filesystem::path p = u8string_view(reinterpret_cast<const char8_t*>(s.data()), s.size());
		if(p.is_relative())
			p = filePath.parent_path() / p;

		// open file
		cout << "Opening buffer " << s << "..." << endl;
		ifstream f(p, ios::in|ios::binary);
		if(!f.is_open())
			throw GltfError("Error opening file " + p.string() + ".");
		f.exceptions(ifstream::badbit | ifstream::failbit);

		// read content
		size_t size = filesystem::file_size(p);
		auto& bufferData = bufferDataList.emplace_back(size);
		f.read(reinterpret_cast<char*>(bufferData.data()), size);
		f.close();
	}

	// read nodes
	struct Node {
		vector<size_t> children;
		glm::mat4 matrix;
		size_t meshIndex;
	};
	vector<Node> nodeList(nodes.size());
	for(size_t i=0,c=nodes.size(); i<c; i++) {

		// get references
		json::object_t& jobj = nodes[i].get_ref<json::object_t&>();
		Node& node = nodeList[i];

		// matrix
		if(auto it = jobj.find("matrix"); it != jobj.end()) {

			// translation, rotation and scale must not be present
			if(jobj.find("translation") != jobj.end() || jobj.find("rotation") != jobj.end() ||
			   jobj.find("scale") != jobj.end())
				throw GltfError("If matrix is provided for the node, translation, rotation and scale must not be present.");

			// read matrix
			json::array_t& a = it->second.get_ref<json::array_t&>();
			if(a.size() != 16)
				throw GltfError("Node.matrix is not vector of 16 components.");
			float* f = glm::value_ptr(node.matrix);
			for(unsigned j=0; j<16; j++)
				f[j] = float(a[j].get<json::number_float_t>());
		}
		else {

			// read scale
			glm::vec3 scale;
			if(auto it = jobj.find("scale"); it != jobj.end()) {
				json::array_t& a = it->second.get_ref<json::array_t&>();
				if(a.size() != 3)
					throw GltfError("Node.scale is not vector of three components.");
				scale.x = float(a[0].get<json::number_float_t>());
				scale.y = float(a[1].get<json::number_float_t>());
				scale.z = float(a[2].get<json::number_float_t>());
			}
			else
				scale = { 1.f, 1.f, 1.f };

			// read rotation
			if(auto it = jobj.find("rotation"); it != jobj.end()) {
				json::array_t& a = it->second.get_ref<json::array_t&>();
				if(a.size() != 4)
					throw GltfError("Node.rotation is not vector of four components.");
				glm::quat q;
				q.x = float(a[0].get<json::number_float_t>());
				q.y = float(a[1].get<json::number_float_t>());
				q.z = float(a[2].get<json::number_float_t>());
				q.w = float(a[3].get<json::number_float_t>());
				glm::mat3 m = glm::mat3(q);
				node.matrix[0] = glm::vec4(m[0] * scale.x, 0.f);
				node.matrix[1] = glm::vec4(m[1] * scale.y, 0.f);;
				node.matrix[2] = glm::vec4(m[2] * scale.z, 0.f);;
				node.matrix[3] = glm::vec4(0.f, 0.f, 0.f, 1.f);
			}
			else {
				// initialize matrix by scale only
				memset(&node.matrix, 0, sizeof(node.matrix));
				node.matrix[0][0] = scale.x;
				node.matrix[1][1] = scale.y;
				node.matrix[2][2] = scale.z;
				node.matrix[3][3] = 1.f;
			}

			// read translation
			if(auto it = jobj.find("translation"); it != jobj.end()) {
				json::array_t& a = it->second.get_ref<json::array_t&>();
				if(a.size() != 3)
					throw GltfError("Node.translation is not vector of three components.");
				node.matrix[3][0] = float(a[0].get<json::number_float_t>());
				node.matrix[3][1] = float(a[1].get<json::number_float_t>());
				node.matrix[3][2] = float(a[2].get<json::number_float_t>());
			}

		}

		// read children
		if(auto it = jobj.find("children"); it != jobj.end()) {
			json::array_t& a = it->second.get_ref<json::array_t&>();
			size_t size = a.size();
			node.children.resize(size);
			for(unsigned j=0; j<size; j++)
				node.children[j] = size_t(a[j].get_ref<json::number_unsigned_t&>());
		}

		// read mesh index
		if(auto it = jobj.find("mesh"); it != jobj.end())
			node.meshIndex = size_t(it->second.get_ref<json::number_unsigned_t&>());
		else
			node.meshIndex = ~size_t(0);
	}

	// get default scene
	size_t numScenes = scenes.size();
	if(numScenes == 0)
		return;
	size_t defaultSceneIndex = glTF.value<json::number_unsigned_t>("scene", ~size_t(0));
	if(defaultSceneIndex == ~size_t(0)) {
		cout << "There is no default scene in the file. Using the first scene." << endl;
		defaultSceneIndex = 0;
	}
	json& scene = scenes.at(defaultSceneIndex);

	// iterate through root nodes
	vector<vector<glm::mat4>> meshMatrixList(meshes.size());
	if(auto rootNodesIt=scene.find("nodes"); rootNodesIt!=scene.end()) {
		json& rootNodes = *rootNodesIt;
		for(auto it=rootNodes.begin(); it!= rootNodes.end(); it++) {

			// process node function
			auto processNode =
				[](size_t nodeIndex, const glm::mat4& parentMatrix, vector<Node>& nodeList,
				   vector<vector<glm::mat4>>& meshMatrixList, const auto& processNode) -> void
				{
					// get node
					Node& node = nodeList.at(nodeIndex);

					// compute local matrix
					glm::mat4 m = parentMatrix * node.matrix;

					// assign one more instancing matrix to the mesh
					if(node.meshIndex != ~size_t(0))
						meshMatrixList.at(node.meshIndex).emplace_back(m);

					// process children
					for(size_t i=0,c=node.children.size(); i<c; i++)
						processNode(node.children[i], m, nodeList, meshMatrixList, processNode);
				};

			// get node
			size_t rootNodeIndex = size_t(it->get_ref<json::number_unsigned_t&>());
			Node& node = nodeList.at(rootNodeIndex);

			// compute root matrix
			// (we need to flip y and z axes to get from glTF coordinate system to Vulkan coordinate system)
			glm::mat4 m = node.matrix;
			for(unsigned i=0; i<4; i++) {
				m[i][1] = -m[i][1];
				m[i][2] = -m[i][2];
			}

			// append instancing matrix to the mesh
			if(node.meshIndex != ~size_t(0))
				meshMatrixList.at(node.meshIndex).emplace_back(m);

			// process children
			for(size_t i=0,c=node.children.size(); i<c; i++)
				processNode(node.children[i], m, nodeList, meshMatrixList, processNode);

		}
	}

	// process images
	vector<vk::Format> imageFormats;
	if(!images.empty()) {

		// is R8G8B8Srgb format supported?
		vk::FormatProperties fp = vulkanInstance.getPhysicalDeviceFormatProperties(
			physicalDevice, vk::Format::eR8G8B8Srgb);
		bool rgb8srgbSupported =
			(fp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear) &&
			(fp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferDst);
		fp = vulkanInstance.getPhysicalDeviceFormatProperties(physicalDevice, vk::Format::eR8G8Srgb);
		bool rg8srgbSupported =
			(fp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear) &&
			(fp.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferDst);

		// load images
		size_t c = images.size();
		cout << "Processing images (" << c << " in total)..." << endl;
		imageDB.reserve(c);
		imageFormats.resize(c, vk::Format::eUndefined);
		for(size_t i=0; i<c; i++) {
			auto& image = images[i];
			auto uriIt = image.find("uri");
			if(uriIt != image.end()) {

				// image file name
				const string& imageFileName = uriIt->get_ref<json::string_t&>();
				cout << "   " << imageFileName;
				filesystem::path p = u8string_view(reinterpret_cast<const char8_t*>(imageFileName.data()), imageFileName.size());
				if(p.is_relative())
					p = filePath.parent_path() / p;

				// open stream
				ifstream fs(p, ios_base::in | ios_base::binary);
				if(!fs)
					goto failed;
				else {

					// file size
					fs.seekg(0, ios_base::end);
					size_t fileSize = fs.tellg();
					fs.seekg(0, ios_base::beg);

					// read file content
					unique_ptr<unsigned char[]> imgBuffer = make_unique<unsigned char[]>(fileSize);
					fs.read(reinterpret_cast<ifstream::char_type*>(imgBuffer.get()), fileSize);
					if(!fs)
						goto failed;
					fs.close();

					// image info
					int width, height, numComponents;
					if(!stbi_info_from_memory(imgBuffer.get(), int(fileSize), &width, &height, &numComponents))
						goto failed;
					vk::Format format;
					size_t alignment;
					switch(numComponents) {
					case 4: // red, green, blue, alpha
						format = vk::Format::eR8G8B8A8Srgb;
						alignment = 4;
						break;
					case 3: // red, green, blue
						if(rgb8srgbSupported) {
							format = vk::Format::eR8G8B8Srgb;
							alignment = 3;
						}
						else {
							// fallback to alpha always one
							format = vk::Format::eR8G8B8A8Srgb;
							alignment = 4;
							numComponents = 4;
						}
						break;
					case 2: // grey, alpha
						if(rg8srgbSupported) {
							format = vk::Format::eR8G8Srgb;
							alignment = 2;
						}
						else {
							// fallback to expand grey into red+green+blue
							format = vk::Format::eR8G8B8A8Srgb;
							alignment = 4;
							numComponents = 4;
						}
						break;
					default: goto failed;
					}

					// load image
					unique_ptr<stbi_uc[], void(*)(stbi_uc*)> data(
						stbi_load_from_memory(imgBuffer.get(), int(fileSize),
							&width, &height, nullptr, numComponents),
						[](stbi_uc* ptr) { stbi_image_free(ptr); }
					);
					if(data == nullptr)
						goto failed;

					// copy data to staging buffer
					size_t bufferSize = size_t(width) * height * numComponents;
					CadR::StagingBuffer sb(renderer.imageStorage(), bufferSize, alignment);
					memcpy(sb.data(), data.get(), bufferSize);
					data.reset();

					// create ImageAllocation
					CadR::ImageAllocation& a = imageDB.emplace_back(renderer.imageStorage());
					imageFormats[imageDB.size()-1] = format;
					a.alloc(
						vk::MemoryPropertyFlagBits::eDeviceLocal,  // requiredFlags
						vk::ImageCreateInfo(  // imageCreateInfo
							vk::ImageCreateFlags{},  // flags
							vk::ImageType::e2D,  // imageType
							format,  // format
							vk::Extent3D(width, height, 1),  // extent
							1,  // mipLevels
							1,  // arrayLayers
							vk::SampleCountFlagBits::e1,  // samples
							vk::ImageTiling::eOptimal,  // tiling
							vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,  // usage
							vk::SharingMode::eExclusive,  // sharingMode
							0,  // queueFamilyIndexCount
							nullptr,  // pQueueFamilyIndices
							vk::ImageLayout::eUndefined  // initialLayout
						),
						device  // vulkanDevice
					);
					sb.submit(
						a,  // ImageAllocation
						vk::ImageLayout::eUndefined,  // currentLayout,
						vk::ImageLayout::eTransferDstOptimal,  // copyLayout,
						vk::ImageLayout::eShaderReadOnlyOptimal,  // newLayout,
						vk::PipelineStageFlagBits::eFragmentShader,  // newLayoutBarrierDstStages,
						vk::AccessFlagBits::eShaderRead,  // newLayoutBarrierDstAccessFlags,
						vk::Extent2D(width, height),  // imageExtent
						bufferSize  // dataSize
					);

				}
				goto succeed;
			failed:
				cout << " - failed" << endl;
				throw GltfError("Failed to load texture " + imageFileName + ".");
			succeed:
				cout << endl;
			}
		}
	}

	// process image samplers
	if(!samplers.empty()) {

		// default settings for samplers
		vk::SamplerCreateInfo samplerCreateInfo(
			vk::SamplerCreateFlags(),  // flags
			vk::Filter::eNearest,  // magFilter - will be set later
			vk::Filter::eNearest,  // minFilter - will be set later
			vk::SamplerMipmapMode::eNearest,  // mipmapMode - will be set later
			vk::SamplerAddressMode::eRepeat,  // addressModeU - will be set later
			vk::SamplerAddressMode::eRepeat,  // addressModeV - will be set later
			vk::SamplerAddressMode::eRepeat,  // addressModeW
			0.f,  // mipLodBias
			VK_TRUE,  // anisotropyEnable
			maxSamplerAnisotropy,  // maxAnisotropy
			VK_FALSE,  // compareEnable
			vk::CompareOp::eNever,  // compareOp
			0.f,  // minLod
			0.f,  // maxLod
			vk::BorderColor::eFloatTransparentBlack,  // borderColor
			VK_FALSE  // unnormalizedCoordinates
		);

		// read image samplers
		size_t c = samplers.size();
		samplerDB.reserve(c);
		for(size_t i=0; i<c; i++) {
			auto& sampler = samplers[i];

			// magFilter
			auto magFilterIt = sampler.find("magFilter");
			if(magFilterIt != sampler.end()) {
				switch(magFilterIt->get_ref<json::number_unsigned_t&>()) {
				case 9728:  // GL_NEAREST
					samplerCreateInfo.magFilter = vk::Filter::eNearest;
					break;
				case 9729:  // GL_LINEAR
					samplerCreateInfo.magFilter = vk::Filter::eLinear;
					break;
				default:
					throw GltfError("Sampler.magFilter contains invalid value.");
				}
			}
			else {
				// no defaults specified in glTF 2.0 spec
				samplerCreateInfo.magFilter = vk::Filter::eNearest;
			}

			// minFilter
			auto minFilterIt = sampler.find("minFilter");
			if(minFilterIt != sampler.end()) {
				switch(minFilterIt->get_ref<json::number_unsigned_t&>()) {
				case 9728:  // GL_NEAREST
					samplerCreateInfo.minFilter = vk::Filter::eNearest;
					samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;  // probably highest texture detail should be chosen here
					break;
				case 9729:  // GL_LINEAR
					samplerCreateInfo.minFilter = vk::Filter::eLinear;
					samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;  // probably highest texture detail should be chosen here
					break;
				case 9984:  // GL_NEAREST_MIPMAP_NEAREST
					samplerCreateInfo.minFilter = vk::Filter::eNearest;
					samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
					break;
				case 9985:  // GL_LINEAR_MIPMAP_NEAREST
					samplerCreateInfo.minFilter = vk::Filter::eLinear;
					samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
					break;
				case 9986:  // GL_NEAREST_MIPMAP_LINEAR
					samplerCreateInfo.minFilter = vk::Filter::eNearest;
					samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
					break;
				case 9987: // GL_LINEAR_MIPMAP_LINEAR
					samplerCreateInfo.minFilter = vk::Filter::eLinear;
					samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
					break;
				default:
					throw GltfError("Sampler.minFilter contains invalid value.");
				}
			}
			else {
				// no defaults specified in glTF 2.0 spec
				samplerCreateInfo.minFilter = vk::Filter::eNearest;
				samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
			}

			// wrapS
			auto wrapSIt = sampler.find("wrapS");
			if(wrapSIt != sampler.end()) {
				switch(wrapSIt->get_ref<json::number_unsigned_t&>()) {
				case 33071:  // GL_CLAMP_TO_EDGE
					samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
					break;
				case 33648:  // GL_MIRRORED_REPEAT
					samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eMirroredRepeat;
					break;
				case 10497:  // GL_REPEAT
					samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
					break;
				default:
					throw GltfError("Sampler.wrapS contains invalid value.");
				}
			}
			else  // default is GL_REPEAT
				samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;

			// wrapT
			auto wrapTIt = sampler.find("wrapT");
			if(wrapTIt != sampler.end()) {
				switch(wrapTIt->get_ref<json::number_unsigned_t&>()) {
				case 33071:  // GL_CLAMP_TO_EDGE
					samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
					break;
				case 33648:  // GL_MIRRORED_REPEAT
					samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eMirroredRepeat;
					break;
				case 10497:  // GL_REPEAT
					samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
					break;
				default:
					throw GltfError("Sampler.wrapT contains invalid value.");
				}
			}
			else  // default is GL_REPEAT
				samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;

			// create sampler
			samplerDB.emplace_back(renderer, samplerCreateInfo);

		}
	}

	// process textures
	if(!textures.empty()) {
		size_t c = textures.size();
		textureDB.reserve(c);
		for(size_t i=0; i<c; i++) {
			auto& texture = textures[i];
			auto sourceIt = texture.find("source");
			if(sourceIt == texture.end())
				throw GltfError("Unsupported functionality: Texture.source is not defined for the texture.");
			size_t imageIndex = sourceIt->get_ref<json::number_unsigned_t&>();
			auto samplerIt = texture.find("sampler");
			vk::Sampler vkSampler;
			if(samplerIt != texture.end()) {
				size_t samplerIndex = samplerIt->get_ref<json::number_unsigned_t&>();
				vkSampler = samplerDB.at(samplerIndex).handle();
			}
			else {
				if(!defaultSampler.handle()) {
					defaultSampler.create(
						vk::SamplerCreateInfo(
							vk::SamplerCreateFlags(),  // flags
							vk::Filter::eNearest,  // magFilter - glTF specifies to use "auto filtering", but what is that?
							vk::Filter::eNearest,  // minFilter - glTF specifies to use "auto filtering", but what is that?
							vk::SamplerMipmapMode::eNearest,  // mipmapMode
							vk::SamplerAddressMode::eRepeat,  // addressModeU
							vk::SamplerAddressMode::eRepeat,  // addressModeV
							vk::SamplerAddressMode::eRepeat,  // addressModeW
							0.f,  // mipLodBias
							VK_TRUE,  // anisotropyEnable
							maxSamplerAnisotropy,  // maxAnisotropy
							VK_FALSE,  // compareEnable
							vk::CompareOp::eNever,  // compareOp
							0.f,  // minLod
							0.f,  // maxLod
							vk::BorderColor::eFloatTransparentBlack,  // borderColor
							VK_FALSE  // unnormalizedCoordinates
						)
					);
				}
				vkSampler = defaultSampler.handle();
			}

			// create texture
			textureDB.emplace_back(
				imageDB.at(imageIndex), // imageAllocation
				vk::ImageViewCreateInfo(  // imageViewCreateInfo
					vk::ImageViewCreateFlags(),  // flags
					nullptr,  // image - will be filled in later
					vk::ImageViewType::e2D,  // viewType
					imageFormats.at(imageIndex),  // format
					vk::ComponentMapping{  // components
						vk::ComponentSwizzle::eR,
						vk::ComponentSwizzle::eG,
						vk::ComponentSwizzle::eB,
						vk::ComponentSwizzle::eA,
					},
					vk::ImageSubresourceRange{  // subresourceRange
						vk::ImageAspectFlagBits::eColor,  // aspectMask
						0,  // baseMipLevel
						1,  // levelCount
						0,  // baseArrayLayer
						1,  // layerCount
					}
				),
				vkSampler,  // sampler
				device  // device
			);
		}
	}

	// process meshes
	cout << "Processing meshes..." << endl;
	size_t numMeshes = meshes.size();
	vector<CadR::BoundingSphere> meshBoundingSphereList(numMeshes);
	vector<CadR::BoundingSphere> primitiveSetBSList;
	for(size_t meshIndex=0; meshIndex<numMeshes; meshIndex++) {

		// ignore non-instanced meshes
		if(meshMatrixList[meshIndex].empty())
			continue;

		// get mesh
		auto& mesh = meshes[meshIndex];
		CadR::BoundingBox meshBB = CadR::BoundingBox::empty();

		// process primitives
		// (mesh.primitives are mandatory)
		auto& primitives = mesh.at("primitives");
		primitiveSetBSList.clear();
		primitiveSetBSList.reserve(primitives.size());
		for(auto& primitive : primitives) {

			// material
			// (mesh.primitive.material is optional)
			json* material;
			bool doubleSided;
			glm::vec4 baseColorFactor;
			size_t baseColorTextureIndex;
			//unsigned baseColorTextureCoord; <- not used yet
			float metallicFactor;
			float roughnessFactor;
			glm::vec3 emissiveFactor;
			auto it = primitive.find("material");
			if(it == primitive.end()) {
				material = nullptr;
				doubleSided = false;
				baseColorTextureIndex = ~size_t(0);
				//baseColorTextureCoord = ~0; <- not used yet
			}
			else {
				size_t materialIndex = it->get_ref<json::number_unsigned_t&>();
				material = &materials.at(materialIndex);

				// material.doubleSided is optional with the default value of false
				doubleSided = material->value<json::boolean_t>("doubleSided", false);

				// read pbr material properties
				if(auto pbrIt = material->find("pbrMetallicRoughness"); pbrIt != material->end()) {

					// read baseColorFactor
					if(auto baseColorFactorIt = pbrIt->find("baseColorFactor"); baseColorFactorIt != pbrIt->end()) {
						json::array_t& a = baseColorFactorIt->get_ref<json::array_t&>();
						if(a.size() != 4)
							throw GltfError("Material.pbrMetallicRoughness.baseColorFactor is not vector of four components.");
						baseColorFactor[0] = float(a[0].get<json::number_float_t>());
						baseColorFactor[1] = float(a[1].get<json::number_float_t>());
						baseColorFactor[2] = float(a[2].get<json::number_float_t>());
						baseColorFactor[3] = float(a[3].get<json::number_float_t>());
					}
					else
						baseColorFactor = glm::vec4(1.f, 1.f, 1.f, 1.f);

					// read properties
					metallicFactor = float(pbrIt->value<json::number_float_t>("metallicFactor", 1.0));
					roughnessFactor = float(pbrIt->value<json::number_float_t>("roughnessFactor", 1.0));

					// not supported properties
					if(auto baseColorTextureIt = pbrIt->find("baseColorTexture"); baseColorTextureIt != pbrIt->end()) {
						baseColorTextureIndex = baseColorTextureIt->at("index").get_ref<json::number_unsigned_t&>();
						if(baseColorTextureIndex >= textureDB.size())
							throw GltfError("baseColorTexture.index is out of range. It is not index to a valid texture.");
						//baseColorTextureCoord = baseColorTextureIt->value("texCoord", 0); <- not used yet

					}
					else {
						baseColorTextureIndex = ~size_t(0);
						//baseColorTextureCoord = ~0; <- not used yet
					}
					if(pbrIt->find("metallicRoughnessTexture") != pbrIt->end())
						throw GltfError("Unsupported functionality: metallic-roughness texture.");

				}
				else
				{
					// default values when pbrMetallicRoughness is not present
					baseColorFactor = glm::vec4(1.f, 1.f, 1.f, 1.f);
					baseColorTextureIndex = ~size_t(0);
					//baseColorTextureCoord = 0; <- not used yet
					metallicFactor = 1.f;
					roughnessFactor = 1.f;
				}

				// read emissiveFactor
				if(auto emissiveFactorIt = material->find("emissiveFactor"); emissiveFactorIt != material->end()) {
					json::array_t& a = emissiveFactorIt->get_ref<json::array_t&>();
					if(a.size() != 3)
						throw GltfError("Material.emissiveFactor is not vector of three components.");
					emissiveFactor[0] = float(a[0].get<json::number_float_t>());
					emissiveFactor[1] = float(a[1].get<json::number_float_t>());
					emissiveFactor[2] = float(a[2].get<json::number_float_t>());
				}
				else
					emissiveFactor = glm::vec3(0.f, 0.f, 0.f);

				// not supported material properties
				if(material->find("normalTexture") != material->end())
					throw GltfError("Unsupported functionality: normal texture.");
				if(material->find("occlusionTexture") != material->end())
					throw GltfError("Unsupported functionality: occlusion texture.");
				if(material->find("emissiveTexture") != material->end())
					throw GltfError("Unsupported functionality: emissive texture.");
				if(material->find("alphaMode") != material->end())
					throw GltfError("Unsupported functionality: alpha mode.");
				if(material->find("alphaCutoff") != material->end())
					throw GltfError("Unsupported functionality: alpha cutoff.");
			}

			// mesh.primitive helper functions
			auto getColorFromVec4f =
				[](uint8_t* srcPtr) -> glm::vec4 {
					glm::vec4 r = *reinterpret_cast<glm::vec4*>(srcPtr);
					return glm::clamp(r, 0.f, 1.f);
				};
			auto getColorFromVec3f =
				[](uint8_t* srcPtr) -> glm::vec4 {
					return
						glm::vec4(
							glm::clamp(*reinterpret_cast<glm::vec3*>(srcPtr), 0.f, 1.f),
							1.f
						);
				};
			auto getColorFromVec4us =
				[](uint8_t* srcPtr) -> glm::vec4 {
					return
						glm::vec4(
							float(reinterpret_cast<uint16_t*>(srcPtr)[0]) / 65535.f,
							float(reinterpret_cast<uint16_t*>(srcPtr)[1]) / 65535.f,
							float(reinterpret_cast<uint16_t*>(srcPtr)[2]) / 65535.f,
							float(reinterpret_cast<uint16_t*>(srcPtr)[3]) / 65535.f
						);
				};
			auto getColorFromVec3us =
				[](uint8_t* srcPtr) -> glm::vec4 {
					return
						glm::vec4(
							float(reinterpret_cast<uint16_t*>(srcPtr)[0]) / 65535.f,
							float(reinterpret_cast<uint16_t*>(srcPtr)[1]) / 65535.f,
							float(reinterpret_cast<uint16_t*>(srcPtr)[2]) / 65535.f,
							1.f
						);
				};
			auto getColorFromVec4ub =
				[](uint8_t* srcPtr) -> glm::vec4 {
					return
						glm::vec4(
							float(reinterpret_cast<uint8_t*>(srcPtr)[0]) / 255.f,
							float(reinterpret_cast<uint8_t*>(srcPtr)[1]) / 255.f,
							float(reinterpret_cast<uint8_t*>(srcPtr)[2]) / 255.f,
							float(reinterpret_cast<uint8_t*>(srcPtr)[3]) / 255.f
						);
				};
			auto getColorFromVec3ub =
				[](uint8_t* srcPtr) -> glm::vec4 {
					return
						glm::vec4(
							float(reinterpret_cast<uint8_t*>(srcPtr)[0]) / 255.f,
							float(reinterpret_cast<uint8_t*>(srcPtr)[1]) / 255.f,
							float(reinterpret_cast<uint8_t*>(srcPtr)[2]) / 255.f,
							1.f
						);
				};
			auto getTexCoordFromVec2f =
				[](uint8_t* srcPtr) -> glm::vec4 {
					return
						glm::vec4(
							*reinterpret_cast<glm::vec2*>(srcPtr),
							0.f,
							0.f
						);
				};
			auto getTexCoordFromVec2us =
				[](uint8_t* srcPtr) -> glm::vec4 {
					return
						glm::vec4(
							float(reinterpret_cast<uint16_t*>(srcPtr)[0]) / 65535.f,
							float(reinterpret_cast<uint16_t*>(srcPtr)[1]) / 65535.f,
							0.f,
							0.f
						);
				};
			auto getTexCoordFromVec2ub =
				[](uint8_t* srcPtr) -> glm::vec4 {
					return
						glm::vec4(
							float(reinterpret_cast<uint8_t*>(srcPtr)[0]) / 255.f,
							float(reinterpret_cast<uint8_t*>(srcPtr)[1]) / 255.f,
							0.f,
							0.f
						);
				};
			auto updateNumVertices =
				[](json& accessor, size_t& numVertices) -> void
				{
					// get position count (accessor.count is mandatory and >=1)
					json::number_unsigned_t count = accessor.at("count").get_ref<json::number_unsigned_t&>();

					// update numVertices if still set to 0
					if(numVertices != count) {
						if(numVertices == 0) {
							if(count != 0)
								numVertices = count;
							else
								throw GltfError("Accessor's count member must be greater than zero.");
						}
						else
							throw GltfError("Number of elements is not the same for all primitive attributes.");
					}
				};
			auto getDataPointerAndStride =
				[](json& accessor, json::array_t& bufferViews, json::array_t& buffers, vector<vector<uint8_t>>& bufferDataList,
				   size_t numElements, size_t elementSize) -> tuple<void*, unsigned>
				{
					// accessor.sparse is not supported yet
					if(accessor.find("sparse") != accessor.end())
						throw GltfError("Unsupported functionality: Property sparse.");

					// get accessor.bufferView (it is optional)
					auto bufferViewIt = accessor.find("bufferView");
					if(bufferViewIt == accessor.end())
						throw GltfError("Unsupported functionality: Omitted bufferView.");
					auto& bufferView = bufferViews.at(bufferViewIt->get_ref<json::number_unsigned_t&>());

					// bufferView.byteStride (it is optional (but mandatory in some cases), if not provided, data are tightly packed)
					unsigned stride = unsigned(bufferView.value<json::number_unsigned_t>("byteStride", elementSize));
					size_t dataSize = (numElements-1) * stride + elementSize;

					// get accessor.byteOffset (it is optional with default value 0)
					json::number_unsigned_t offset = accessor.value<json::number_unsigned_t>("byteOffset", 0);

					// make sure we not run over bufferView.byteLength (byteLength is mandatory and >=1)
					if(offset + dataSize > bufferView.at("byteLength").get_ref<json::number_unsigned_t&>())
						throw GltfError("Accessor range is not completely inside its BufferView.");

					// append bufferView.byteOffset (byteOffset is optional with default value 0)
					offset += bufferView.value<json::number_unsigned_t>("byteOffset", 0);

					// get bufferView.buffer (buffer is mandatory)
					size_t bufferIndex = bufferView.at("buffer").get_ref<json::number_unsigned_t&>();

					// get buffer
					auto& buffer = buffers.at(bufferIndex);

					// make sure we do not run over buffer.byteLength (byteLength is mandatory)
					if(offset + dataSize > buffer.at("byteLength").get_ref<json::number_unsigned_t&>())
						throw GltfError("BufferView range is not completely inside its Buffer.");

					// return pointer to buffer data and data stride
					auto& bufferData = bufferDataList[bufferIndex];
					if(offset + dataSize > bufferData.size())
						throw GltfError("BufferView range is not completely inside data range.");
					return { bufferData.data() + offset, stride };
				};

			// attributes (mesh.primitive.attributes is mandatory)
			auto& attributes = primitive.at("attributes");
			uint32_t vertexSize = 0;
			size_t numVertices = 0;
			glm::vec3* positionData = nullptr;
			glm::vec3* normalData = nullptr;
			unsigned positionDataStride;
			unsigned normalDataStride;
			uint8_t* colorData = nullptr;
			glm::vec4 (*getColorFunc)(uint8_t* srcPtr);
			uint8_t* texCoordData = nullptr;
			glm::vec4 (*getTexCoordFunc)(uint8_t* srcPtr);
			unsigned colorDataStride;
			unsigned texCoordDataStride;
			CadR::BoundingBox primitiveSetBB;
			for(auto it = attributes.begin(); it != attributes.end(); it++) {
				if(it.key() == "POSITION") {

					// vertex size
					vertexSize += 16;

					// accessor
					json& accessor = accessors.at(it.value().get_ref<json::number_unsigned_t&>());

					// accessor.type is mandatory and it must be VEC3 for position accessor
					if(accessor.at("type").get_ref<json::string_t&>() != "VEC3")
						throw GltfError("Position attribute is not of VEC3 type.");

					// accessor.componentType is mandatory and it must be FLOAT (5126) for position accessor
					if(accessor.at("componentType").get_ref<json::number_unsigned_t&>() != 5126)
						throw GltfError("Position attribute componentType is not float.");

					// accessor.normalized is optional with default value false; it must be false for float componentType
					if(auto it=accessor.find("normalized"); it!=accessor.end())
						if(it->get_ref<json::boolean_t&>() == true)
							throw GltfError("Position attribute normalized flag is true.");

					// update numVertices
					updateNumVertices(accessor, numVertices);

					// position data and stride
					tie(reinterpret_cast<void*&>(positionData), positionDataStride) =
						getDataPointerAndStride(accessor, bufferViews, buffers, bufferDataList,
						                        numVertices, sizeof(glm::vec3));

					// get min and max
					if(auto it=accessor.find("min"); it!=accessor.end()) {
						json::array_t& a = it->get_ref<json::array_t&>();
						if(a.size() != 3)
							throw GltfError("Accessor.min is not vector of three components.");
						primitiveSetBB.min.x = float(a[0].get<json::number_float_t>());
						primitiveSetBB.min.y = float(a[1].get<json::number_float_t>());
						primitiveSetBB.min.z = float(a[2].get<json::number_float_t>());
					}
					else
						throw GltfError("Accessor.min be defined for POSITION accessor.");
					if(auto it=accessor.find("max"); it!=accessor.end()) {
						json::array_t& a = it->get_ref<json::array_t&>();
						if(a.size() != 3)
							throw GltfError("Accessor.max is not vector of three components.");
						primitiveSetBB.max.x = float(a[0].get<json::number_float_t>());
						primitiveSetBB.max.y = float(a[1].get<json::number_float_t>());
						primitiveSetBB.max.z = float(a[2].get<json::number_float_t>());
					}
					else
						throw GltfError("Accessor.max be defined for POSITION accessor.");

				}
				else if(it.key() == "NORMAL") {

					// vertex size
					vertexSize += 16;

					// accessor
					json& accessor = accessors.at(it.value().get_ref<json::number_unsigned_t&>());

					// accessor.type is mandatory and it must be VEC3 for normal accessor
					if(accessor.at("type").get_ref<json::string_t&>() != "VEC3")
						throw GltfError("Normal attribute is not of VEC3 type.");

					// accessor.componentType is mandatory and it must be FLOAT (5126) for normal accessor
					if(accessor.at("componentType").get_ref<json::number_unsigned_t&>() != 5126)
						throw GltfError("Normal attribute componentType is not float.");

					// accessor.normalized is optional with default value false; it must be false for float componentType
					if(auto it=accessor.find("normalized"); it!=accessor.end())
						if(it->get_ref<json::boolean_t&>() == true)
							throw GltfError("Normal attribute normalized flag is true.");

					// update numVertices
					updateNumVertices(accessor, numVertices);

					// normal data and stride
					tie(reinterpret_cast<void*&>(normalData), normalDataStride) =
						getDataPointerAndStride(accessor, bufferViews, buffers, bufferDataList,
						                        numVertices, sizeof(glm::vec3));

				}
				else if(it.key() == "COLOR_0") {

					// vertex size
					vertexSize += 16;

					// accessor
					json& accessor = accessors.at(it.value().get_ref<json::number_unsigned_t&>());

					// accessor.type is mandatory and it must be VEC3 or VEC4 for color accessors
					const json::string_t& t = accessor.at("type").get_ref<json::string_t&>();
					if(t != "VEC3" && t != "VEC4")
						throw GltfError("Color attribute is not of VEC3 or VEC4 type.");

					// accessor.componentType is mandatory and it must be FLOAT (5126),
					// UNSIGNED_BYTE (5121) or UNSIGNED_SHORT (5123) for color accessors
					const json::number_unsigned_t ct = accessor.at("componentType").get_ref<json::number_unsigned_t&>();
					if(ct != 5126 && ct != 5121 && ct != 5123)
						throw GltfError("Color attribute componentType is not float, unsigned byte, or unsigned short.");

					// accessor.normalized is optional with default value false; it must be false for float componentType
					if(auto it=accessor.find("normalized"); it!=accessor.end()) {
						if(it->get_ref<json::boolean_t&>() == false) {
							if(ct == 5121 || ct == 5123)
								throw GltfError("Color attribute component type is set to unsigned byte or unsigned short while normalized flag is not true.");
						}
						else
							if(ct == 5126)
								throw GltfError("Color attribute component type is set to float while normalized flag is true.");
					} else
						if(ct == 5121 || ct == 5123)
							throw GltfError("Color attribute component type is set to unsigned byte or unsigned short while normalized flag is not true.");

					// update numVertices
					updateNumVertices(accessor, numVertices);

					// getColorFunc and elementSize
					size_t elementSize;
					if(t == "VEC4")
						switch(ct) {
						case 5126: getColorFunc = getColorFromVec4f;  elementSize = 16; break;
						case 5121: getColorFunc = getColorFromVec4ub; elementSize = 4;  break;
						case 5123: getColorFunc = getColorFromVec4us; elementSize = 8;  break;
						}
					else // "VEC3"
						switch(ct) {
						case 5126: getColorFunc = getColorFromVec3f;  elementSize = 12; break;
						case 5121: getColorFunc = getColorFromVec3ub; elementSize = 3;  break;
						case 5123: getColorFunc = getColorFromVec3us; elementSize = 6;  break;
						}

					// color data and stride
					tie(reinterpret_cast<void*&>(colorData), colorDataStride) =
						getDataPointerAndStride(accessor, bufferViews, buffers, bufferDataList,
						                        numVertices, elementSize);

				}
				else if(it.key() == "TEXCOORD_0" && baseColorTextureIndex != ~size_t(0)) {

					// vertex size
					vertexSize += 16;

					// accessor
					json& accessor = accessors.at(it.value().get_ref<json::number_unsigned_t&>());

					// accessor.type is mandatory and it must be VEC2 for texCoord accessors
					const json::string_t& t = accessor.at("type").get_ref<json::string_t&>();
					if(t != "VEC2")
						throw GltfError("TexCoord attribute is not of VEC2 type.");

					// accessor.componentType is mandatory and it must be FLOAT (5126),
					// UNSIGNED_BYTE (5121) or UNSIGNED_SHORT (5123) for color accessors
					const json::number_unsigned_t ct = accessor.at("componentType").get_ref<json::number_unsigned_t&>();
					if(ct != 5126 && ct != 5121 && ct != 5123)
						throw GltfError("TexCoord attribute componentType is not float, unsigned byte, or unsigned short.");

					// accessor.normalized is optional with default value false; it must be false for float componentType
					if(auto it=accessor.find("normalized"); it!=accessor.end()) {
						if(it->get_ref<json::boolean_t&>() == false) {
							if(ct == 5121 || ct == 5123)
								throw GltfError("TexCoord attribute component type is set to unsigned byte or unsigned short while normalized flag is not true.");
						}
						else
							if(ct == 5126)
								throw GltfError("TexCoord attribute component type is set to float while normalized flag is true.");
					} else
						if(ct == 5121 || ct == 5123)
							throw GltfError("TexCoord attribute component type is set to unsigned byte or unsigned short while normalized flag is not true.");

					// update numVertices
					updateNumVertices(accessor, numVertices);

					// getTexCoordFunc and elementSize
					size_t elementSize;
					switch(ct) {
					case 5126: getTexCoordFunc = getTexCoordFromVec2f;  elementSize = 8; break;
					case 5121: getTexCoordFunc = getTexCoordFromVec2ub; elementSize = 2; break;
					case 5123: getTexCoordFunc = getTexCoordFromVec2us; elementSize = 4; break;
					}

					// texCoord data and stride
					tie(reinterpret_cast<void*&>(texCoordData), texCoordDataStride) =
						getDataPointerAndStride(accessor, bufferViews, buffers, bufferDataList,
						                        numVertices, elementSize);

				}
				else
					throw GltfError("Unsupported functionality: " + it.key() + " attribute.");
			}

			// indices
			// (they are optional)
			size_t numIndices;
			void* indexData;
			unsigned indexComponentType;
			if(auto indicesIt=primitive.find("indices"); indicesIt!=primitive.end()) {

				// accessor
				json& accessor = accessors.at(indicesIt.value().get_ref<json::number_unsigned_t&>());

				// accessor.type is mandatory and it must be SCALAR for index accessors
				const json::string_t& t = accessor.at("type").get_ref<json::string_t&>();
				if(t != "SCALAR")
					throw GltfError("Indices are not of SCALAR type.");

				// accessor.componentType is mandatory and it must be UNSIGNED_INT (5125) for index accessors;
				// unsigned short and unsigned byte component types seems not allowed by the spec
				// but they are used in Khronos sample models, for example Box.gltf
				// (https://github.com/KhronosGroup/glTF-Sample-Models/blob/main/2.0/Box/glTF/Box.gltf)
				indexComponentType = unsigned(accessor.at("componentType").get_ref<json::number_unsigned_t&>());
				if(indexComponentType != 5125 && indexComponentType != 5123 && indexComponentType != 5121)
					throw GltfError("Index componentType is not unsigned int, unsigned short or unsigned byte.");

				// accessor.normalized is optional and must be false for index accessor
				if(auto it=accessor.find("normalized"); it!=accessor.end())
					if(it->get_ref<json::boolean_t&>() == true)
						throw GltfError("Indices cannot have normalized flag set to true.");

				// get index count (accessor.count is mandatory and >=1)
				numIndices = accessor.at("count").get_ref<json::number_unsigned_t&>();
				if(numIndices == 0)
					throw GltfError("Accessor's count member must be greater than zero.");

				// index data
				size_t elementSize;
				switch(indexComponentType) {
				case 5125: elementSize = sizeof(uint32_t); break;
				case 5123: elementSize = sizeof(uint16_t); break;
				case 5121: elementSize = sizeof(uint8_t); break;
				}
				size_t tmp;
				tie(indexData, tmp) =
					getDataPointerAndStride(accessor, bufferViews, buffers, bufferDataList,
					                        numIndices, elementSize);
			}
			else {
				numIndices = numVertices;
				indexData = nullptr;
			}

			// ignore empty primitives
			if(indexData == nullptr && numVertices == 0)
				continue;

			// create Geometry
			cout << "Creating geometry" << endl;
			CadR::Geometry& g = geometryDB.emplace_back(renderer);

			// update mesh bounds
			meshBB.extendBy(primitiveSetBB);

			// prepare for computing primitiveSet bounding sphere
			CadR::BoundingSphere primitiveSetBS{
				.center = positionData ? primitiveSetBB.getCenter() : glm::vec3(0.f, 0.f, 0.f),
				.radius = 0.f,  // actually, radius^2 is stored here in the following loop as an performance optimization
			};

			// set vertex data
			CadR::StagingData sd = g.createVertexStagingData(numVertices * vertexSize);
			uint8_t* p = sd.data<uint8_t>();
			for(size_t i=0; i<numVertices; i++) {
				if(positionData) {

					// copy position
					glm::vec3 pos(*positionData);
					*reinterpret_cast<glm::vec4*>(p)= glm::vec4(pos.x, pos.y, pos.z, 1.f);
					p += 16;
					positionData = reinterpret_cast<glm::vec3*>(reinterpret_cast<uint8_t*>(positionData) + positionDataStride);

					// update bounding sphere
					// (square of radius is stored in primitiveBS.radius)
					primitiveSetBS.extendRadiusByPointUsingRadius2(pos);

				}
				if(normalData) {
					glm::vec3 normal = glm::vec3(*normalData);
					*reinterpret_cast<glm::vec4*>(p) = glm::vec4(normal.x, normal.y, normal.z, 0.f);
					p += 16;
					normalData = reinterpret_cast<glm::vec3*>(reinterpret_cast<uint8_t*>(normalData) + normalDataStride);
				}
				if(colorData) {
					glm::vec4* v = reinterpret_cast<glm::vec4*>(p);
					*v = getColorFunc(colorData);
					p += 16;
					colorData += colorDataStride;
				}
				if(texCoordData) {
					glm::vec4* v = reinterpret_cast<glm::vec4*>(p);
					*v = getTexCoordFunc(texCoordData);
					p += 16;
					texCoordData += texCoordDataStride;
				}
			}

			// mesh.primitive.mode is optional with default value 4 (TRIANGLES)
			unsigned mode = unsigned(primitive.value<json::number_unsigned_t>("mode", 4));

			// set index data
			if(mode == 4) {  // TRIANGLES
				if(numIndices < 3)
					throw GltfError("Invalid number of indices for TRIANGLES.");
				goto copyIndices;
			}
			else if(mode == 1) {  // LINES
				if(numIndices < 2)
					throw GltfError("Invalid number of indices for LINES.");
				goto copyIndices;
			}
			else if(mode == 0) {  // POINTS
				if(numIndices < 1)
					throw GltfError("Invalid number of indices for POINTS.");

				// POINTS, LINES, TRIANGLES - copy indices directly,
				// while converting uint8_t and uint16_t indices to uint32_t
			copyIndices:
				size_t indexDataSize = numIndices * sizeof(uint32_t);
				sd = g.createIndexStagingData(indexDataSize);
				uint32_t* pi = sd.data<uint32_t>();
				if(indexData)
					switch(indexComponentType) {
					case 5125: memcpy(pi, indexData, indexDataSize); break;
					case 5123: {
						for(size_t i=0; i<numIndices; i++)
							pi[i] = reinterpret_cast<uint16_t*>(indexData)[i];
						break;
					}
					case 5121: {
						for(size_t i=0; i<numIndices; i++)
							pi[i] = reinterpret_cast<uint8_t*>(indexData)[i];
						break;
					}
					}
				else {
					if(numIndices >= size_t((~uint32_t(0))-1)) // value 0xffffffff is forbidden, thus (~0)-1
						throw GltfError("Too large primitive. Index out of 32-bit integer range.");
					for(uint32_t i=0; i<uint32_t(numIndices); i++)
						pi[i] = i;
				}
			}
			else if(mode == 5) {

				// TRIANGLE_STRIP - convert strip indices to indices of separate triangles
				// while considering even and odd triangle ordering
				if(numIndices < 3)
					throw GltfError("Invalid number of indices for TRIANGLE_STRIP.");
				numIndices = (numIndices-2) * 3;
				sd = g.createIndexStagingData(numIndices * sizeof(uint32_t));
				uint32_t* stgIndices = sd.data<uint32_t>();
				if(indexData) {

					// create new indices
					auto createTriangleStripIndices =
						[]<typename T>(uint32_t* dst, void* srcPtr, size_t numIndices) {

							T* src = reinterpret_cast<T*>(srcPtr);
							uint32_t* dstEnd = dst + numIndices;
							uint32_t v1 = *src;
							src++;
							uint32_t v2 = *src;
							src++;
							uint32_t v3 = *src;
							src++;
							while(true) {

								// odd triangle
								*dst = v1;
								dst++;
								*dst = v2;
								dst++;
								*dst = v3;
								dst++;
								if(dst == dstEnd)
									break;
								v1 = v2;
								v2 = v3;
								v3 = *src;
								src++;

								// even triangle
								*dst = v2;
								dst++;
								*dst = v1;
								dst++;
								*dst = v3;
								dst++;
								if(dst == dstEnd)
									break;
								v1 = v2;
								v2 = v3;
								v3 = *src;
								src++;
							}
						};
					switch(indexComponentType) {
					case 5125: createTriangleStripIndices.operator()<uint32_t>(stgIndices, indexData, numIndices); break;
					case 5123: createTriangleStripIndices.operator()<uint16_t>(stgIndices, indexData, numIndices); break;
					case 5121: createTriangleStripIndices.operator()<uint8_t >(stgIndices, indexData, numIndices); break;
					}
				}
				else {

					// generate indices
					if(numIndices >= size_t((~uint32_t(0))-1)) // value 0xffffffff is forbidden, thus (~0)-1
						throw GltfError("Too large primitive. Index out of 32-bit integer range.");

					uint32_t i = 0;
					uint32_t v1 = i;
					i++;
					uint32_t v2 = i;
					i++;
					uint32_t v3 = i;
					i++;
					uint32_t* e = stgIndices + numIndices;
					while(true) {

						// odd triangle
						*stgIndices = v1;
						stgIndices++;
						*stgIndices = v2;
						stgIndices++;
						*stgIndices = v3;
						stgIndices++;
						if(stgIndices == e)
							break;
						v1 = v2;
						v2 = v3;
						v3 = i;
						i++;

						// even triangle
						*stgIndices = v2;
						stgIndices++;
						*stgIndices = v1;
						stgIndices++;
						*stgIndices = v3;
						stgIndices++;
						if(stgIndices == e)
							break;
						v1 = v2;
						v2 = v3;
						v3 = i;
						i++;
					}
				}
			}
			else if(mode == 6) {

				// TRIANGLE_FAN
				if(numIndices < 3)
					throw GltfError("Invalid number of indices for TRIANGLE_FAN.");
				numIndices = (numIndices-2) * 3;
				sd = g.createIndexStagingData(numIndices * sizeof(uint32_t));
				uint32_t* stgIndices = sd.data<uint32_t>();
				if(indexData) {

					// create new indices
					auto createTriangleStripIndices =
						[]<typename T>(uint32_t* dst, void* srcPtr, size_t numIndices) {

							T* src = reinterpret_cast<T*>(srcPtr);
							uint32_t* dstEnd = dst + numIndices;
							uint32_t v1 = *src;
							src++;
							uint32_t v2 = *src;
							src++;
							uint32_t v3 = *src;
							src++;
							while(true) {
								*dst = v1;
								dst++;
								*dst = v2;
								dst++;
								*dst = v3;
								dst++;
								if(dst == dstEnd)
									break;
								v2 = v3;
								v3 = *src;
								src++;
							}
						};
					switch(indexComponentType) {
					case 5125: createTriangleStripIndices.operator()<uint32_t>(stgIndices, indexData, numIndices); break;
					case 5123: createTriangleStripIndices.operator()<uint16_t>(stgIndices, indexData, numIndices); break;
					case 5121: createTriangleStripIndices.operator()<uint8_t >(stgIndices, indexData, numIndices); break;
					}
				}
				else {

					// generate indices
					if(numIndices >= size_t((~uint32_t(0))-1)) // value 0xffffffff is forbidden, thus (~0)-1
						throw GltfError("Too large primitive. Index out of 32-bit integer range.");

					uint32_t i = 0;
					uint32_t v1 = i;
					i++;
					uint32_t v2 = i;
					i++;
					uint32_t v3 = i;
					i++;
					uint32_t* e = stgIndices + numIndices;
					while(true) {
						*stgIndices = v1;
						stgIndices++;
						*stgIndices = v2;
						stgIndices++;
						*stgIndices = v3;
						stgIndices++;
						if(stgIndices == e)
							break;
						v2 = v3;
						v3 = i;
						i++;
					}
				}
			}
			else if(mode == 3) {

				// LINE_STRIP
				if(numIndices < 2)
					throw GltfError("Invalid number of indices for LINE_STRIP.");
				numIndices = (numIndices-1) * 2;
				sd = g.createIndexStagingData(numIndices * sizeof(uint32_t));
				uint32_t* stgIndices = sd.data<uint32_t>();
				if(indexData) {

					// create new indices
					auto createLineStripIndices =
						[]<typename T>(uint32_t* dst, void* srcPtr, size_t numIndices) {
							T* src = reinterpret_cast<T*>(srcPtr);
							*dst = *src;
							uint32_t* dstEnd = dst + (numIndices-1);
							dst++; src++;
							while(dst < dstEnd) {
								*dst = *src;
								dst++;
								*dst = *src;
								dst++; src++;
							}
							*dst = *src;
						};
					switch(indexComponentType) {
					case 5125: createLineStripIndices.operator()<uint32_t>(stgIndices, indexData, numIndices); break;
					case 5123: createLineStripIndices.operator()<uint16_t>(stgIndices, indexData, numIndices); break;
					case 5121: createLineStripIndices.operator()<uint8_t >(stgIndices, indexData, numIndices); break;
					}
				}
				else {

					// generate indices
					if(numIndices >= size_t((~uint32_t(0))-1)) // value 0xffffffff is forbidden, thus (~0)-1
						throw GltfError("Too large primitive. Index out of 32-bit integer range.");
					uint32_t i = 0;
					*stgIndices = i;
					uint32_t* e = stgIndices + (numIndices-1);
					stgIndices++;
					i++;
					while(stgIndices < e) {
						*stgIndices = i;
						stgIndices++;
						*stgIndices = i;
						stgIndices++;
						i++;
					}
					*stgIndices = i;
				}
			}
			else if(mode == 2) {

				// LINE_LOOP
				if(numIndices < 2)
					throw GltfError("Invalid number of indices for LINE_LOOP.");
				numIndices = numIndices * 2;
				sd = g.createIndexStagingData(numIndices * sizeof(uint32_t));
				uint32_t* stgIndices = sd.data<uint32_t>();
				if(indexData) {

					// create new indices
					auto createLineLoopIndices =
						[]<typename T>(uint32_t* dst, void* srcPtr, size_t numIndices) {
							T* src = reinterpret_cast<T*>(srcPtr);
							uint32_t firstValue = *src;
							*dst = *src;
							uint32_t* dstEnd = dst + (numIndices-1);
							dst++; src++;
							while(dst < dstEnd) {
								*dst = *src;
								dst++;
								*dst = *src;
								dst++; src++;
							}
							*dst = firstValue;
						};
					switch(indexComponentType) {
					case 5125: createLineLoopIndices.operator()<uint32_t>(stgIndices, indexData, numIndices); break;
					case 5123: createLineLoopIndices.operator()<uint16_t>(stgIndices, indexData, numIndices); break;
					case 5121: createLineLoopIndices.operator()<uint8_t >(stgIndices, indexData, numIndices); break;
					}
				}
				else {

					// generate indices
					if(numIndices >= size_t((~uint32_t(0))-1)) // value 0xffffffff is forbidden, thus (~0)-1
						throw GltfError("Too large primitive. Index out of 32-bit integer range.");
					uint32_t i = 0;
					*stgIndices = i;
					uint32_t* e = stgIndices + (numIndices-1);
					stgIndices++;
					i++;
					while(stgIndices < e) {
						*stgIndices = i;
						stgIndices++;
						*stgIndices = i;
						stgIndices++;
						i++;
					}
					*stgIndices = 0;
				}
			}
			else
				throw GltfError("Invalid value for mesh.primitive.mode.");

			// set primitiveSet data
			struct PrimitiveSetGpuData {
				uint32_t count;
				uint32_t first;
			};
			sd = g.createPrimitiveSetStagingData(sizeof(PrimitiveSetGpuData));
			PrimitiveSetGpuData* ps = sd.data<PrimitiveSetGpuData>();
			ps->count = uint32_t(numIndices);
			ps->first = 0;

			// primitiveSet bounding sphere list
			// (convert square of radius stored in primitiveBS.radius back to radius)
			primitiveSetBS.radius = sqrt(primitiveSetBS.radius);
			primitiveSetBSList.emplace_back(primitiveSetBS);

			// get pipeline index
			size_t pipelineIndex;
			switch(mode) {
			case 0:  // POINTS
				pipelineIndex =
					PipelineLibrary::getPointPipelineIndex(
						normalData   != nullptr,  // phong
						texCoordData != nullptr,  // texturing
						colorData    != nullptr   // perVertexColor
					);
				break;
			case 1:  // LINES
			case 2:  // LINE_LOOP
			case 3:  // LINE_STRIP
				pipelineIndex =
					PipelineLibrary::getLinePipelineIndex(
						normalData   != nullptr,  // phong
						texCoordData != nullptr,  // texturing
						colorData    != nullptr   // perVertexColor
					);
				break;
			case 4:  // TRIANGLES
			case 5:  // TRIANGLE_STRIP
			case 6: {  // TRIANGLE_FAN
				pipelineIndex =
					PipelineLibrary::getTrianglePipelineIndex(
						normalData   != nullptr,  // phong
						texCoordData != nullptr,  // texturing
						colorData    != nullptr,  // perVertexColor
						!doubleSided,             // backFaceCulling
						vk::FrontFace::eCounterClockwise  // frontFace
					);
				break;
			}
			default:
				throw GltfError("Invalid value for mesh.primitive.mode.");
			}

			// get StateSet
			PipelineStateSet& pipelineStateSet = pipelineStateSetList[pipelineIndex];
			CadR::StateSet* ss;
			if(baseColorTextureIndex == ~size_t(0))

				// if no texture, use StateSet of the pipeline
				ss = &pipelineStateSet.stateSet;

			else {

				// if texturing, each texture have its StateSet as child of the pipeline StateSet

				// initialize textureIndexToStateSet
				if(pipelineStateSet.textureIndexToStateSet.empty())
					pipelineStateSet.textureIndexToStateSet.resize(textureDB.size(), nullptr);

				// get or create TextureStateSet
				TextureStateSet* textureStateSet = pipelineStateSet.textureIndexToStateSet[baseColorTextureIndex];
				if(!textureStateSet) {

					// create TextureStateSet
					CadR::Texture& t = textureDB[baseColorTextureIndex];
					textureStateSet = &pipelineStateSet.textureStateSetList.emplace_back(renderer, &t);
					pipelineStateSet.textureIndexToStateSet[baseColorTextureIndex] = textureStateSet;

					// alloc descriptor set
					ss = &textureStateSet->stateSet;
					pipelineStateSet.stateSet.childList.append(*ss);
					ss->allocDescriptorSet(vk::DescriptorType::eCombinedImageSampler, pipelineLibrary.textureDescriptorSetLayout());

					// attach StateSet for updating its descriptor
					t.attachStateSet(
						*ss,
						ss->descriptorSets()[0],
						[](CadR::StateSet& ss, vk::DescriptorSet descriptorSet, CadR::Texture& t) {
							// update descriptor
							ss.updateDescriptorSet(
								vk::WriteDescriptorSet(
									descriptorSet,  // dstSet
									0,  // dstBinding
									0,  // dstArrayElement
									1,  // descriptorCount
									vk::DescriptorType::eCombinedImageSampler,  // descriptorType
									&(const vk::DescriptorImageInfo&)vk::DescriptorImageInfo{  // pImageInfo
										t.sampler(),  // sampler
										t.imageView(),  // imageView
										vk::ImageLayout::eShaderReadOnlyOptimal  // imageLayout
									},
									nullptr,  // pBufferInfo
									nullptr  // pTexelBufferView
								)
							);
						}
					);
				}

				// get StateSet of texture
				ss = &textureStateSet->stateSet;
			}

			// drawable
			vector<glm::mat4>& matrixList = meshMatrixList[meshIndex];
			uint32_t numInstances = uint32_t(matrixList.size());
			drawableDB.emplace_back(
				g,  // geometry
				0,  // primitiveSetOffset
				sd,  // shaderStagingData
				64+(numInstances*64),  // shaderDataSize
				numInstances,  // numInstances
				*ss  // stateSet
			);

			// material
			struct MaterialData {
				glm::vec3 ambient;  // offset 0
				uint32_t settings;  // offset 12
				glm::vec4 diffuseAndAlpha;  // offset 16
				glm::vec3 specular;  // offset 32
				float shininess;  // offset 44
				glm::vec3 emission;  // offset 48
				float pointSize;  // offset 60
			};
			MaterialData* m = sd.data<MaterialData>();
			m->settings = (normalData) ? 0x04 : 0x02;  // choose between Phong and Unlit color
			m->settings |= (texCoordData) ? 0x01 : 0x00;  // choose between textured and not textured
			if(colorData == nullptr)
				m->settings |= 0x30;  // ambient and diffuse color will be taken from material instead of attribute
			if(material) {

				// set material data
				m->ambient = glm::vec3(baseColorFactor);
				m->diffuseAndAlpha = baseColorFactor;
				m->specular = baseColorFactor * metallicFactor;  // very vague and imprecise conversion
				m->shininess = (1.f - roughnessFactor) * 128.f;  // very vague and imprecise conversion
				m->emission = emissiveFactor;
				m->pointSize = 0.f;

			}
			else {

				// set default material data
				m->ambient = glm::vec3(1.f, 1.f, 1.f);
				m->diffuseAndAlpha = glm::vec4(1.f, 1.f, 1.f, 1.f);
				m->specular = glm::vec3(0.f, 0.f, 0.f);
				m->shininess = 0.f;
				m->emission = glm::vec3(0.f, 0.f, 0.f);
				m->pointSize = 0.f;

			}

			// copy transformation matrices
			// (transformation matrices follow material data on offset 64)
			glm::mat4* modelMatrix = reinterpret_cast<glm::mat4*>(reinterpret_cast<uint8_t*>(m) + 64);
			memcpy(modelMatrix, matrixList.data(), numInstances*64);

			// mesh bounding sphere
			CadR::BoundingSphere meshBS{
				.center = meshBB.getCenter(),
				.radius = 0.f,
			};
			for(size_t i=0,c=primitiveSetBSList.size(); i<c; i++)
				meshBS.extendRadiusBy(primitiveSetBSList[i]);

			// bounding box of all instances of particular mesh
			CadR::BoundingBox instancesBB =
				CadR::BoundingBox::createByCenterAndHalfExtents(
					glm::mat3(matrixList[0]) * meshBS.center + glm::vec3(matrixList[0][3]),  // center
					glm::mat3(matrixList[0]) * glm::vec3(meshBS.radius)  // halfExtents
				);
			for(size_t instanceIndex=1, instanceCount=matrixList.size();
			    instanceIndex<instanceCount; instanceIndex++)
			{
				glm::mat4& m = matrixList[instanceIndex];
				instancesBB.extendBy(
					CadR::BoundingBox::createByCenterAndHalfExtents(
						glm::mat3(m) * meshBS.center + glm::vec3(m[3]),  // center
						glm::mat3(m) * glm::vec3(meshBS.radius)  // radius
					)
				);
			}

			// bounding sphere of all instances of particular mesh
			CadR::BoundingSphere instancesBS{
				.center = instancesBB.getCenter(),
				.radius = 0.f,
			};
			for(size_t instanceIndex=0, instanceCount=matrixList.size();
			    instanceIndex<instanceCount; instanceIndex++)
			{
				instancesBS.extendRadiusBy(matrixList[instanceIndex] * meshBS);
			}
			meshBoundingSphereList[meshIndex] = instancesBS;

		}
	}

	// scene bounding box
	CadR::BoundingBox sceneBB = meshBoundingSphereList[0].getBoundingBox();
	for(size_t i=1, c=meshBoundingSphereList.size(); i<c; i++)
		sceneBB.extendBy(meshBoundingSphereList[i].getBoundingBox());

	// scene bounding sphere
	sceneBoundingSphere.center = sceneBB.getCenter();
	sceneBoundingSphere.radius =
		sqrt(glm::distance2(meshBoundingSphereList[0].center, sceneBoundingSphere.center)) +
		meshBoundingSphereList[0].radius;
	for(size_t i=1, c=meshBoundingSphereList.size(); i<c; i++)
		sceneBoundingSphere.extendRadiusBy(meshBoundingSphereList[i]);
	sceneBoundingSphere.radius *= 1.001f;  // increase radius to accommodate for all floating computations imprecisions

	// initial camera distance
	float fovy2Clamped = glm::clamp(fovy / 2.f, 1.f / 180.f * glm::pi<float>(), 90.f / 180.f * glm::pi<float>());
	cameraDistance = sceneBoundingSphere.radius / sin(fovy2Clamped);

#if 0  // show scene bounding sphere
	auto createBoundingSphereVisualization =
		[](const CadR::BoundingSphere bs, App& app) {

			// vertex data
			// (bounding sphere in the form of axis cross)
			CadR::Geometry& g = app.geometryDB.emplace_back(app.renderer);
			CadR::StagingData sd = g.createVertexStagingData(6 * sizeof(glm::vec4));
			glm::vec4* pos = sd.data<glm::vec4>();
			pos[0] = glm::vec4(bs.center.x-bs.radius, bs.center.y, bs.center.z, 1.f);
			pos[1] = glm::vec4(bs.center.x+bs.radius, bs.center.y, bs.center.z, 1.f);
			pos[2] = glm::vec4(bs.center.x, bs.center.y-bs.radius, bs.center.z, 1.f);
			pos[3] = glm::vec4(bs.center.x, bs.center.y+bs.radius, bs.center.z, 1.f);
			pos[4] = glm::vec4(bs.center.x, bs.center.y, bs.center.z-bs.radius, 1.f);
			pos[5] = glm::vec4(bs.center.x, bs.center.y, bs.center.z+bs.radius, 1.f);

			// index data
			sd = g.createIndexStagingData(6 * sizeof(uint32_t));
			uint32_t* indices = sd.data<uint32_t>();
			for(uint32_t i=0; i<6; i++)
				indices[i] = i;

			// primitive set
			struct PrimitiveSetGpuData {
				uint32_t count;
				uint32_t first;
			};
			sd = g.createPrimitiveSetStagingData(sizeof(PrimitiveSetGpuData));
			PrimitiveSetGpuData* ps = sd.data<PrimitiveSetGpuData>();
			ps->count = 6;
			ps->first = 0;

			// state set
			unsigned pipelineIndex =
				PipelineLibrary::getLinePipelineIndex(
					false,  // phong
					false,  // texturing
					false   // perVertexColor
				);
			CadR::StateSet& ss = app.stateSetDB[pipelineIndex];

			// drawable
			app.drawableDB.emplace_back(
				g,  // geometry
				0,  // primitiveSetOffset
				sd,  // shaderStagingData
				64+64,  // shaderDataSize
				1,  // numInstances
				ss);  // stateSet

			// material
			struct MaterialData {
				glm::vec3 ambient;  // offset 0
				uint32_t type;  // offset 12
				glm::vec4 diffuseAndAlpha;  // offset 16
				glm::vec3 specular;  // offset 32
				float shininess;  // offset 44
				glm::vec3 emission;  // offset 48
				float pointSize;  // offset 60
			};
			MaterialData* m = sd.data<MaterialData>();
			m->ambient = glm::vec3(1.f, 1.f, 1.f);
			m->type = 0;
			m->diffuseAndAlpha = glm::vec4(1.f, 1.f, 1.f, 1.f);
			m->specular = glm::vec3(0.f, 0.f, 0.f);
			m->shininess = 0.f;
			m->emission = glm::vec3(0.f, 0.f, 0.f);
			m->pointSize = 0.f;

			// transformation matrices
			glm::mat4* matrices = sd.data<glm::mat4>() + 1;
			matrices[0] = glm::mat4(1.f);
		};

#if 0  // show scene bounding sphere
	createBoundingSphereVisualization(sceneBoundingSphere, *this);
#endif

#if 0  // show bounding sphere of each mesh
	for(size_t i=0, c=meshBoundingSphereList.size(); i<c; i++)
		createBoundingSphereVisualization(meshBoundingSphereList[i], *this);
#endif
#endif

	// upload all staging buffers
	renderer.executeCopyOperations();
}


void App::resize(VulkanWindow& window,
	const vk::SurfaceCapabilitiesKHR& surfaceCapabilities, vk::Extent2D newSurfaceExtent)
{
	// clear resources
	for(auto v : swapchainImageViews)  device.destroy(v);
	swapchainImageViews.clear();
	device.destroy(depthImage);
	device.free(depthImageMemory);
	device.destroy(depthImageView);
	for(auto f : framebuffers)  device.destroy(f);
	framebuffers.clear();

	// perspective matrix given by FOV (Field Of View)
	// FOV is given in vertical direction in radians
	// The function perspectiveLH_ZO() produces exactly the matrix we need in Vulkan for right-handed, zero-to-one coordinate system.
	// The coordinate system will have +x in the right direction, +y in down direction (as Vulkan does it), and +z forward into the scene.
	// ZO - Zero to One is output depth range,
	// RH - Right Hand coordinate system, +Y is down, +Z is towards camera
	// LH - LeftHand coordinate system, +Y is down, +Z points into the scene
	constexpr float zNear = 0.5f;
	constexpr float zFar = 100.f;
	glm::mat4 projectionMatrix = glm::perspectiveLH_ZO(fovy, float(newSurfaceExtent.width)/newSurfaceExtent.height, zNear, zFar);

	// pipeline specialization constants
	const array<float,6> specializationConstants = {
		projectionMatrix[2][0], projectionMatrix[2][1], projectionMatrix[2][3],
		projectionMatrix[3][0], projectionMatrix[3][1], projectionMatrix[3][3],
	};
	const array specializationMap {
		vk::SpecializationMapEntry{0,0,4},  // constantID, offset, size
		vk::SpecializationMapEntry{1,4,4},
		vk::SpecializationMapEntry{2,8,4},
		vk::SpecializationMapEntry{3,12,4},
		vk::SpecializationMapEntry{4,16,4},
		vk::SpecializationMapEntry{5,20,4},
	};
	const vk::SpecializationInfo specializationInfo(  // pSpecializationInfo
		6,  // mapEntryCount
		specializationMap.data(),  // pMapEntries
		6*sizeof(float),  // dataSize
		specializationConstants.data()  // pData
	);

	// recreate pipelines
	pipelineLibrary.create(device, newSurfaceExtent, specializationInfo, renderPass, renderer.pipelineCache());
	for(size_t i=0; i<pipelineLibrary.numPipelines(); i++)
		pipelineStateSetList[i].pipeline.set(pipelineLibrary.pipeline(i));

	// print info
	cout << "Recreating swapchain (extent: " << newSurfaceExtent.width << "x" << newSurfaceExtent.height
	     << ", extent by surfaceCapabilities: " << surfaceCapabilities.currentExtent.width << "x"
	     << surfaceCapabilities.currentExtent.height << ", minImageCount: " << surfaceCapabilities.minImageCount
	     << ", maxImageCount: " << surfaceCapabilities.maxImageCount << ")" << endl;

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
	vector<vk::Image> swapchainImages = device.getSwapchainImagesKHR(swapchain);
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

	// depth image
	depthImage =
		device.createImage(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				depthFormat,             // format
				vk::Extent3D(newSurfaceExtent.width, newSurfaceExtent.height, 1),  // extent
				1,                       // mipLevels
				1,                       // arrayLayers
				vk::SampleCountFlagBits::e1,  // samples
				vk::ImageTiling::eOptimal,    // tiling
				vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr,                      // pQueueFamilyIndices
				vk::ImageLayout::eUndefined   // initialLayout
			)
		);

	// memory for images
	vk::PhysicalDeviceMemoryProperties memoryProperties = vulkanInstance.getPhysicalDeviceMemoryProperties(physicalDevice);
	auto allocateMemory =
		[](CadR::VulkanDevice& device, vk::Image image, vk::MemoryPropertyFlags requiredFlags,
		   const vk::PhysicalDeviceMemoryProperties& memoryProperties) -> vk::DeviceMemory
		{
			vk::MemoryRequirements memoryRequirements = device.getImageMemoryRequirements(image);
			for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++)
				if(memoryRequirements.memoryTypeBits & (1<<i))
					if((memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags)
						return
							device.allocateMemory(
								vk::MemoryAllocateInfo(
									memoryRequirements.size,  // allocationSize
									i                         // memoryTypeIndex
								)
							);
			throw std::runtime_error("No suitable memory type found for the image.");
		};
	depthImageMemory = allocateMemory(device, depthImage, vk::MemoryPropertyFlagBits::eDeviceLocal, memoryProperties);
	device.bindImageMemory(
		depthImage,  // image
		depthImageMemory,  // memory
		0  // memoryOffset
	);

	// image views
	depthImageView =
		device.createImageView(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),  // flags
				depthImage,                  // image
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

	// framebuffers
	framebuffers.reserve(swapchainImages.size());
	for(size_t i=0, c=swapchainImages.size(); i<c; i++)
		framebuffers.emplace_back(
			device.createFramebuffer(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),  // flags
					renderPass,  // renderPass
					2,  // attachmentCount
					array{  // pAttachments
						swapchainImageViews[i],
						depthImageView,
					}.data(),
					newSurfaceExtent.width,  // width
					newSurfaceExtent.height,  // height
					1  // layers
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
			renderFinishedFence,  // fences
			VK_TRUE,  // waitAll
			uint64_t(3e9)  // timeout
		);
	if(r != vk::Result::eSuccess) {
		if(r == vk::Result::eTimeout)
			throw runtime_error("GPU timeout. Task is probably hanging on GPU.");
		throw runtime_error("Vulkan error: vkWaitForFences failed with error " + to_string(r) + ".");
	}
	device.resetFences(renderFinishedFence);

	// _sceneDataAllocation
	uint32_t sceneDataSize = uint32_t(sizeof(SceneGpuData));
	CadR::StagingData sceneStagingData = sceneDataAllocation.alloc(sceneDataSize);
	SceneGpuData* sceneData = sceneStagingData.data<SceneGpuData>();
	sceneData->viewMatrix =
		glm::lookAtLH(  // 0,0,+5 is produced inside translation part of viewMatrix
			sceneBoundingSphere.center + glm::vec3(  // eye
				+cameraDistance*sin(-cameraHeading)*cos(cameraElevation),  // x
				-cameraDistance*sin(cameraElevation),  // y
				-cameraDistance*cos(cameraHeading)*cos(cameraElevation)  // z
			),
			sceneBoundingSphere.center,  // center
			glm::vec3(0.f,1.f,0.f)  // up
		);
	float zFar = fabs(cameraDistance) + sceneBoundingSphere.radius;
	float zNear = fabs(cameraDistance) - sceneBoundingSphere.radius;
	float minZNear = zFar / maxZNearZFarRatio;
	if(zNear < minZNear)
		zNear = minZNear;
	glm::mat4 projectionMatrix = glm::perspectiveLH_ZO(fovy, float(window.surfaceExtent().width)/window.surfaceExtent().height, zNear, zFar);
	sceneData->p11 = projectionMatrix[0][0];
	sceneData->p22 = projectionMatrix[1][1];
	sceneData->p33 = projectionMatrix[2][2];
	sceneData->p43 = projectionMatrix[3][2];
	sceneData->ambientLight = glm::vec3(0.2f, 0.2f, 0.2f);
	sceneData->numLights = 0;

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
	size_t numDrawables = renderer.prepareSceneRendering(stateSetRoot);

	// record compute shader preprocessing
	renderer.recordDrawableProcessing(commandBuffer, numDrawables);

	// record scene rendering
	device.cmdPushConstants(
		commandBuffer,  // commandBuffer
		pipelineLibrary.pipelineLayout(),  // pipelineLayout
		vk::ShaderStageFlagBits::eVertex,  // stageFlags
		0,  // offset
		sizeof(uint64_t),  // size
		array<uint64_t,1>{  // pValues
			renderer.drawablePointersBufferAddress(),  // payloadBufferPtr
		}.data()
	);
	device.cmdPushConstants(
		commandBuffer,  // commandBuffer
		pipelineLibrary.pipelineLayout(),  // pipelineLayout
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,  // stageFlags
		8,  // offset
		sizeof(uint64_t),  // size
		array<uint64_t,1>{  // pValues
			sceneDataAllocation.deviceAddress(),  // sceneDataPtr
		}.data()
	);
	renderer.recordSceneRendering(
		commandBuffer,  // commandBuffer
		stateSetRoot,  // stateSetRoot
		renderPass,  // renderPass
		framebuffers[imageIndex],  // framebuffer
		vk::Rect2D(vk::Offset2D(0, 0), window.surfaceExtent()),  // renderArea
		2,  // clearValueCount
		array<vk::ClearValue,2>{  // pClearValues
			vk::ClearColorValue(array<float,4>{0.f, 0.f, 0.f, 1.f}),
			vk::ClearDepthStencilValue(1.f, 0),
		}.data()
	);

	// end command buffer recording
	renderer.endRecording(commandBuffer);

	// submit all copy operations that were not submitted yet
	renderer.executeCopyOperations();

	// submit frame
	device.queueSubmit(
		graphicsQueue,  // queue
		vk::SubmitInfo(
			1, &imageAvailableSemaphore,  // waitSemaphoreCount + pWaitSemaphores +
			&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(  // pWaitDstStageMask
				vk::PipelineStageFlagBits::eColorAttachmentOutput),
			1, &commandBuffer,  // commandBufferCount + pCommandBuffers
			1, &renderFinishedSemaphore  // signalSemaphoreCount + pSignalSemaphores
		),
		renderFinishedFence  // fence
	);

	// present
	r =
		device.presentKHR(
			presentationQueue,  // queue
			&(const vk::PresentInfoKHR&)vk::PresentInfoKHR(  // presentInfo
				1, &renderFinishedSemaphore,  // waitSemaphoreCount + pWaitSemaphores
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


void App::mouseMove(VulkanWindow& window, const VulkanWindow::MouseState& mouseState)
{
	if(mouseState.buttons[VulkanWindow::MouseButton::Left]) {
		cameraHeading = startCameraHeading + (mouseState.posX - startMouseX) * 0.01f;
		cameraElevation = startCameraElevation + (mouseState.posY - startMouseY) * 0.01f;
		window.scheduleFrame();
	}
}


void App::mouseButton(VulkanWindow& window, size_t button, VulkanWindow::ButtonState buttonState,
                      const VulkanWindow::MouseState& mouseState)
{
	if(button == VulkanWindow::MouseButton::Left) {
		if(buttonState == VulkanWindow::ButtonState::Pressed) {
			startMouseX = mouseState.posX;
			startMouseY = mouseState.posY;
			startCameraHeading = cameraHeading;
			startCameraElevation = cameraElevation;
		}
		else {
			cameraHeading = startCameraHeading + (mouseState.posX - startMouseX) * 0.01f;
			cameraElevation = startCameraElevation + (mouseState.posY - startMouseY) * 0.01f;
			window.scheduleFrame();
		}
	}
}


void App::mouseWheel(VulkanWindow& window, float wheelX, float wheelY, const VulkanWindow::MouseState& mouseState)
{
	cameraDistance *= pow(zoomStepRatio, wheelY);
	window.scheduleFrame();
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
	default:   throw GltfError("Invalid accessor's componentType value.");
	}
	if(type=="VEC3")  return 3*componentSize;
	else if(type=="VEC4")  return 4*componentSize;
	else if(type=="VEC2")  return 2*componentSize;
	else if(type=="SCALAR")  return componentSize;
	else if(type=="MAT4")  return 16*componentSize;
	else if(type=="MAT3")  return 9*componentSize;
	else if(type=="MAT2")  return 4*componentSize;
	else throw GltfError("Invalid accessor's type.");
}


vk::Format getFormat(int componentType,const string& type,bool normalize,bool wantInt=false)
{
	if(componentType>=5127)
		throw GltfError("Invalid accessor's componentType.");

	// FLOAT component type
	if(componentType==5126) {
		if(normalize)
			throw GltfError("Normalize set while accessor's componentType is FLOAT (5126).");
		if(wantInt)
			throw GltfError("Integer format asked while accessor's componentType is FLOAT (5126).");
		if(type=="VEC3")       return vk::Format::eR32G32B32Sfloat;
		else if(type=="VEC4")  return vk::Format::eR32G32B32A32Sfloat;
		else if(type=="VEC2")  return vk::Format::eR32G32Sfloat;
		else if(type=="SCALAR")  return vk::Format::eR32Sfloat;
		else if(type=="MAT4")  return vk::Format::eR32G32B32A32Sfloat;
		else if(type=="MAT3")  return vk::Format::eR32G32B32Sfloat;
		else if(type=="MAT2")  return vk::Format::eR32G32Sfloat;
		else throw GltfError("Invalid accessor's type.");
	}

	// UNSIGNED_INT component type
	else if(componentType==5125)
		throw GltfError("UNSIGNED_INT componentType shall be used only for accessors containing indices. No attribute format is supported.");

	// INT component type
	else if(componentType==5124)
		throw GltfError("Invalid componentType. INT is not valid value for glTF 2.0.");

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
		else throw GltfError("Invalid accessor's type.");
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
		else throw GltfError("Invalid accessor's type.");
		if(componentType==5120)  // signed BYTE
			base+=1;  // VK_FORMAT_R8*_S*
		if(wantInt)    return vk::Format(base+4);  // VK_FORMAT_R16*_[U|S]INT
		if(normalize)  return vk::Format(base);    // VK_FORMAT_R16*_[U|S]NORM
		else           return vk::Format(base+2);  // VK_FORMAT_R16*_[U|S]SCALED
	}

	// componentType bellow 5120
	throw GltfError("Invalid accessor's componentType.");
	return vk::Format(0);
}


int main(int argc, char* argv[])
{
	try {

		// set console code page to utf-8 to print non-ASCII characters correctly
#ifdef _WIN32
		if(!SetConsoleOutputCP(CP_UTF8))
			cout << "Failed to set console code page to utf-8." << endl;
#endif

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
		app.window.setMouseMoveCallback(
			bind(&App::mouseMove, &app, placeholders::_1, placeholders::_2)
		);
		app.window.setMouseButtonCallback(
			bind(&App::mouseButton, &app, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4)
		);
		app.window.setMouseWheelCallback(
			bind(&App::mouseWheel, &app, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4)
		);

		// show window and run main loop
		app.window.show();
		VulkanWindow::mainLoop();

		// finish all pending work on device
		app.device.waitIdle();

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

	return 0;
}
