#include <CadR/Geometry.h>
#include <CadR/Exceptions.h>
#include <CadR/Pipeline.h>
#include <CadR/StagingData.h>
#include <CadR/StateSet.h>
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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

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
	void mouseWheel(VulkanWindow& window, int wheelX, int wheelY, const VulkanWindow::MouseState& mouseState);
	void key(VulkanWindow& window, VulkanWindow::KeyState keyState, VulkanWindow::ScanCode scanCode);

	// Vulkan core objects
	// (the order of members is not arbitrary but defines construction and destruction order)
	// (it is probably good idea to destroy vulkanLib and vulkanInstance after the display connection)
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
	float fovy = 80.f / 180.f * glm::pi<float>();
	float cameraHeading=0.f;
	float cameraElevation=0.f;
	float cameraDistance=5.f;
	float startMouseX, startMouseY;
	float startCameraHeading, startCameraElevation;

	filesystem::path filePath;

	CadR::HandlelessAllocation sceneDataAllocation;
	CadR::StateSet stateSetRoot;
	array<CadR::StateSet,32> stateSetDB;
	array<CadR::Pipeline,32> pipelineDB;
	vector<CadR::Geometry> geometryDB;
	vector<CadR::Drawable> drawableDB;

};


// create_array<T,N>() allows for initialization of an std::array when passing the same value to all the constructors is needed
// (for passing references, use std::ref() and std::cref() to pass them into create_array())
template<typename T, size_t N, size_t index = N, typename T2, typename... Ts>
constexpr array<T, N> create_array_ref(T2& t, Ts&... ts)
{
	if constexpr (index <= 1)
		return array<T, N>{ t, ts... };
	else
		return create_array_ref<T, N, index-1>(t, t, ts...);
}


