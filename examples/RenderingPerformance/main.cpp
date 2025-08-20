#include <CadR/Drawable.h>
#include <CadR/Exceptions.h>
#include <CadR/Geometry.h>
#include <CadR/Pipeline.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/StagingData.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#include <CadR/VulkanLibrary.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.hpp>
#include <iomanip>
#include <iostream>
#include <tuple>
#include "VulkanWindow.h"
#include "Tests.h"

using namespace std;
using namespace CadR;


// global variables
static const vk::Extent2D defaultImageExtent(1920, 1080);
static const constexpr auto shortTestDuration = chrono::milliseconds(2000);
static const constexpr auto longTestDuration = chrono::seconds(20);
static const string appName = "RenderingPerformance";
static const string vulkanAppName = "CadR performance test";
static const uint32_t vulkanAppVersion = VK_MAKE_VERSION(0, 0, 0);
static const string engineName = "CADR";
static const uint32_t engineVersion = VK_MAKE_VERSION(0, 0, 0);
static const uint32_t vulkanApiVersion = VK_API_VERSION_1_2;


enum class RenderingSetup {
	Performance = 0,
	Picking = 1,
};
constexpr size_t RenderingSetupCount = 2;


struct SceneGpuData {
	glm::mat4 viewMatrix;   // current camera view matrix
	float p11,p22,p33,p43;  // projectionMatrix - members that depend on zNear and zFar clipping planes
};


// shader code in SPIR-V binary
static const uint32_t vsSpirv[]={
#include "shader.vert.spv"
};
static const uint32_t vsIdBufferSpirv[]={
#include "shader-idBuffer.vert.spv"
};
static const uint32_t fsSpirv[]={
#include "shader.frag.spv"
};
static const uint32_t fsIdBufferSpirv[]={
#include "shader-idBuffer.frag.spv"
};


// global application data
class App {
public:

	App(int argc, char* argv[]);
	~App();

	void init();
	void resize(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities, vk::Extent2D newSurfaceExtent);
	void frame();
	void mainLoop();
	void printResults();

	// no move constructor and operator
	App(App&&) = delete;
	App& operator=(App&&) = delete;

	// Vulkan objects
	// (they need to be destructed in non-arbitrary order in the destructor)
	VulkanLibrary library;
	VulkanInstance instance;
	VulkanDevice device;
	vk::PhysicalDevice physicalDevice;
	uint32_t graphicsQueueFamily;
	uint32_t presentationQueueFamily;
	vk::Queue graphicsQueue;
	vk::Queue presentationQueue;
	Renderer renderer;
	array<vk::RenderPass, RenderingSetupCount> renderPassList;
	RenderingSetup renderingSetup = RenderingSetup::Performance;
	vk::Extent2D imageExtent = defaultImageExtent;
	VulkanWindow window;
	vk::SwapchainKHR swapchain;
	vk::Image colorImage;
	vk::Image depthImage;
	vk::Image idImage;
	vk::DeviceMemory colorImageMemory;
	vk::DeviceMemory depthImageMemory;
	vk::DeviceMemory idImageMemory;
	vector<vk::ImageView> colorImageViews;
	vk::ImageView depthImageView;
	vk::ImageView idImageView;
	vector<vk::Framebuffer> framebuffers;
	vk::ShaderModule vsModule;
	vk::ShaderModule fsModule;
	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;
	vk::Semaphore imageAvailableSemaphore;
	vector<vk::Semaphore> renderingFinishedSemaphoreList;
	vk::Fence renderFinishedFence;

	TestType testType = TestType::Undefined;
	bool longTest = false;
	bool useWindow = false;
	bool rasterizerDiscard = false;
	bool printFrameTimes = false;
	int deviceIndex = -1;
	string deviceNameFilter;
	size_t requestedNumTriangles = 0;
	bool calibratedTimestampsSupported = false;
	vk::ColorSpaceKHR colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;
	vk::Format depthFormat = vk::Format::eUndefined;
	glm::vec4 backgroundColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
	vk::PresentModeKHR presentationMode = vk::PresentModeKHR::eFifo;
	list<FrameInfo> frameInfoList;
	vector<double> sceneUpdateTimeList;
	chrono::steady_clock::duration realTestTime;
	uint32_t numStateSets;
	vector<CadR::StateSet*> id2stateSetMap;
	CadR::StateSet stateSetRoot;
	CadR::Pipeline pipeline;
	CadR::HandlelessAllocation sceneDataAllocation;
	union {
		struct {
			glm::mat4 projection;
			glm::mat4 view;
		} matrix;
		array<glm::mat4,2> projectionAndViewMatrix;
	};
	vector<Geometry> geometryList;
	vector<Drawable> drawableList;

};