/// Construct application object
App::App(int argc, char** argv)
	// none of the following initializators shall be allowed to throw,
	// otherwise a special care must be given to avoid memory leaks in the case of exception
	: sceneDataAllocation(renderer.dataStorage())  // HandlelessAllocation(DataStorage&) does not throw, so it can be here
	, stateSetRoot(renderer)
	, stateSetDB{ create_array_ref<CadR::StateSet, 32>(renderer) }
	, pipelineDB{ create_array_ref<CadR::Pipeline, 32>(renderer) }
{
	// process command-line arguments
	if(argc < 2) {
		cout << "Please, specify glTF file to load." << endl;
		exit(99);
	}
	filePath = argv[1];
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
		cout << "Cannot open file " << filePath << "." << endl;
		exit(1);
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
			vulkanInstance.chooseDevice(vk::QueueFlagBits::eGraphics, window.surface());
	device.create(
		vulkanInstance, deviceAndQueueFamilies,
#if 1 // enable or disable validation extensions
		"VK_KHR_swapchain",
		CadR::Renderer::requiredFeatures()
#else
		{"VK_KHR_swapchain", "VK_KHR_shader_non_semantic_info"},
		[]() {
			CadR::Renderer::RequiredFeaturesStructChain f = CadR::Renderer::requiredFeaturesStructChain();
			f.get<vk::PhysicalDeviceVulkan12Features>().uniformAndStorageBuffer8BitAccess = true;
			return f;
		}().get<vk::PhysicalDeviceFeatures2>()
#endif
	);
	physicalDevice = std::get<0>(deviceAndQueueFamilies);
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

	// stateSets and pipelines
	pipelineLibrary.create(device);
	vk::PipelineLayout pipelineLayout = pipelineLibrary.pipelineLayout();
	for(size_t i=0; i<stateSetDB.size(); i++) {
		CadR::Pipeline& p = pipelineDB[i];
		p.init(nullptr, pipelineLayout, nullptr);
		CadR::StateSet& s = stateSetDB[i];
		s.pipeline = &p;
		s.pipelineLayout = pipelineLayout;
		stateSetRoot.childList.append(s);
	}

	// parse json
	cout << "Processing file " << filePath << "..." << endl;
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
			auto ref = newGltfItems[key];
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
	cout << "   Scenes:  " << scenes.size() << endl;
	cout << "   Nodes:   " << nodes.size() << endl;
	cout << "   Meshes:  " << meshes.size() << endl;
	cout << endl;

	// read buffers
	vector<vector<uint8_t>> bufferDataList;
	bufferDataList.reserve(buffers.size());
	for(auto& b : buffers) {

		// get path
		auto uriIt = b.find("uri");
		if(uriIt == b.end())
			throw GltfError("Unsupported functionality: Undefined buffer.uri.");
		string s = uriIt->get_ref<json::string_t&>();
		filesystem::path p = s;
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
	size_t sceneIndex = glTF.value<json::number_unsigned_t>("scene", ~size_t(0));
	if(sceneIndex == ~size_t(0)) {
		cout << "There is no default scene in the file. Using the first scene." << endl;
		sceneIndex = 0;
	}
	json& scene = scenes.at(sceneIndex);

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
			// (we need to flip y axis)
			glm::mat4 m = node.matrix;
			for(unsigned i=0; i<4; i++)
				m[i][1] = -m[i][1];

			// assign one more instancing matrix to the mesh
			if(node.meshIndex != ~size_t(0))
				meshMatrixList.at(node.meshIndex).emplace_back(m);

			// process children
			for(size_t i=0,c=node.children.size(); i<c; i++)
				processNode(node.children[i], m, nodeList, meshMatrixList, processNode);

		}
	}

	// process meshes
	cout << "Processing meshes..." << endl;
	for(size_t i=0,c=meshes.size(); i<c; i++) {

		// ignore non-instanced meshes
		if(meshMatrixList[i].empty())
			continue;

		// get mesh
		auto& mesh = meshes[i];

		// process primitives
		// (mesh.primitives are mandatory)
		auto& primitives = mesh.at("primitives");
		for(auto& primitive : primitives) {

			// material
			// (mesh.primitive.material is optional)
			json* material;
			auto it = primitive.find("material");
			if(it != primitive.end()) {
				size_t materialIndex = it->get_ref<json::number_unsigned_t&>();
				material = &materials.at(materialIndex);
			}
			else
				material = nullptr;

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
					json::string_t& t = accessor.at("type").get_ref<json::string_t&>();
					if(t != "VEC3" && t != "VEC4")
						throw GltfError("Color attribute is not of VEC3 or VEC4 type.");

					// accessor.componentType is mandatory and it must be FLOAT (5126),
					// UNSIGNED_BYTE (5121) or UNSIGNED_SHORT (5123) for color accessors
					json::number_unsigned_t& ct = accessor.at("componentType").get_ref<json::number_unsigned_t&>();
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
				else if(it.key() == "TEXCOORD_0") {

					// vertex size
					vertexSize += 16;

					// accessor
					json& accessor = accessors.at(it.value().get_ref<json::number_unsigned_t&>());

					// accessor.type is mandatory and it must be VEC2 for texCoord accessors
					json::string_t& t = accessor.at("type").get_ref<json::string_t&>();
					if(t != "VEC2")
						throw GltfError("TexCoord attribute is not of VEC2 type.");

					// accessor.componentType is mandatory and it must be FLOAT (5126),
					// UNSIGNED_BYTE (5121) or UNSIGNED_SHORT (5123) for color accessors
					json::number_unsigned_t& ct = accessor.at("componentType").get_ref<json::number_unsigned_t&>();
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
			uint32_t* indexData;
			size_t indexDataSize;
			unsigned indexComponentType;
			if(auto indicesIt=primitive.find("indices"); indicesIt!=primitive.end()) {

				// accessor
				json& accessor = accessors.at(indicesIt.value().get_ref<json::number_unsigned_t&>());

				// accessor.type is mandatory and it must be SCALAR for index accessors
				json::string_t& t = accessor.at("type").get_ref<json::string_t&>();
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
				tie(reinterpret_cast<void*&>(indexData), tmp) =
					getDataPointerAndStride(accessor, bufferViews, buffers, bufferDataList,
											numIndices, elementSize);
				indexDataSize = numIndices * sizeof(uint32_t);
			}
			else {
				numIndices = numVertices;
				indexData = nullptr;
				indexDataSize = numIndices * sizeof(uint32_t);
			}

			// ignore empty primitives
			if(indexData == nullptr && numVertices == 0)
				continue;

			// create Geometry
			cout << "Creating geometry" << endl;
			CadR::Geometry& g = geometryDB.emplace_back(renderer);

			// set primitiveSet data
			struct PrimitiveSetGpuData {
				uint32_t count;
				uint32_t first;
			};
			CadR::StagingData sd = g.createPrimitiveSetStagingData(sizeof(PrimitiveSetGpuData));
			PrimitiveSetGpuData* ps = sd.data<PrimitiveSetGpuData>();
			ps->count = uint32_t(numIndices);
			ps->first = 0;

			// mesh.primitive.mode is optional with default value 4 (TRIANGLES)
			unsigned mode = unsigned(primitive.value<json::number_unsigned_t>("mode", 4));
			if(mode != 4)
				throw GltfError("Unsupported functionality: mode is not 4 (TRIANGLES).");

			// no support for textures yet
			texCoordData = nullptr;

			// get stateSet and pipeline index
			bool doubleSided = (material) ? material->value<json::boolean_t>("doubleSided", false) : false;
			size_t pipelineIndex =
				PipelineLibrary::getPipelineIndex(
					normalData   != nullptr,  // phong
					texCoordData != nullptr,  // texturing
					colorData    != nullptr,  // perVertexColor
					!doubleSided,             // backFaceCulling
					vk::FrontFace::eCounterClockwise  // frontFace
				);
			CadR::StateSet& ss = stateSetDB[pipelineIndex];

			// drawable
			vector<glm::mat4>& matrixList = meshMatrixList[i];
			uint32_t numInstances = uint32_t(matrixList.size());
			drawableDB.emplace_back(g, 0, sd, 64+(numInstances*64), numInstances, ss);

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
			if(material) {

				// pbr material variables
				glm::vec4 baseColorFactor;
				float metallicFactor;
				float roughnessFactor;

				// read pbr material properties
				if(auto pbrIt = material->find("pbrMetallicRoughness"); pbrIt != material->end()) {

					// read baseColorFactor
					if(auto baseColorFactorIt = pbrIt->find("baseColorFactor"); baseColorFactorIt != pbrIt->end()) {
						json::array_t& a = baseColorFactorIt->get_ref<json::array_t&>();
						if(a.size() != 4)
							throw GltfError("Material.pbrMetallicRoughness.baseColorFactor is not vector of four components.");
						baseColorFactor[0] = float(a[0].get_ref<json::number_float_t&>());
						baseColorFactor[1] = float(a[1].get_ref<json::number_float_t&>());
						baseColorFactor[2] = float(a[2].get_ref<json::number_float_t&>());
						baseColorFactor[3] = float(a[3].get_ref<json::number_float_t&>());
					}

					// read properties
					metallicFactor = float(pbrIt->value<json::number_float_t>("metallicFactor", 1.0));
					roughnessFactor = float(pbrIt->value<json::number_float_t>("roughnessFactor", 1.0));

					// not supported properties
					if(auto baseColorTextureIt = pbrIt->find("baseColorTexture"); baseColorTextureIt != pbrIt->end())
						throw GltfError("Unsupported functionality: material.pbrMetallicRoughness.baseColorTexture.");
					if(pbrIt->find("metallicRoughnessTexture") != pbrIt->end())
						throw GltfError("Unsupported functionality: metallic-roughness material model.");

				}
				else
				{
					// default values when pbrMetallicRoughness is not present
					baseColorFactor = glm::vec4(1.f, 1.f, 1.f, 1.f);
					metallicFactor = 1.f;
					roughnessFactor = 1.f;
				}

				// read emissiveFactor
				glm::vec3 emissiveFactor;
				if(auto emissiveFactorIt = material->find("emissiveFactor"); emissiveFactorIt != material->end()) {
					json::array_t& a = emissiveFactorIt->get_ref<json::array_t&>();
					if(a.size() != 3)
						throw GltfError("Material.emissiveFactor is not vector of three components.");
					emissiveFactor[0] = float(a[0].get_ref<json::number_float_t&>());
					emissiveFactor[1] = float(a[1].get_ref<json::number_float_t&>());
					emissiveFactor[2] = float(a[2].get_ref<json::number_float_t&>());
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

				// set material data
				m->ambient = glm::vec3(baseColorFactor);
				m->type = 0;
				m->diffuseAndAlpha = baseColorFactor;
				m->specular = baseColorFactor * metallicFactor;  // very vague and imprecise conversion
				m->shininess = (1.f - roughnessFactor) * 128.f;  // very vague and imprecise conversion
				m->emission = emissiveFactor;
				m->pointSize = 0.f;

			}
			else {

				// set default material data
				m->ambient = glm::vec3(1.f, 1.f, 1.f);
				m->type = 0;
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

			// set vertex data
			sd = g.createVertexStagingData(numVertices * vertexSize);
			uint8_t* p = sd.data<uint8_t>();
			for(size_t i=0; i<numVertices; i++) {
				if(positionData) {
					glm::vec4 pos(*positionData, 1.f);
					*reinterpret_cast<glm::vec4*>(p)= pos;
					p += 16;
					positionData = reinterpret_cast<glm::vec3*>(reinterpret_cast<uint8_t*>(positionData) + positionDataStride);
				}
				if(normalData) {
					glm::vec4 normal = glm::vec4(*normalData, 0.f);
					*reinterpret_cast<glm::vec4*>(p) = normal;
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

			// set index data
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
	}

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
		stateSetDB[i].pipeline->set(pipelineLibrary.pipeline(i));

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
			glm::vec3(  // eye
				+cameraDistance*sin(-cameraHeading)*cos(cameraElevation),  // x
				-cameraDistance*sin(cameraElevation),  // y
				-cameraDistance*cos(cameraHeading)*cos(cameraElevation)  // z
			),
			glm::vec3(0.f,0.f,0.f),  // center
			glm::vec3(0.f,1.f,0.f)  // up
		);
	constexpr float zNear = 0.5f;
	constexpr float zFar = 100.f;
	glm::mat4 projectionMatrix = glm::perspectiveLH_ZO(fovy, float(window.surfaceExtent().width)/window.surfaceExtent().height, zNear, zFar);
	sceneData->p11 = projectionMatrix[0][0];
	sceneData->p22 = projectionMatrix[1][1];
	sceneData->p33 = projectionMatrix[2][2];
	sceneData->p43 = projectionMatrix[3][2];
	sceneData->ambientLight = glm::vec3(0.2f, 0.2f, 0.2f);
	sceneData->numLights = 0;

	// begin the frame
	size_t frameNumber = renderer.beginFrame();

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
		if(r == vk::Result::eSuboptimalKHR) {
			window.scheduleSwapchainResize();
			return;
		} else if(r == vk::Result::eErrorOutOfDateKHR) {
			window.scheduleSwapchainResize();
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
			renderer.drawablePayloadDeviceAddress(),  // payloadBufferPtr
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
			window.scheduleSwapchainResize();
			cout << "present result: Suboptimal" << endl;
		} else if(r == vk::Result::eErrorOutOfDateKHR) {
			window.scheduleSwapchainResize();
			cout << "present error: OutOfDate" << endl;
		} else
			throw runtime_error("Vulkan error: vkQueuePresentKHR() failed with error " + to_string(r) + ".");
	}
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


void App::mouseWheel(VulkanWindow& window, int wheelX, int wheelY, const VulkanWindow::MouseState& mouseState)
{
	cameraDistance -= wheelY;
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


int main(int argc,char** argv)
{
	try {

		App app(argc, argv);
		app.init();
		app.window.setRecreateSwapchainCallback(
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
		cout<<"Failed because of CadR exception: "<<e.what()<<endl;
	} catch(vk::Error &e) {
		cout<<"Failed because of Vulkan exception: "<<e.what()<<endl;
	} catch(exception &e) {
		cout<<"Failed because of exception: "<<e.what()<<endl;
	} catch(...) {
		cout<<"Failed because of unspecified exception."<<endl;
	}

	return 0;
}