/// Construct application object
App::App(int argc, char** argv)
	: stateSetRoot(renderer)
	, pipeline(renderer)
	, sceneDataAllocation(renderer.dataStorage())
{
	// process command-line arguments
	bool printHelp = false;
	for(int i=1; i<argc; i++) {

		// parse options starting with '-'
		if(argv[i][0] == '-') {

			// select test
			if(strcmp(argv[i], "-t") == 0 || strncmp(argv[i], "--test=", 7) == 0)
			{
				const char* start;
				if(argv[i][1] == 't') {
					if(i+1 == argc) {
						printHelp = true;
						continue;
					}
					i++;
					start = argv[i];
				}
				else
					start = &argv[i][7];

				// parse test name
				if(strcmp(start, "TrianglePerformance") == 0) {
					testType = TestType::TrianglePerformance;
					requestedNumTriangles = size_t(10 * 1e6);
				}
				else if(strcmp(start, "TriangleStripPerformance") == 0) {
					testType = TestType::TriangleStripPerformance;
					requestedNumTriangles = size_t(10 * 1e6);
				}
				else if(strcmp(start, "DrawablePerformance") == 0) {
					testType = TestType::DrawablePerformance;
					requestedNumTriangles = size_t(0.1 * 1e6);
				}
				else if(strcmp(start, "PrimitiveSetPerformance") == 0) {
					testType = TestType::PrimitiveSetPerformance;
					requestedNumTriangles = size_t(0.1 * 1e6);
				}
				else if(strcmp(start, "BakedBoxesPerformance") == 0) {
					testType = TestType::BakedBoxesPerformance;
					requestedNumTriangles = size_t(12 * 1e6);
				}
				else if(strcmp(start, "BakedBoxesScene") == 0) {
					testType = TestType::BakedBoxesScene;
					requestedNumTriangles = size_t(12 * 1e6);
				}
				else if(strcmp(start, "InstancedBoxesPerformance") == 0) {
					testType = TestType::InstancedBoxesPerformance;
					requestedNumTriangles = size_t(12 * 1e6);
				}
				else if(strcmp(start, "InstancedBoxesScene") == 0) {
					testType = TestType::InstancedBoxesScene;
					requestedNumTriangles = size_t(12 * 1e6);
				}
				else if(strcmp(start, "IndependentBoxesPerformance") == 0) {
					testType = TestType::IndependentBoxesPerformance;
					requestedNumTriangles = size_t(12 * 1e6);
				}
				else if(strcmp(start, "IndependentBoxesScene") == 0) {
					testType = TestType::IndependentBoxesScene;
					requestedNumTriangles = size_t(12 * 1e6);
				}
				else if(strcmp(start, "IndependentBoxes1000MaterialsScene") == 0) {
					testType = TestType::IndependentBoxes1000MaterialsScene;
					requestedNumTriangles = size_t(12 * 1e6);
				}
				else if(strcmp(start, "IndependentBoxes1000000MaterialsScene") == 0) {
					testType = TestType::IndependentBoxes1000000MaterialsScene;
					requestedNumTriangles = size_t(12 * 1e6);
				}
				else if(strcmp(start, "IndependentBoxesShowHideScene") == 0) {
					testType = TestType::IndependentBoxesShowHideScene;
					requestedNumTriangles = size_t(12 * 1e6);
				}
				else {
					cout << "Invalid test type \"" << start << "\"." << endl;
					printHelp = true;
				}
			}

			// select rendering setup
			else if(strcmp(argv[i], "-r") == 0 || strncmp(argv[i], "--rendering-setup=", 18) == 0)
			{
				const char* start;
				if(argv[i][1] == 'r') {
					if(i+1 == argc) {
						printHelp = true;
						continue;
					}
					i++;
					start = argv[i];
				}
				else
					start = &argv[i][18];

				// parse test name
				if(strcmp(start, "Performance") == 0)
					renderingSetup = RenderingSetup::Performance;
				else if(strcmp(start, "Picking") == 0)
					renderingSetup = RenderingSetup::Picking;
				else {
					cout << "Invalid rendering setup \"" << start << "\"." << endl;
					printHelp = true;
				}
			}

			// process simple options
			else if(strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--long") == 0)
				longTest = true;
			else if(strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--window") == 0)
				useWindow = true;
			else if(strncmp(argv[i], "-s=", 3) == 0 || strncmp(argv[i], "--size=", 7) == 0) {
				char* endp = nullptr;
				char* startp = &argv[i][argv[i][2]=='=' ? 3 : 7];
				imageExtent.width = strtoul(startp, &endp, 10);
				if((imageExtent.width == 0 && endp == startp) || *endp != 'x')
					printHelp = true;
				else {
					startp = endp + 1;
					imageExtent.height = strtoul(startp, &endp, 10);
					if(imageExtent.height == 0 && endp == startp)
						printHelp = true;
				}
			}
			else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--rasterizer-discard") == 0)
				rasterizerDiscard = true;
			else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--print-frame-times") == 0)
				printFrameTimes = true;
			else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
				printHelp = true;
			else
				printHelp = true;
		}

		// parse whatever does not start with '-'
		else {
			// parse numbers
			if(argv[i][0] >= '0' && argv[i][0] <= '9') {
				char* e = nullptr;
				deviceIndex = strtoul(argv[i], &e, 10);
				if(e == nullptr || *e != 0) {
					cout << "Invalid parameter \"" << argv[i] << "\"" << endl;
					printHelp = true;
				}
			}
			// parse text
			else
				deviceNameFilter = argv[i];
		}
	}

	// print help
	if(printHelp || testType == TestType::Undefined)
	{
		// header
		cout << appName << " tests various performance characteristics of CADR library.\n\n"
		     << "Devices in this system:" << endl;

		// get device names
		try {
			// initialize Vulkan and get device names
			library.load();
			instance.create(
				library,  // Vulkan library
				vulkanAppName.c_str(),  // application name
				vulkanAppVersion,  // application version
				engineName.c_str(),  // engine name
				engineVersion,  // engine version
				vulkanApiVersion,  // api version
				nullptr,  // enabled layers
				nullptr  // enabled extensions
			);
			vector<string> deviceNames = instance.getPhysicalDeviceNames(vk::QueueFlagBits::eGraphics);

			// print device names
			if(deviceNames.empty())
				cout << "   < no devices found >" << endl;
			else
				for(size_t i=0, c=deviceNames.size(); i<c; i++)
					cout << "   " << i << ": " << deviceNames[i] << endl;
		}
		catch(vk::Error& e) {
			cout << "   Vulkan initialization failed because of exception: " << e.what() << endl;
		}

		// print usage info and exit
		cout << "\nSimple usage:\n"
		        "   " << appName << " -t<test-name>]\n";
		cout << "\nComplete usage info:\n"
		        "   " << appName << " -t <test-name> -r <rendering-setup>\n"
		        "         [gpu name filter] [gpu index] [-l] [-p] [-h]\n"
		        "   [gpu filter name] - optional string argument used to filter devices by\n"
		        "      their names; the argument must not start with a number\n"
		        "   [gpu index] - optional device index that will be used to select device;\n"
		        "      the argument must be composed of numbers only\n"
		        "   -t <test-name> or --test=<test-name> - select particular test;\n"
		        "      valid values of <test-name>:\n"
		        "         TrianglePerformance - separated triangles rendered inbetween pixel\n"
		        "                               sampling locations; single Geometry and\n"
		        "                               single Drawable\n"
		        "         TriangleStripPerformance - connected triangles forming long strips;\n"
		        "                               the strips are rendered inbetween pixel\n"
		        "                               sampling locations; single Geometry and\n"
		        "                               single Drawable; this is the default option\n"
		        "         DrawablePerformance - each triangle is rendered by its own Drawable;\n"
		        "                               triangles are rendered inbetween pixel\n"
		        "                               sampling locations\n"
		        "         PrimitiveSetPerformance - each triangle is rendered by its own\n"
		        "                               Drawable and its own PrimitiveSet; triangles\n"
		        "                               are rendered inbetween pixel sampling locations\n"
		        "         BakedBoxesPerformance - boxes baked into single Drawable, e.g. single\n"
		        "                               draw call; boxes are small enough to be\n"
		        "                               rendered inbetween pixel sampling locations\n"
		        "         BakedBoxesScene - screen visible boxes; the boxes are baked into\n"
		        "                               a single Drawable\n"
		        "         InstancedBoxesPerformance - box in single Geometry instanced by a\n"
		        "                               single Drawable; boxes are small enough to be\n"
		        "                               rendered inbetween pixel sampling locations\n"
		        "         InstancedBoxesScene - screen visible boxes; box instanced by a\n"
		        "                               single Drawable\n"
		        "         IndependentBoxesPerformance - each box rendered by its own Drawable;\n"
		        "                               boxes are small enough to be rendered\n"
		        "                               inbetween pixel sampling locations\n"
		        "         IndependentBoxesScene - screen visible boxes; each box rendered by\n"
		        "                               its own Drawable\n"
		        "         IndependentBoxes1000MaterialsScene - screen visible boxes; each box\n"
		        "                               rendered by its own Drawable; boxes are using\n"
		        "                               1000 different materials\n"
		        "         IndependentBoxes1000000MaterialsScene - screen visible boxes;\n"
		        "                               each box rendered by its own Drawable;\n"
		        "                               boxes are using 1000000 different materials\n"
		        "         IndependentBoxesShowHideScene - screen visible boxes; each box\n"
		        "                               rendered by its own Drawable; half of boxes\n"
		        "                               is made visible and half of them invisible\n"
		        "                               each frame\n"
		        "   -r <rendering-setup> or --rendering-setup=<rendering-setup> - name of\n"
		        "      rendering setup; valid values of <rendering-setup>:\n"
		        "         Performance - rendering pipeline focused on performance without\n"
		        "                       picking capability, e.g. no id-buffer attachment\n"
		        "                       and no object ids shader output;\n"
		        "                       this is the default option\n"
		        "         Picking - picking capability included, e.g. id-buffer attachment\n"
		        "                   and shaders producing id values into the id-buffer\n"
		        "   -l or --long - perform long test instead of short one; long test takes " <<
		        chrono::duration<unsigned>(longTestDuration).count() << " seconds\n"
		        "        and short test " << setprecision(2) <<
		        chrono::duration<float>(shortTestDuration).count() <<" seconds\n"
		        "   -w or --window - perform rendering into the window\n"
		        "      instead of offscreen\n"
		        "   -s=wxh or --size=wxh - width and height of the framebuffer\n"
		        "   -d or --rasterizer-discard - discards primitives in the rasterizer\n"
		        "      just before the rasterization; no fragments are produced\n"
		        "   -p or --print-frame-times - print times of each rendered frame\n"
		        "   --help or -h - prints this usage information" << endl;
		exit(99);
	}
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

		// delete scene
		drawableList.clear();
		geometryList.clear();

		// destroy handles
		// (the handles are destructed in certain (not arbitrary) order)
		sceneDataAllocation.free();
		stateSetRoot.destroy();
		pipeline.destroyPipeline();
		pipeline.destroyPipelineLayout();
		renderer.finalize();
		device.destroy(renderFinishedFence);
		device.destroy(imageAvailableSemaphore);
		for(vk::Semaphore s : renderingFinishedSemaphoreList)
			device.destroy(s);
		renderingFinishedSemaphoreList.clear();
		device.destroy(commandPool);
		device.destroy(fsModule);
		device.destroy(vsModule);
		for(auto f : framebuffers)
			device.destroy(f);
		framebuffers.clear();
		for(auto v : colorImageViews)
			device.destroy(v);
		colorImageViews.clear();
		device.destroy(depthImageView);
		device.destroy(idImageView);
		device.free(colorImageMemory);
		device.free(depthImageMemory);
		device.free(idImageMemory);
		device.destroy(colorImage);
		device.destroy(depthImage);
		device.destroy(idImage);
		if(swapchain)
			device.destroy(swapchain);
		for(auto r : renderPassList)
			device.destroy(r);
		frameInfoList.clear();
		id2stateSetMap.clear();
		device.destroy();
	}

	if(useWindow) {
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
	}

	instance.destroy();
	library.unload();
}


void App::init()
{
	// header
	cout << appName << " tests various performance characteristics of CADR library.\n" << endl;

	// init window
	if(useWindow)
		VulkanWindow::init();

	// init Vulkan
	library.load();
	instance.create(
		library,  // Vulkan library
		vulkanAppName.c_str(),  // application name
		vulkanAppVersion,  // application version
		engineName.c_str(),  // engine name
		engineVersion,  // engine version
		vulkanApiVersion,  // api version
		nullptr,  // enabled layers
		useWindow ? VulkanWindow::requiredExtensions()  // enabled extensions
		          : vector<const char*>{}
	);
	if(useWindow) {
		window.create(VkInstance(instance), VkExtent2D(imageExtent), vulkanAppName, library.vkGetInstanceProcAddr);
		window.setResizeCallback(bind(&App::resize, this, placeholders::_2, placeholders::_3));
		window.setFrameCallback(bind(&App::frame, this));
	}

	// select device
	tie(physicalDevice, graphicsQueueFamily, presentationQueueFamily) =
		instance.chooseDevice(
			vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute,
			useWindow ? window.surface() : nullptr,
			[](CadR::VulkanInstance& instance, vk::PhysicalDevice pd) -> bool  // filterCallback
			{
				if(instance.getPhysicalDeviceProperties(pd).apiVersion < VK_API_VERSION_1_2)
					return false;
				auto features =
					instance.getPhysicalDeviceFeatures2<
						vk::PhysicalDeviceFeatures2,
						vk::PhysicalDeviceVulkan11Features,
						vk::PhysicalDeviceVulkan12Features>(pd);
				return features.get<vk::PhysicalDeviceFeatures2>().features.multiDrawIndirect &&
				       features.get<vk::PhysicalDeviceFeatures2>().features.shaderInt64 &&
				       features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
				       features.get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress;
			},
			"",  // nameFilter
			deviceIndex  // index
		);
	cout << "Device:" << endl;
	if(!physicalDevice) {
		if(deviceIndex == -1 && deviceNameFilter.empty())
			cout << "   < no devices found >" << endl;
		else {
			cout << "   < no device selected based on command line parameters >\n"
			     << "Device list:" << endl;
			vector<string> deviceNames = instance.getPhysicalDeviceNames(vk::QueueFlagBits::eGraphics);
			if(deviceNames.empty())
				cout << "   < no devices found >" << endl;
			else
				for(size_t i=0, c=deviceNames.size(); i<c; i++)
					cout << "   " << i << ": " << deviceNames[i] << endl;
		}
		exit(99);
	}
	cout << "   " << instance.getPhysicalDeviceProperties(physicalDevice).deviceName << endl;

	// test setup
	cout << "Test:" << endl;
	switch(testType) {
	case TestType::TrianglePerformance:       cout << "   TrianglePerformance" << endl; break;
	case TestType::TriangleStripPerformance:  cout << "   TriangleStripPerformance" << endl; break;
	case TestType::DrawablePerformance:       cout << "   DrawablePerformance" << endl; break;
	case TestType::PrimitiveSetPerformance:   cout << "   PrimitiveSetPerformance" << endl; break;
	case TestType::BakedBoxesPerformance:     cout << "   BakedBoxesPerformance" << endl; break;
	case TestType::BakedBoxesScene:           cout << "   BakedBoxesScene" << endl; break;
	case TestType::InstancedBoxesPerformance: cout << "   InstancedBoxesPerformance" << endl; break;
	case TestType::InstancedBoxesScene:       cout << "   InstancedBoxesScene" << endl; break;
	case TestType::IndependentBoxesPerformance:   cout << "   IndependentBoxesPerformance" << endl; break;
	case TestType::IndependentBoxesScene:         cout << "   IndependentBoxesScene" << endl; break;
	case TestType::IndependentBoxes1000MaterialsScene:    cout << "   IndependentBoxes1000MaterialsScene" << endl; break;
	case TestType::IndependentBoxes1000000MaterialsScene: cout << "   IndependentBoxes1000000MaterialsScene" << endl; break;
	case TestType::IndependentBoxesShowHideScene: cout << "   IndependentBoxesShowHideScene" << endl; break;
	default: cout << "   Undefined" << endl;
	};
	cout << "Rendering setup:" << endl;
	switch(renderingSetup) {
	case RenderingSetup::Performance: cout << "   Performance"; break;
	case RenderingSetup::Picking:     cout << "   Picking"; break;
	default: cout << "   Undefined";
	};
	if(rasterizerDiscard)
		cout << ", rasterizerDiscard";
	cout << "\n" << endl;

	// create device
	vk::PhysicalDeviceFeatures2 features2(Renderer::requiredFeatures());
	if(renderingSetup == RenderingSetup::Picking)
		features2.features.setGeometryShader(true);
	device.create(
		instance,  // instance
		physicalDevice,
		graphicsQueueFamily,
		presentationQueueFamily,
	#if 0 // enable to use debugPrintfEXT in shader code
		useWindow ? vector<const char*>{"VK_KHR_swapchain", "VK_KHR_shader_non_semantic_info"}  // extensions
		          : vector<const char*>{"VK_KHR_shader_non_semantic_info"},
	#else
		useWindow ? vector<const char*>{"VK_KHR_swapchain"}  // extensions
		          : vector<const char*>{},
	#endif
		features2  // features
	);
	graphicsQueue = device.getQueue(graphicsQueueFamily, 0);
	presentationQueue = device.getQueue(presentationQueueFamily, 0);
	renderer.init(device, instance, physicalDevice, graphicsQueueFamily);
	renderer.setCollectFrameInfo(true, calibratedTimestampsSupported);
	if(useWindow)
		window.setDevice(VkDevice(device), physicalDevice);

	// choose surface format
	if(useWindow)
		tie(colorFormat, colorSpace) =
			[](vk::PhysicalDevice physicalDevice, CadR::VulkanInstance& vulkanInstance, vk::SurfaceKHR surface)
				-> tuple<vk::Format, vk::ColorSpaceKHR>
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
					return {candidateFormats[0].format, candidateFormats[0].colorSpace};
				else {
					for(vk::SurfaceFormatKHR sf : availableFormats) {
						auto it = std::find(candidateFormats.begin(), candidateFormats.end(), sf);
						if(it != candidateFormats.end())
							return {it->format, it->colorSpace};
					}
					if(availableFormats.size() == 0)  // Vulkan must return at least one format (this is mandated since Vulkan 1.0.37 (2016-10-10), but was missing in the spec before probably because of omission)
						throw std::runtime_error("Vulkan error: getSurfaceFormatsKHR() returned empty list.");
					return {availableFormats[0].format, availableFormats[0].colorSpace};
				}
			}(physicalDevice, instance, window.surface());

	// depth format
	bool depthFormatIsFloat;
	tie(depthFormat, depthFormatIsFloat) =
		[](App& app){
			constexpr const array<tuple<vk::Format,bool>, 2> candidateFormats {
				tuple{ vk::Format::eD32SfloatS8Uint, true },
				tuple{ vk::Format::eD24UnormS8Uint, false },
			};
			for(tuple f : candidateFormats)
			{
				vk::FormatProperties p = app.instance.getPhysicalDeviceFormatProperties(app.physicalDevice, get<0>(f));
				if(p.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
					return f;
			}
			throw std::runtime_error("RI Vulkan error: No suitable depth buffer format.");
		}(*this);

	// present mode
	if(useWindow) {
		vector<vk::PresentModeKHR> presentModes =
			instance.getPhysicalDeviceSurfacePresentModesKHR(physicalDevice, window.surface());
		if(find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end())
			presentationMode = vk::PresentModeKHR::eMailbox;
		else if(find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eImmediate) != presentModes.end())
			presentationMode = vk::PresentModeKHR::eImmediate;
	}

	// render passes
	auto createRenderPass =
		[](App& app, RenderingSetup renderingSetup)
		{
			return 
				app.device.createRenderPass(
					vk::RenderPassCreateInfo(
						vk::RenderPassCreateFlags(),  // flags
						(renderingSetup == RenderingSetup::Picking) ? 3 :2,  // attachmentCount
						array<const vk::AttachmentDescription, 3>{  // pAttachments
							vk::AttachmentDescription{  // color attachment
								vk::AttachmentDescriptionFlags(),  // flags
								app.colorFormat,                   // format
								vk::SampleCountFlagBits::e1,       // samples
								vk::AttachmentLoadOp::eClear,      // loadOp
								vk::AttachmentStoreOp::eStore,     // storeOp
								vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
								vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
								vk::ImageLayout::eUndefined,       // initialLayout
								app.useWindow                      // finalLayout
									? vk::ImageLayout::ePresentSrcKHR
									: vk::ImageLayout::eColorAttachmentOptimal,
							},
							vk::AttachmentDescription{  // depth attachment
								vk::AttachmentDescriptionFlags(),  // flags
								app.depthFormat,                   // format
								vk::SampleCountFlagBits::e1,       // samples
								vk::AttachmentLoadOp::eClear,      // loadOp
								(renderingSetup == RenderingSetup::Picking)  // storeOp
									? vk::AttachmentStoreOp::eStore
									: vk::AttachmentStoreOp::eDontCare,
								vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
								vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
								vk::ImageLayout::eUndefined,       // initialLayout
								vk::ImageLayout::eDepthStencilReadOnlyOptimal  // finalLayout
							},
							vk::AttachmentDescription{  // id-buffer attachment
								vk::AttachmentDescriptionFlags(),  // flags
								vk::Format::eR32G32B32A32Uint,     // format, support of eR32G32B32A32Uint format is mandatory in Vulkan
								vk::SampleCountFlagBits::e1,       // samples
								vk::AttachmentLoadOp::eClear,      // loadOp
								vk::AttachmentStoreOp::eStore,     // storeOp
								vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
								vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
								vk::ImageLayout::eUndefined,       // initialLayout
								vk::ImageLayout::eShaderReadOnlyOptimal  // finalLayout
							},
						}.data(),
						1,  // subpassCount
						&(const vk::SubpassDescription&)vk::SubpassDescription(  // pSubpasses
							vk::SubpassDescriptionFlags(),     // flags
							vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
							0,        // inputAttachmentCount
							nullptr,  // pInputAttachments
							(renderingSetup == RenderingSetup::Picking) ? 2 : 1,  // colorAttachmentCount
							array{    // pColorAttachments
								vk::AttachmentReference(
									0,  // attachment
									vk::ImageLayout::eColorAttachmentOptimal  // layout
								),
								vk::AttachmentReference(
									2,  // attachment
									vk::ImageLayout::eColorAttachmentOptimal  // layout
								),
							}.data(),
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
								vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // srcStageMask - eColorAttachmentOutput is required because of WSI and should match pWaitDstStageMask
								vk::PipelineStageFlags(vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eVertexInput |  // dstStageMask
									vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eEarlyFragmentTests |
									vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eLateFragmentTests |
									vk::PipelineStageFlagBits::eColorAttachmentOutput),
								vk::AccessFlags(),     // srcAccessMask
								vk::AccessFlags(vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eShaderRead |  // dstAccessMask
									vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite |
									vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite),
								vk::DependencyFlags()  // dependencyFlags
							),
							vk::SubpassDependency(
								0,                    // srcSubpass
								VK_SUBPASS_EXTERNAL,  // dstSubpass
								vk::PipelineStageFlags(vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eVertexInput |  // srcStageMask
									vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eEarlyFragmentTests |
									vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eLateFragmentTests |
									vk::PipelineStageFlagBits::eColorAttachmentOutput),
								vk::PipelineStageFlags(vk::PipelineStageFlagBits::eBottomOfPipe),  // dstStageMask
								vk::AccessFlags(vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead |  // srcAccessMask
									vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eShaderRead |
									vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite |
									vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite),
								vk::AccessFlags(),     // dstAccessMask
								vk::DependencyFlags()  // dependencyFlags
							),
						}.data()
					)
				);
		};
	renderPassList = {
		createRenderPass(*this, RenderingSetup::Performance),
		createRenderPass(*this, RenderingSetup::Picking),
	};

	// create shader modules
	vsModule =
		device.createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				(renderingSetup == RenderingSetup::Picking) ? sizeof(vsIdBufferSpirv) : sizeof(vsSpirv),  // codeSize
				(renderingSetup == RenderingSetup::Picking) ? vsIdBufferSpirv : vsSpirv  // pCode
			)
		);
	fsModule =
		device.createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				(renderingSetup == RenderingSetup::Picking) ? sizeof(fsIdBufferSpirv) : sizeof(fsSpirv),  // codeSize
				(renderingSetup == RenderingSetup::Picking) ? fsIdBufferSpirv : fsSpirv // pCode
			)
		);

	// pipeline layout
	// (pipeline will be supplied in resize() because we need to know framebuffer extent)
	pipeline.init(
		nullptr,  // pipeline
		device.createPipelineLayout(  // pipelineLayout
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				0,       // setLayoutCount
				nullptr, // pSetLayouts
				2,  // pushConstantRangeCount
				array{  // pPushConstantRanges
					vk::PushConstantRange{
						vk::ShaderStageFlagBits::eVertex,  // stageFlags
						0,  // offset
						2*sizeof(uint64_t)  // size
					},
					vk::PushConstantRange{
						vk::ShaderStageFlagBits::eFragment,  // stageFlags
						8,  // offset
						sizeof(uint64_t) + sizeof(uint32_t)  // size
					},
				}.data()
			}
		),
		{}  // descriptorSetLayouts
	);

	// sceneDataAllocation
	sceneDataAllocation.alloc(sizeof(SceneGpuData));

	// command pool
	commandPool =
		device.createCommandPool(
			vk::CommandPoolCreateInfo(
				vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,  // flags
				graphicsQueueFamily            // queueFamilyIndex
			)
		);

	// allocate command buffer
	commandBuffer = std::move(
		device.allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				commandPool,                       // commandPool
				vk::CommandBufferLevel::ePrimary,  // level
				1                                  // commandBufferCount
			)
		)[0]);

	// rendering semaphores and fences
	imageAvailableSemaphore =
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

	// scene
	stateSetRoot.pipeline = &pipeline;

	// do resize
	if(!useWindow)
		resize({}, imageExtent);
}


void App::resize(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities, vk::Extent2D newExtent)
{
	// clear resources
	device.destroy(colorImage);
	device.destroy(depthImage);
	device.destroy(idImage);
	for(auto v : colorImageViews)  device.destroy(v);
	device.destroy(depthImageView);
	device.destroy(idImageView);
	device.free(colorImageMemory);
	device.free(depthImageMemory);
	device.free(idImageMemory);
	for(auto f : framebuffers)  device.destroy(f);
	colorImage = nullptr;
	depthImage = nullptr;
	idImage = nullptr;
	colorImageViews.clear();
	depthImageView = nullptr;
	idImageView = nullptr;
	colorImageMemory = nullptr;
	depthImageMemory = nullptr;
	idImageMemory = nullptr;
	framebuffers.clear();

	// projection and view matrix
	// ZO - Zero to One is output depth range,
	// RH - Right Hand coordinate system, +Y is down, +Z is towards camera
	// LH - Left Hand coordinate system, +Y is down, +Z points into the scene
	constexpr float zNear = 0.5f;
	constexpr float zFar = 100.f;
	matrix.projection =
		glm::orthoLH_ZO(
			-float(newExtent.width) / 2.f,  // left
			 float(newExtent.width) / 2.f,  // right
			-float(newExtent.height) / 2.f,  // bottom
			 float(newExtent.height) / 2.f,  // top
			zNear,  // near
			zFar  // far
		);

	// pipeline specialization constants
	const array<float,6> specializationConstants{
		matrix.projection[2][0], matrix.projection[2][1], matrix.projection[2][3],
		matrix.projection[3][0], matrix.projection[3][1], matrix.projection[3][3],
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

	// recreate pipeline
	auto pipelineUnique =
		device.createGraphicsPipelineUnique(
			nullptr,  // pipelineCache
			vk::GraphicsPipelineCreateInfo(
				vk::PipelineCreateFlags(),  // flags

				// shader stages
				(rasterizerDiscard) ? 1 : 2,  // stageCount
				array{  // pStages
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eVertex,      // stage
						vsModule,  // module
						"main",  // pName
						&specializationInfo  // pSpecializationInfo
					},
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eFragment,    // stage
						fsModule,  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					},
				}.data(),

				// vertex input
				&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{
					vk::PipelineVertexInputStateCreateFlags(),  // flags
					0,  // vertexBindingDescriptionCount
					nullptr,  // pVertexBindingDescriptions
					0,  // vertexAttributeDescriptionCount
					nullptr  // pVertexAttributeDescriptions
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
						vk::Viewport(0.f, 0.f, float(newExtent.width), float(newExtent.height), 0.f, 1.f),
					}.data(),
					1,  // scissorCount
					array{  // pScissors
						vk::Rect2D({0, 0}, newExtent)
					}.data(),
				},

				// rasterization
				&(const vk::PipelineRasterizationStateCreateInfo&)vk::PipelineRasterizationStateCreateInfo{  // pRasterizationState
					vk::PipelineRasterizationStateCreateFlags(),  // flags
					VK_FALSE,  // depthClampEnable
					vk::Bool32(rasterizerDiscard ? VK_TRUE : VK_FALSE),  // rasterizerDiscardEnable
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

				// blending
				&(const vk::PipelineColorBlendStateCreateInfo&)vk::PipelineColorBlendStateCreateInfo{  // pColorBlendState
					vk::PipelineColorBlendStateCreateFlags(),  // flags
					VK_FALSE,  // logicOpEnable
					vk::LogicOp::eClear,  // logicOp
					(renderingSetup == RenderingSetup::Picking) ? 2u : 1u,  // attachmentCount
					array{  // pAttachments
						vk::PipelineColorBlendAttachmentState{
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
						vk::PipelineColorBlendAttachmentState{
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
					}.data(),
					array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
				},

				nullptr,  // pDynamicState
				pipeline.layout(),  // layout
				renderPassList[uint32_t(renderingSetup)],  // renderPass
				0,  // subpass
				vk::Pipeline(nullptr),  // basePipelineHandle
				-1 // basePipelineIndex
			)
		);
	pipeline.destroyPipeline();
	pipeline.set(pipelineUnique.release());

	if(useWindow) {

		// print info
		cout << "Recreating swapchain (extent: " << newExtent.width << "x" << newExtent.height
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
					colorFormat,                    // imageFormat
					colorSpace,                     // imageColorSpace
					newExtent,                      // imageExtent
					1,                              // imageArrayLayers
					vk::ImageUsageFlagBits::eColorAttachment,  // imageUsage
					(graphicsQueueFamily==presentationQueueFamily) ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent, // imageSharingMode
					uint32_t(2),  // queueFamilyIndexCount
					array<uint32_t, 2>{graphicsQueueFamily, presentationQueueFamily}.data(),  // pQueueFamilyIndices
					surfaceCapabilities.currentTransform,    // preTransform
					vk::CompositeAlphaFlagBitsKHR::eOpaque,  // compositeAlpha
					presentationMode,  // presentMode
					VK_TRUE,  // clipped
					swapchain  // oldSwapchain
				)
			);
		device.destroy(swapchain);
		swapchain = newSwapchain.release();

		// swapchain images and image views
		vector<vk::Image> swapchainImages = device.getSwapchainImagesKHR(swapchain);
		colorImageViews.reserve(swapchainImages.size());
		for(vk::Image image : swapchainImages)
			colorImageViews.emplace_back(
				device.createImageView(
					vk::ImageViewCreateInfo(
						vk::ImageViewCreateFlags(),  // flags
						image,                       // image
						vk::ImageViewType::e2D,      // viewType
						colorFormat,                 // format
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

		// rendering finished semaphores
		vk::SemaphoreCreateInfo semaphoreCreateInfo{
			vk::SemaphoreCreateFlags()  // flags
		};
		renderingFinishedSemaphoreList.reserve(swapchainImages.size());
		for(size_t i=0,c=swapchainImages.size(); i<c; i++)
			renderingFinishedSemaphoreList.emplace_back(
				device.createSemaphore(semaphoreCreateInfo));

	} else {

		colorImage =
			device.createImage(
				vk::ImageCreateInfo(
					vk::ImageCreateFlags(),       // flags
					vk::ImageType::e2D,           // imageType
					colorFormat,                  // format
					vk::Extent3D(newExtent, 1),   // extent
					1,                            // mipLevels
					1,                            // arrayLayers
					vk::SampleCountFlagBits::e1,  // samples
					vk::ImageTiling::eOptimal,    // tiling
					vk::ImageUsageFlagBits::eColorAttachment,  // usage
					vk::SharingMode::eExclusive,  // sharingMode
					0,                            // queueFamilyIndexCount
					nullptr,                      // pQueueFamilyIndices
					vk::ImageLayout::eUndefined   // initialLayout
				)
			);

	}

	// depth image
	depthImage =
		device.createImage(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				depthFormat,             // format
				vk::Extent3D(newExtent, 1),  // extent
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

	// id image
	idImage =
		device.createImage(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				vk::Format::eR32G32B32A32Uint,  // format, support of eR32G32B32A32Uint format is mandatory in Vulkan
				vk::Extent3D(newExtent, 1),  // extent
				1,                       // mipLevels
				1,                       // arrayLayers
				vk::SampleCountFlagBits::e1,  // samples
				vk::ImageTiling::eOptimal,    // tiling
				vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,  // usage
				vk::SharingMode::eExclusive,  // sharingMode
				0,                            // queueFamilyIndexCount
				nullptr,                      // pQueueFamilyIndices
				vk::ImageLayout::eUndefined   // initialLayout
			)
		);

	// memory for images
	vk::PhysicalDeviceMemoryProperties memoryProperties = instance.getPhysicalDeviceMemoryProperties(physicalDevice);
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
	if(!useWindow)
		colorImageMemory = allocateMemory(device, colorImage, vk::MemoryPropertyFlagBits::eDeviceLocal, memoryProperties);
	depthImageMemory = allocateMemory(device, depthImage, vk::MemoryPropertyFlagBits::eDeviceLocal, memoryProperties);
	idImageMemory    = allocateMemory(device, idImage,    vk::MemoryPropertyFlagBits::eDeviceLocal, memoryProperties);
	if(!useWindow)
		device.bindImageMemory(
			colorImage,        // image
			colorImageMemory,  // memory
			0                  // memoryOffset
		);
	device.bindImageMemory(
		depthImage,        // image
		depthImageMemory,  // memory
		0                  // memoryOffset
	);
	device.bindImageMemory(
		idImage,        // image
		idImageMemory,  // memory
		0               // memoryOffset
	);

	// image views
	if(!useWindow)
		colorImageViews.push_back(
			device.createImageView(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),  // flags
					colorImage,                  // image
					vk::ImageViewType::e2D,      // viewType
					colorFormat,                 // format
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
	idImageView =
		device.createImageView(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),  // flags
				idImage,                     // image
				vk::ImageViewType::e2D,      // viewType
				vk::Format::eR32G32B32A32Uint,  // format, support of eR32G32B32A32Uint format is mandatory in Vulkan
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

	// framebuffers
	framebuffers.reserve(colorImageViews.size());
	for(size_t i=0, c=colorImageViews.size(); i<c; i++)
		framebuffers.emplace_back(
			device.createFramebuffer(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),  // flags
					renderPassList[uint32_t(renderingSetup)],  // renderPass
					(renderingSetup == RenderingSetup::Picking) ? 3 : 2,  // attachmentCount
					array<vk::ImageView, 3>{  // pAttachments
						colorImageViews[i],
						depthImageView,
						idImageView,
					}.data(),
					newExtent.width,  // width
					newExtent.height,  // height
					1  // layers
				)
			)
		);

	// create new test scene
	drawableList.clear();
	geometryList.clear();
	createTestScene(testType, int(newExtent.width), int(newExtent.height),
		renderer, stateSetRoot, geometryList, drawableList);

	// update image size
	imageExtent = newExtent;
}


void App::frame()
{
	// wait for previous frame rendering work
	// if still not finished
	// (we might start copy operations before, but we need to exclude TableHandles that must stay intact
	// until the rendering is finished)
	vk::Result r =
		device.waitForFences(
			renderFinishedFence,  // fences (vk::ArrayProxy)
			VK_TRUE,       // waitAll
			uint64_t(3e9)  // timeout (3s)
		);
	if(r != vk::Result::eSuccess) {
		if(r == vk::Result::eTimeout)
			throw runtime_error("GPU timeout. Task is probably hanging on GPU.");
		throw runtime_error("Vulkan error: vkWaitForFences failed with error " + to_string(r) + ".");
	}
	device.resetFences(renderFinishedFence);

	// collect previous frame info
	frameInfoList.push_back(renderer.getFrameInfo());

	// update scene
	if(testType == TestType::IndependentBoxesShowHideScene) {
		chrono::time_point startTime = chrono::high_resolution_clock::now();
		updateTestScene(
			testType,
			imageExtent.width,
			imageExtent.height,
			renderer,
			stateSetRoot,
			geometryList,
			drawableList);
		chrono::time_point finishTime = chrono::high_resolution_clock::now();
		double updateTime = chrono::duration<double>(finishTime - startTime).count();
		sceneUpdateTimeList.push_back(updateTime);
	}

	// view matrix
	// (set it to identity for even frames and shift it by 1 in X direction for odd frames)
	float x = (renderer.frameNumber() & 0x01) ? 0.f : 1.f;
	matrix.view =
		glm::lookAtLH(
			glm::vec3(x,0,-50),  // eye
			glm::vec3(x,0,0),  // center
			glm::vec3(0,-1,0));  // up

	// begin new frame
	renderer.beginFrame();

	// update SceneGpuData
	CadR::StagingData stagingSceneData = sceneDataAllocation.createStagingData();
	SceneGpuData* sceneData = stagingSceneData.data<SceneGpuData>();
	assert(sizeof(SceneGpuData) == sceneDataAllocation.size());
	sceneData->viewMatrix = matrix.view;
	sceneData->p11 = matrix.projection[0][0];
	sceneData->p22 = matrix.projection[1][1];
	sceneData->p33 = matrix.projection[2][2];
	sceneData->p43 = matrix.projection[3][2];

	// submit all copy operations that were not submitted yet
	renderer.executeCopyOperations();

	// acquire image
	uint32_t imageIndex;
	if(!useWindow)
		imageIndex = 0;
	else {
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
	}

	// begin command buffer recording
	renderer.beginRecording(commandBuffer);

	// prepare recording
	numStateSets = 1;  // value zero reserved for no container
	size_t numDrawables = renderer.prepareSceneRendering(stateSetRoot);
	id2stateSetMap.clear();
	if(id2stateSetMap.capacity() < numStateSets)
		id2stateSetMap.reserve(numStateSets+(numStateSets>>4));  // make the capacity 1/16 bigger to avoid reallocations on slowly growing scenes
	else if(id2stateSetMap.capacity() >= 4*numStateSets) {
		auto stateSets = decltype(id2stateSetMap){};
		id2stateSetMap.swap(stateSets);
		id2stateSetMap.reserve(numStateSets+(numStateSets>>4));  // make the capacity 1/16 bigger to avoid reallocations on slowly growing scenes
	}
	id2stateSetMap.emplace_back(nullptr);  // no container for value zero

	// record compute shader preprocessing
	renderer.recordDrawableProcessing(commandBuffer, numDrawables);

	// record scene rendering
	device.cmdPushConstants(
		commandBuffer,  // commandBuffer
		pipeline.layout(),  // pipelineLayout
		vk::ShaderStageFlagBits::eVertex,  // stageFlags
		0,  // offset
		sizeof(uint64_t),  // size
		array<uint64_t,1>{  // pValues
			renderer.drawablePointersBufferAddress(),  // payloadBufferPtr
		}.data()
	);
	device.cmdPushConstants(
		commandBuffer,  // commandBuffer
		pipeline.layout(),  // pipelineLayout
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
		renderPassList[uint32_t(renderingSetup)],  // renderPass
		framebuffers[imageIndex],  // framebuffer
		vk::Rect2D(vk::Offset2D(0, 0), imageExtent),  // renderArea
		(renderingSetup == RenderingSetup::Picking) ? 3u : 2u,  // clearValueCount
		array<vk::ClearValue, 3>{  // pClearValues
			vk::ClearColorValue(*reinterpret_cast<const array<float, 4>*>(glm::value_ptr(backgroundColor))),
			vk::ClearDepthStencilValue(1.f, 0),
			vk::ClearColorValue(array<uint32_t,4>{ 0, 0, 0, 0 }),
		}.data()
	);

	// end command buffer recording
	renderer.endRecording(commandBuffer);

	// submit all pending copy operations
	renderer.executeCopyOperations();

	if(!useWindow) {

		// render
		device.queueSubmit(
			graphicsQueue,  // queue
			vk::SubmitInfo(
				0, nullptr, nullptr,  // waitSemaphoreCount, pWaitSemaphores, pWaitDstStageMask
				1, &commandBuffer,    // commandBufferCount+pCommandBuffers
				0, nullptr            // signalSemaphoreCount, pSignalSemaphores
			),
			renderFinishedFence  // fence
		);

	}
	else {

		// submit frame
		vk::Semaphore renderingFinishedSemaphore = renderingFinishedSemaphoreList[imageIndex];
		device.queueSubmit(
			graphicsQueue,  // queue
			vk::SubmitInfo(
				1, &imageAvailableSemaphore,  // waitSemaphoreCount + pWaitSemaphores +
				&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(  // pWaitDstStageMask
					vk::PipelineStageFlagBits::eColorAttachmentOutput),
				1, &commandBuffer,  // commandBufferCount + pCommandBuffers
				1, &renderingFinishedSemaphore  // signalSemaphoreCount + pSignalSemaphores
			),
			renderFinishedFence  // fence
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

	}

	// mark the end of the frame
	renderer.endFrame();
}


void App::mainLoop()
{
	if(!useWindow) {

		chrono::steady_clock::duration testTime = longTest ? longTestDuration : shortTestDuration;
		chrono::steady_clock::time_point startTime = chrono::steady_clock::now();
		chrono::steady_clock::time_point finishTime = startTime + testTime;
		chrono::steady_clock::time_point t;

		do {
			frame();
		} while(chrono::steady_clock::now() < finishTime);

		device.waitIdle();
		realTestTime = chrono::steady_clock::now() - startTime;
		frameInfoList.push_back(renderer.getFrameInfo());

	}
	else {

		window.show();

		chrono::steady_clock::duration testTime = longTest ? longTestDuration : shortTestDuration;
		chrono::steady_clock::time_point startTime = chrono::steady_clock::now();
		chrono::steady_clock::time_point finishTime = startTime + testTime;

		window.setFrameCallback(
			bind(
				[](App& app, chrono::steady_clock::time_point finishTime){
					app.frame();
					if(chrono::steady_clock::now() < finishTime)
						app.window.scheduleFrame();
					else
						VulkanWindow::exitMainLoop();
				},
				ref(*this),
				finishTime
			)
		);
		VulkanWindow::mainLoop();

		device.waitIdle();
		realTestTime = chrono::steady_clock::now() - startTime;
		frameInfoList.push_back(renderer.getFrameInfo());
	}
}


void App::printResults()
{
	// print results, start with the header of output
	// (using log-level D_OFF to do it always because it was requested by cmd-line argument)
	if (testType == TestType::TrianglePerformance)
		cout << "Triangle performance test of " << size_t(requestedNumTriangles/1e6) << " Mtri ("
		     << requestedNumTriangles << " triangles) inside 1 drawable." << endl;
	else if (testType == TestType::TriangleStripPerformance)
		cout << "Triangle strip performance test of " << size_t(requestedNumTriangles/1e6) << " Mtri ("
		     << requestedNumTriangles << " triangles) inside 1 drawable." << endl;
	else if (testType == TestType::DrawablePerformance)
		cout << "Drawable performance test of " << size_t(requestedNumTriangles / 1e6)
		     << " Mtri in " << size_t(requestedNumTriangles / 1e6) << " Mdrawables ("
		     << requestedNumTriangles << " triangles in " << requestedNumTriangles << " drawables)." << endl;
	else if (testType == TestType::PrimitiveSetPerformance)
		cout << "PrimitiveSet performance test of " << size_t(requestedNumTriangles / 1e6)
		     << " Mtri in " << size_t(requestedNumTriangles / 1e6) << " Mdrawables ("
		     << requestedNumTriangles << " triangles in " << requestedNumTriangles << " drawables)." << endl;
	else if (testType == TestType::BakedBoxesPerformance)
		cout << "Baked boxes performance test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
		     << "M (" << size_t(requestedNumTriangles + 11) / 12 << ") boxes ("
		     << requestedNumTriangles << " triangles in 1 drawable)." << endl;
	else if (testType == TestType::BakedBoxesScene)
		cout << "Baked boxes scene test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
		     << "M (" << size_t(requestedNumTriangles + 11) / 12 << ") boxes ("
		     << requestedNumTriangles << " triangles in 1 drawable)." << endl;
	else if (testType == TestType::InstancedBoxesPerformance)
		cout << "Instanced boxes performance test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
		     << "M (" << size_t(requestedNumTriangles + 11) / 12 << ") boxes ("
		     << requestedNumTriangles << " triangles in 1 drawable with "
		     << size_t(requestedNumTriangles + 11) / 12 << " transformations)." << endl;
	else if (testType == TestType::InstancedBoxesScene)
		cout << "Instanced boxes scene test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
		     << "M (" << size_t(requestedNumTriangles + 11) / 12 << ") boxes ("
		     << requestedNumTriangles << " triangles in 1 drawable with "
		     << size_t(requestedNumTriangles + 11) / 12 << " transformations)." << endl;
	else if (testType == TestType::IndependentBoxesPerformance)
		cout << "Independent boxes performance test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
		     << "M (" << size_t((requestedNumTriangles + 11) / 12) << ") boxes ("
		     << requestedNumTriangles << " triangles in " << size_t((requestedNumTriangles + 11) / 12)
		     << " drawables)." << endl;
	else if (testType == TestType::IndependentBoxesScene)
		cout << "Independent boxes scene test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
		     << "M (" << size_t(requestedNumTriangles + 11) / 12 << ") boxes ("
		     << requestedNumTriangles << " triangles in " << size_t(requestedNumTriangles + 11) / 12 << " drawables)." << endl;
	else if (testType == TestType::IndependentBoxes1000MaterialsScene)
		cout << "Independent boxes with 1000 materials scene test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
		     << "M (" << size_t(requestedNumTriangles + 11) / 12 << ") boxes ("
		     << requestedNumTriangles << " triangles in " << size_t(requestedNumTriangles + 11) / 12 << " drawables)." << endl;
	else if (testType == TestType::IndependentBoxesScene)
		cout << "Independent boxes with 1000000 materials scene test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
		     << "M (" << size_t(requestedNumTriangles + 11) / 12 << ") boxes ("
		     << requestedNumTriangles << " triangles in " << size_t(requestedNumTriangles + 11) / 12 << " drawables)." << endl;
	else if (testType == TestType::IndependentBoxesShowHideScene)
		cout << "Independent boxes show and hide scene test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
		     << "M (" << size_t(requestedNumTriangles + 11) / 12 << ") boxes ("
		     << requestedNumTriangles << " triangles in " << size_t(requestedNumTriangles + 11) / 12 << " drawables)." << endl;

	// if no measurements
	if(frameInfoList.empty()) {
		cout << "      No measurements made." << endl;
		return;
	}

	// get time info
	struct FrameTimeInfo
	{
		size_t frameId;
		double totalTime;
		double cpuTime;
		double gpuTime;
		double gpuPreparations;
		double gpuDrawableProcessing;
		double gpuRendering;
	};
	vector<FrameTimeInfo> frameTimeList;
	frameTimeList.reserve(frameInfoList.size());
	for(FrameInfo& info : frameInfoList) {
		double cpuTime = double(info.cpuEndFrame - info.cpuBeginFrame) * renderer.cpuTimestampPeriod();
		double gpuTime = double(info.gpuEndExecution - info.gpuBeginExecution) * renderer.gpuTimestampPeriod();
		double totalTime = max(double(info.gpuEndExecution - info.gpuBeginFrame) * renderer.gpuTimestampPeriod(),
		                       cpuTime);
		double gpuPreparations = double(info.gpuAfterTransfersAndBeforeDrawableProcessing - info.gpuBeginExecution) * renderer.gpuTimestampPeriod();
		double gpuDrawableProcessing = double(info.gpuAfterDrawableProcessingAndBeforeRendering - info.gpuAfterTransfersAndBeforeDrawableProcessing) * renderer.gpuTimestampPeriod();
		double gpuRendering = double(info.gpuEndExecution - info.gpuAfterDrawableProcessingAndBeforeRendering) * renderer.gpuTimestampPeriod();
		frameTimeList.emplace_back(FrameTimeInfo{info.frameNumber, totalTime, cpuTime, gpuTime, gpuPreparations, gpuDrawableProcessing, gpuRendering });
	}

	// print median frame
	cout << "   Median results:" << endl;
	auto median = [](vector<FrameTimeInfo>& l, double (*getElement)(FrameTimeInfo&)) -> double {
		vector<double> v;
		v.reserve(l.size());
		for (FrameTimeInfo& i : l)
			v.push_back(getElement(i));
		std::nth_element(v.begin(), v.begin() + l.size() / 2, v.end());
		return v[l.size() / 2];
	};
	double performance = double(requestedNumTriangles) / median(frameTimeList, [](FrameTimeInfo& i) { return i.totalTime; });
	if(performance <= 1e9)
		cout << "      Performance: " << size_t(performance / 1e6 + 0.5) << " MTri/s" << endl;
	else
		cout << "      Performance: " << fixed << setprecision(2) << performance / 1e9 << " GTri/s" << endl;
	cout << fixed << setprecision(3);
	cout << "      Total time:  " << median(frameTimeList, [](FrameTimeInfo& i) { return i.totalTime; }) * 1000 << "ms" << endl;
	cout << "      Cpu time:    " << median(frameTimeList, [](FrameTimeInfo& i) { return i.cpuTime; }) * 1000 << "ms" << endl;
	cout << "      Gpu time:    " << median(frameTimeList, [](FrameTimeInfo& i) { return i.gpuTime; }) * 1000 << "ms" << endl;
	cout << "      Gpu preparation time:          " << median(frameTimeList, [](FrameTimeInfo& i) { return i.gpuPreparations; }) * 1000 << "ms" << endl;
	cout << "      Gpu drawable processing time:  " << median(frameTimeList, [](FrameTimeInfo& i) { return i.gpuDrawableProcessing; }) * 1000 << "ms" << endl;
	cout << "      Gpu rendering time:            " << median(frameTimeList, [](FrameTimeInfo& i) { return i.gpuRendering; }) * 1000 << "ms" << endl;

	// print frame times
	auto printValue =
		[](double v) -> void {
			cout << fixed;
			if(v < 1.)
				cout << setprecision(3) << v;
			else if(v < 10.)
				cout << setprecision(2) << v;
			else if(v < 100.)
				cout << setprecision(1) << v;
			else
				cout << setprecision(0) << v;
		};
	cout << "   Rendered " << frameInfoList.size() << " frames in " << fixed << setprecision(2) << chrono::duration<double>(realTestTime).count() << " seconds." << endl;
	if(printFrameTimes)
	{
		cout << "   Frame times:" << endl;
		for(FrameTimeInfo& i : frameTimeList) {
			cout << "      " << i.frameId << ": ";
			printValue(i.totalTime * 1000);
			cout << "ms (cpu time: ";
			printValue(i.cpuTime * 1000);
			cout << "ms, gpu time: ";
			printValue(i.gpuTime * 1000);
			cout << "ms)" << endl;
		}
	}
	else
		cout << "   To print times of each frame, specify --frame-times on command line." << endl;
}


int main(int argc, char** argv)
try {

	App app(argc, argv);
	app.init();
	app.mainLoop();
	app.printResults();

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
