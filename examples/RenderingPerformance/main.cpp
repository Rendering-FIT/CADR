#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/Pipeline.h>
#include <CadR/PrimitiveSet.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#include <CadR/VulkanLibrary.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <tuple>

using namespace std;
using namespace CadR;


// constants
static const vk::Extent2D imageExtent(128, 128);
static const string appName = "RenderingPerformance";
static const string vulkanAppName = "CadR performance test";
static const uint32_t vulkanAppVersion = VK_MAKE_VERSION(0, 0, 0);
static const string engineName = "CADR";
static const uint32_t engineVersion = VK_MAKE_VERSION(0, 0, 0);
static const uint32_t vulkanApiVersion = VK_API_VERSION_1_2;


enum class TestType
{
	TrianglePerformance,
	TriangleStripPerformance,
	BakedBoxesPerformance,
	BoxDrawablePerformance,
	DrawablePerformance
};


enum class RenderingSetup {
	Performance = 0,
	Picking = 1,
};
constexpr size_t RenderingSetupCount = 2;


// shader code in SPIR-V binary
static const uint32_t vsSpirv[]={
#include "shader.vert.spv"
};
static const uint32_t fsPerformanceSpirv[]={
#include "shader-performance.frag.spv"
};
static const uint32_t fsPickingSpirv[]={
#include "shader-picking.frag.spv"
};


// global application data
class App {
public:

	App(int argc, char* argv[]);
	~App();

	void init();
	void mainLoop();
	void frame(bool collectInfo);
	void printResults();
	tuple<Geometry*, vector<Drawable*>> generateInvisibleTriangleScene(
		StateSet& stateSet, glm::uvec2 screenSize, size_t requestedNumTriangles, TestType testType);

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
	vk::Queue graphicsQueue;
	Renderer renderer;
	array<vk::RenderPass, RenderingSetupCount> renderPassList;
	RenderingSetup renderingSetup = RenderingSetup::Picking;
	vk::Image colorImage;
	vk::Image depthImage;
	vk::Image idImage;
	vk::DeviceMemory colorImageMemory;
	vk::DeviceMemory depthImageMemory;
	vk::DeviceMemory idImageMemory;
	vk::ImageView colorImageView;
	vk::ImageView depthImageView;
	vk::ImageView idImageView;
	vk::Framebuffer framebuffer;
	vk::ShaderModule vsModule;
	vk::ShaderModule fsModule;
	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;
	vk::Fence renderingFinishedFence;

	TestType testType = TestType::TriangleStripPerformance;
	bool longTest = false;
	int deviceIndex = -1;
	string deviceNameFilter;
	size_t requestedNumTriangles = 12;
	bool calibratedTimestampsSupported = false;
	vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;
	vk::Format depthFormat = vk::Format::eUndefined;
	glm::vec4 backgroundColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
	list<FrameInfo> frameInfoList;
	chrono::steady_clock::duration realTestTime;
	uint32_t numContainers;
	vector<CadR::StateSetDrawableContainer*> id2containerMap;
	CadR::StateSet stateSetRoot;
	CadR::Pipeline pipeline;
	Geometry* geometry = nullptr;
	vector<Drawable*> drawableList;

};



tuple<Geometry*, vector<Drawable*>> App::generateInvisibleTriangleScene(
	StateSet& stateSet, glm::uvec2 screenSize, size_t requestedNumTriangles, TestType testType)
{
	if(screenSize.x <= 1 || screenSize.y <= 1)
		return {};

	// initialize variables
	// (final number of triangles stored in numTriangles is always equal or a little higher
	// than requestedNumTriangles, but at the end we create primitive set to render only
	// requestedNumTriangles)
	uint32_t numStrips = uint32_t(screenSize.x - 1 + screenSize.y - 1);
	size_t numTrianglesPerStrip = (requestedNumTriangles + numStrips - 1) / numStrips;
	if(testType == TestType::BakedBoxesPerformance || testType == TestType::BoxDrawablePerformance)
		numTrianglesPerStrip = ((numTrianglesPerStrip + 11) / 12) * 12;  // make it divisible by 12
	size_t numTriangles = numTrianglesPerStrip * numStrips;
	float triangleDistanceX = float(screenSize.x) / numTrianglesPerStrip;
	float triangleDistanceY = float(screenSize.y) / numTrianglesPerStrip;

	// alloc geometry
	size_t numVertices;
	switch(testType)
	{
		case TestType::TrianglePerformance:
		case TestType::DrawablePerformance:
			numVertices = numTriangles * 3;
			break;
		case TestType::TriangleStripPerformance:
			numVertices = numStrips * (numTrianglesPerStrip + 2);
			break;
		case TestType::BakedBoxesPerformance:
		case TestType::BoxDrawablePerformance:
			numVertices = numTriangles / 12 * 8;
			break;
		default:
			numVertices = 0;
	}
	size_t numPrimitiveSets;
	if (testType == TestType::TrianglePerformance || testType == TestType::TriangleStripPerformance || testType == TestType::BakedBoxesPerformance)
		numPrimitiveSets = 1;
	else if (testType == TestType::DrawablePerformance)
		numPrimitiveSets = requestedNumTriangles;
	else if (testType == TestType::BoxDrawablePerformance)
		numPrimitiveSets = (requestedNumTriangles + 11) / 12;
	unique_ptr<Geometry> geometry = make_unique<Geometry>(&renderer, AttribSizeList{ 12 }, numVertices, numTriangles*3, numPrimitiveSets);

	// generate indices
	StagingBuffer& indexStagingBuffer = geometry->createIndexStagingBuffer();
	uint32_t* indices = indexStagingBuffer.data<uint32_t>();
	if (testType == TestType::TrianglePerformance || testType == TestType::DrawablePerformance)
		for (uint32_t i = 0, c = uint32_t(numTriangles) * 3; i < c; i++)
			indices[i] = i;
	else if (testType == TestType::TriangleStripPerformance)
	{
		size_t i = 0;
		for (uint32_t s = 0; s < numStrips; s++)
		{
			uint32_t si = s * uint32_t(numTrianglesPerStrip + 2);
			for (uint32_t ti = 0; ti < uint32_t(numTrianglesPerStrip); ti++)
			{
				indices[i++] = si + ti;
				indices[i++] = si + ti + 1;
				indices[i++] = si + ti + 2;
			}
		}
	}
	else if (testType == TestType::BakedBoxesPerformance || testType == TestType::BoxDrawablePerformance)
	{
		for (uint32_t i = 0, c = uint32_t(numTriangles) * 3, j = 0; i < c;)
		{
			// -x face
			indices[i++] = j + 0;
			indices[i++] = j + 1;
			indices[i++] = j + 2;
			indices[i++] = j + 2;
			indices[i++] = j + 1;
			indices[i++] = j + 3;
			// -y face
			indices[i++] = j + 0;
			indices[i++] = j + 2;
			indices[i++] = j + 4;
			indices[i++] = j + 4;
			indices[i++] = j + 2;
			indices[i++] = j + 6;
			// +z face
			indices[i++] = j + 0;
			indices[i++] = j + 1;
			indices[i++] = j + 4;
			indices[i++] = j + 4;
			indices[i++] = j + 1;
			indices[i++] = j + 5;
			// +x face
			indices[i++] = j + 4;
			indices[i++] = j + 5;
			indices[i++] = j + 6;
			indices[i++] = j + 6;
			indices[i++] = j + 5;
			indices[i++] = j + 7;
			// +y face
			indices[i++] = j + 1;
			indices[i++] = j + 3;
			indices[i++] = j + 5;
			indices[i++] = j + 5;
			indices[i++] = j + 3;
			indices[i++] = j + 7;
			// -z face
			indices[i++] = j + 2;
			indices[i++] = j + 3;
			indices[i++] = j + 6;
			indices[i++] = j + 6;
			indices[i++] = j + 3;
			indices[i++] = j + 7;
			j += 8;
		}
	}

	// generate vertex positions,
	// first horizontal strips followed by vertical strips
	StagingBuffer& positionStagingBuffer = geometry->createVertexStagingBuffer(0);
	glm::vec3* positions = positionStagingBuffer.data<glm::vec3>();
	size_t i = 0;
	if (testType == TestType::TrianglePerformance || testType == TestType::DrawablePerformance)
	{
		for (unsigned y = 0, c = screenSize.y - 1; y < c; y++)
			for (size_t x = 0; x < numTrianglesPerStrip; x++)
			{
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.6f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.8f, 0.9f);
				positions[i++] = glm::vec3((float(x) + 0.5f) * triangleDistanceX, float(y) + 0.6f, 0.9f);
			}
		for (unsigned x = 0, c = screenSize.x - 1; x < c; x++)
			for (size_t y = 0; y < numTrianglesPerStrip; y++)
			{
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.6f, (float(y) + 0.5f) * triangleDistanceY, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY, 0.9f);
			}
	}
	else if (testType == TestType::TriangleStripPerformance)
	{
		for (unsigned y = 0, c = screenSize.y - 1; y < c; y++)
		{
			size_t x;
			for (x = 0; x <= numTrianglesPerStrip; x += 2)
			{
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.6f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.8f, 0.9f);
			}
			if (x == numTrianglesPerStrip + 1)
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.6f, 0.9f);
		}
		for (unsigned x = 0, c = screenSize.x - 1; x < c; x++)
		{
			size_t y;
			for (y = 0; y <= numTrianglesPerStrip; y += 2)
			{
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY, 0.9f);
			}
			if (y == numTrianglesPerStrip + 1)
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY, 0.9f);
		}
	}
	else if (testType == TestType::BakedBoxesPerformance || testType == TestType::BoxDrawablePerformance)
	{
		float s = triangleDistanceX * 10.f;  // boxes are spaced by 12 so make the size 10
		for (unsigned y = 0, c = screenSize.y - 1; y < c; y++)
		{
			size_t x;
			for (x = 0; x < numTrianglesPerStrip; x += 12)
			{
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.6f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.8f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.6f, 0.8f);
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.8f, 0.8f);
				positions[i++] = glm::vec3(x * triangleDistanceX + s, float(y) + 0.6f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX + s, float(y) + 0.8f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX + s, float(y) + 0.6f, 0.8f);
				positions[i++] = glm::vec3(x * triangleDistanceX + s, float(y) + 0.8f, 0.8f);
			}
		}
		s = triangleDistanceY * 10.f;  // boxes are spaced by 12 so make the size 10
		for (unsigned x = 0, c = screenSize.x - 1; x < c; x++)
		{
			size_t y;
			for (y = 0; y < numTrianglesPerStrip; y += 12)
			{
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY, 0.8f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY, 0.8f);
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY + s, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY + s, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY + s, 0.8f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY + s, 0.8f);
			}
		}
	}

	// generate primitiveSets
	StagingBuffer& primitiveSetStagingBuffer = geometry->createPrimitiveSetStagingBuffer();
	PrimitiveSetGpuData* primitiveSets = primitiveSetStagingBuffer.data<PrimitiveSetGpuData>();
	if (testType == TestType::TrianglePerformance || testType == TestType::TriangleStripPerformance || testType == TestType::BakedBoxesPerformance)
	{
		primitiveSets[0] = { uint32_t(requestedNumTriangles)*3, 0, 0, 0 };
	}
	else if (testType == TestType::DrawablePerformance)
	{
		for (size_t i = 0; i < requestedNumTriangles; i++)
			primitiveSets[i] = { 3, uint32_t(i) * 3, 0, 0 };
	}
	else if (testType == TestType::BoxDrawablePerformance)
	{
		size_t numBoxes = (requestedNumTriangles + 11) / 12;
		for (size_t i = 0; i < numBoxes; i++)
			primitiveSets[i] = { 36, uint32_t(i) * 36, 0, 0 };
	}

	// create drawable(s)
	struct DrawableList : vector<Drawable*> {
		~DrawableList() {
			for(Drawable* d : *this)
				delete d;
		}
	} drawableList;
	if (testType == TestType::TrianglePerformance || testType == TestType::TriangleStripPerformance || testType == TestType::BakedBoxesPerformance)
	{
		drawableList.emplace_back(make_unique<Drawable>(*geometry, 0, 0, 1, stateSet).release());
	}
	else if (testType == TestType::DrawablePerformance)
	{
		drawableList.reserve(requestedNumTriangles);
		for (uint32_t i=0, c=uint32_t(requestedNumTriangles); i<c; i++)
			drawableList.emplace_back(make_unique<Drawable>(*geometry, i, 0, 1, stateSet).release());
	}
	else if (testType == TestType::BoxDrawablePerformance)
	{
		uint32_t numBoxes = uint32_t((requestedNumTriangles + 11) / 12);
		drawableList.reserve(numBoxes);
		for (uint32_t i = 0; i < numBoxes; i++)
			drawableList.emplace_back(make_unique<Drawable>(*geometry, i, 0, 1, stateSet).release());
	}

	vector<Drawable*> dl;
	drawableList.swap(dl);
	return make_tuple(geometry.release(), dl);
}


/// Construct application object
App::App(int argc, char** argv)
	: stateSetRoot(&renderer)
	, pipeline(&renderer)
{
	// process command-line arguments
	bool printHelp = false;
	for(int i=1; i<argc; i++) {

		// parse options starting with '-'
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
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
	if(printHelp)
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
		
		// print usage and exit
		cout << "\nUsage:\n"
		        "   " << appName << " [-h] [gpu name filter] [gpu index]\n"
		        "   [gpu filter name] - optional string used to filter devices by their names\n"
		        "   [gpu index] - optional device index that will be used to select device\n"
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
		for(Drawable* d : drawableList)  delete d;
		delete geometry;

		// destroy handles
		// (the handles are destructed in certain (not arbitrary) order)
		pipeline.destroyPipeline();
		pipeline.destroyPipelineLayout();
		device.destroy(renderingFinishedFence);
		device.destroy(commandPool);
		device.destroy(fsModule);
		device.destroy(vsModule);
		device.destroy(framebuffer);
		device.destroy(colorImageView);
		device.destroy(depthImageView);
		device.destroy(idImageView);
		device.free(colorImageMemory);
		device.free(depthImageMemory);
		device.free(idImageMemory);
		device.destroy(colorImage);
		device.destroy(depthImage);
		device.destroy(idImage);
		for(auto r : renderPassList)  device.destroy(r);
	}
	// device, instance and library will be destroyed automatically in their destructors
}


void App::init()
{
	// header
	cout << appName << " tests various performance characteristics of CADR library.\n" << endl;

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
		nullptr  // enabled extensions
	);
	
	// select device
	tie(physicalDevice, graphicsQueueFamily, ignore) =
		instance.chooseDevice(vk::QueueFlagBits::eGraphics, nullptr, deviceNameFilter, deviceIndex);
	cout << "Tested device:" << endl;
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
	cout << "   " << instance.getPhysicalDeviceProperties(physicalDevice).deviceName << "\n" << endl;

	// create device
	device.create(
		instance,  // instance
		physicalDevice,
		graphicsQueueFamily,
		graphicsQueueFamily,
		nullptr,  // extensions
		Renderer::requiredFeatures()  // features
	);
	graphicsQueue = device.getQueue(graphicsQueueFamily, 0);
	renderer.init(device, instance, physicalDevice, graphicsQueueFamily);

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
								vk::ImageLayout::eColorAttachmentOptimal  // finalLayout
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
								vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput),  // srcStageMask
								vk::PipelineStageFlags(vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eVertexInput |  // dstStageMask
									vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eEarlyFragmentTests |
									vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eLateFragmentTests |
									vk::PipelineStageFlagBits::eColorAttachmentOutput),
								vk::AccessFlags(),     // srcAccessMask
								vk::AccessFlags(vk::AccessFlagBits::eIndirectCommandRead | vk::AccessFlagBits::eIndexRead |  // dstAccessMask
									vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eShaderRead |
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

	// framebuffer images
	colorImage =
		device.createImage(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),       // flags
				vk::ImageType::e2D,           // imageType
				colorFormat,                  // format
				vk::Extent3D(imageExtent.width, imageExtent.height, 1),  // extent
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
	depthImage =
		device.createImage(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				depthFormat,             // format
				vk::Extent3D(imageExtent.width, imageExtent.height, 1),  // extent
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
	idImage =
		device.createImage(
			vk::ImageCreateInfo(
				vk::ImageCreateFlags(),  // flags
				vk::ImageType::e2D,      // imageType
				vk::Format::eR32G32B32A32Uint,  // format, support of eR32G32B32A32Uint format is mandatory in Vulkan
				vk::Extent3D(imageExtent.width, imageExtent.height, 1),  // extent
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

	// memory for image
	auto allocateMemory =
		[](App& app, vk::Image image, vk::MemoryPropertyFlags requiredFlags) -> vk::DeviceMemory{
			vk::MemoryRequirements memoryRequirements = app.device.getImageMemoryRequirements(image);
			vk::PhysicalDeviceMemoryProperties memoryProperties = app.instance.getPhysicalDeviceMemoryProperties(app.physicalDevice);
			for(uint32_t i=0; i<memoryProperties.memoryTypeCount; i++)
				if(memoryRequirements.memoryTypeBits & (1<<i))
					if((memoryProperties.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags)
						return
							app.device.allocateMemory(
								vk::MemoryAllocateInfo(
									memoryRequirements.size,  // allocationSize
									i                         // memoryTypeIndex
								)
							);
			throw std::runtime_error("No suitable memory type found for image.");
		};
	colorImageMemory = allocateMemory(*this, colorImage, vk::MemoryPropertyFlagBits::eDeviceLocal);
	depthImageMemory = allocateMemory(*this, depthImage, vk::MemoryPropertyFlagBits::eDeviceLocal);
	idImageMemory    = allocateMemory(*this, idImage,    vk::MemoryPropertyFlagBits::eDeviceLocal);
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

	// image view
	colorImageView =
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
				idImage  ,                   // image
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
	framebuffer =
		device.createFramebuffer(
			vk::FramebufferCreateInfo(
				vk::FramebufferCreateFlags(),  // flags
				renderPassList[uint32_t(renderingSetup)],  // renderPass
				(renderingSetup == RenderingSetup::Picking) ? 3 : 2,  // attachmentCount
				array<vk::ImageView, 3>{  // pAttachments
					colorImageView,
					depthImageView,
					idImageView,
				}.data(),
				imageExtent.width,  // width
				imageExtent.height,  // height
				1  // layers
			)
		);

	// create shader modules
	vsModule =
		device.createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				sizeof(vsSpirv),  // codeSize
				vsSpirv  // pCode
			)
		);
	fsModule =
		device.createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),  // flags
				(renderingSetup == RenderingSetup::Picking) ? sizeof(fsPickingSpirv) : sizeof(fsPerformanceSpirv),  // codeSize
				(renderingSetup == RenderingSetup::Picking) ? fsPickingSpirv : fsPerformanceSpirv // pCode
			)
		);

	// pipeline layout
	auto pipelineLayoutUnique =
		device.createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo{
				vk::PipelineLayoutCreateFlags(),  // flags
				0,       // setLayoutCount
				nullptr, // pSetLayouts
				1,       // pushConstantRangeCount
				&(const vk::PushConstantRange&)vk::PushConstantRange{  // pPushConstantRanges
					vk::ShaderStageFlagBits::eVertex,  // stageFlags
					0,  // offset
					2*16*sizeof(float)  // size
				}
			}
		);

	// pipeline
	auto pipelineUnique =
		device.createGraphicsPipelineUnique(
			nullptr,  // pipelineCache
			vk::GraphicsPipelineCreateInfo(
				vk::PipelineCreateFlags(),  // flags

				// shader stages
				2,  // stageCount
				array{  // pStages
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),  // flags
						vk::ShaderStageFlagBits::eVertex,      // stage
						vsModule,  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
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
				&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{  // pVertexInputState
					vk::PipelineVertexInputStateCreateFlags(),  // flags
					1,  // vertexBindingDescriptionCount
					array{  // pVertexBindingDescriptions
						vk::VertexInputBindingDescription(
							0, 12, vk::VertexInputRate::eVertex
						),
					}.data(),
					1,  // vertexAttributeDescriptionCount
					array{  // pVertexAttributeDescriptions
						vk::VertexInputAttributeDescription(
							0, 0, vk::Format::eR32G32B32Sfloat, 0
						),
					}.data()
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
						vk::Viewport(0.f, 0.f, float(imageExtent.width), float(imageExtent.height), 0.f, 1.f),
					}.data(),
					1,  // scissorCount
					array{  // pScissors
						vk::Rect2D(vk::Offset2D(0,0), imageExtent)
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
				pipelineLayoutUnique.get(),  // layout
				renderPassList[uint32_t(renderingSetup)],  // renderPass
				0,  // subpass
				vk::Pipeline(nullptr),  // basePipelineHandle
				-1 // basePipelineIndex
			)
		);
	pipeline.init(pipelineUnique.release(), pipelineLayoutUnique.release(), nullptr);

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

	// fence
	renderingFinishedFence =
		device.createFence(
			vk::FenceCreateInfo{
				vk::FenceCreateFlags()  // flags
			}
		);

	// scene
	stateSetRoot.pipeline = &pipeline;
	tie(geometry, drawableList) = generateInvisibleTriangleScene(
		stateSetRoot, glm::vec2(imageExtent.width, imageExtent.height), requestedNumTriangles, testType);
}


void App::frame(bool collectInfo)
{
	// construct camera matrices
	glm::mat4 matrices[2];
	matrices[0] =
		glm::orthoLH_ZO(
			0.f,  // left
			float(imageExtent.width),  // right
			float(imageExtent.height),  // bottom
			0.f,  // top
			0.5f,  // near
			100.f  // far
		);
	matrices[1] = glm::mat4(1);

	// begin the frame
	renderer.setCollectFrameInfo(collectInfo, calibratedTimestampsSupported);
	size_t frameNumber = renderer.beginFrame();

	// submit all copy operations that were not submitted yet
	renderer.executeCopyOperations();

	// begin command buffer recording
	renderer.beginRecording(commandBuffer);

	// record camera matrices
	device.cmdPushConstants(
		commandBuffer,  // commandBuffer
		pipeline.layout(),  // pipelineLayout
		vk::ShaderStageFlagBits::eVertex,  // stageFlags
		0,  // offset
		(16+4)*sizeof(float),  // size
		&matrices  // pValues
	);

	// prepare recording
	numContainers = 1;  // value zero reserved for no container
	size_t numDrawables = renderer.prepareSceneRendering(stateSetRoot);
	id2containerMap.clear();
	if(id2containerMap.capacity() < numContainers)
		id2containerMap.reserve(numContainers+(numContainers>>4));  // make the capacity 1/16 bigger to avoid reallocations on slowly growing scenes
	else if(id2containerMap.capacity() >= 2*numContainers) {
		auto stateSetDrawableContainers = decltype(id2containerMap){};
		id2containerMap.swap(stateSetDrawableContainers);
		id2containerMap.reserve(numContainers+(numContainers>>4));  // make the capacity 1/16 bigger to avoid reallocations on slowly growing scenes
	}
	id2containerMap.emplace_back(nullptr);  // no container for value zero

	// record scene rendering
	renderer.recordDrawableProcessing(commandBuffer, numDrawables);
	renderer.recordSceneRendering(
		commandBuffer,  // commandBuffer
		stateSetRoot,  // stateSetRoot
		renderPassList[uint32_t(renderingSetup)],  // renderPass
		framebuffer,  // framebuffer
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

	// render
	device.queueSubmit(
		graphicsQueue,  // queue
		vk::SubmitInfo(
			0, nullptr, nullptr,  // waitSemaphoreCount, pWaitSemaphores, pWaitDstStageMask
			1, &commandBuffer,    // commandBufferCount+pCommandBuffers
			0, nullptr            // signalSemaphoreCount, pSignalSemaphores
		),
		renderingFinishedFence  // fence
	);

	// mark the end of the frame
	renderer.endFrame();

	// wait for the work
	vk::Result r = device.waitForFences(
		renderingFinishedFence,  // fences (vk::ArrayProxy)
		VK_TRUE,       // waitAll
		uint64_t(3e9)  // timeout (3s)
	);
	if(r == vk::Result::eTimeout)
		throw std::runtime_error("GPU timeout. Task is probably hanging.");
	device.resetFences(renderingFinishedFence);
}


void App::mainLoop()
{
	chrono::steady_clock::duration testTime = longTest ? chrono::seconds(30) : chrono::milliseconds(1500);
	chrono::steady_clock::time_point startTime = chrono::steady_clock::now();
	chrono::steady_clock::time_point finishTime = startTime + testTime;
	chrono::steady_clock::time_point t;

	do{
		frame(true);
		frameInfoList.splice(frameInfoList.end(), renderer.getFrameInfos());
		t = chrono::steady_clock::now();
	}while(t < finishTime);
	realTestTime = t - startTime;

	device.waitIdle();
	frameInfoList.splice(frameInfoList.end(), renderer.getFrameInfos());
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
	else if (testType == TestType::BakedBoxesPerformance)
		cout << "Baked boxes performance test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
				<< "M (" << size_t(requestedNumTriangles + 11) / 12 << ") boxes ("
				<< requestedNumTriangles << " triangles in 1 drawable)." << endl;
	else if (testType == TestType::BoxDrawablePerformance)
		cout << "Drawable per box performance test of " << size_t((requestedNumTriangles + 11) / 12 / 1e6)
				<< "M (" << size_t((requestedNumTriangles + 11) / 12) << ") boxes ("
				<< requestedNumTriangles << " triangles in " << size_t((requestedNumTriangles + 11) / 12)
				<< " drawables)." << endl;
	else if (testType == TestType::DrawablePerformance)
		cout << "Drawable performance test of " << size_t(requestedNumTriangles / 1e6)
				<< " Mtri in " << size_t(requestedNumTriangles / 1e6) << " Mdrawables ("
				<< requestedNumTriangles << " triangles in " << requestedNumTriangles << " drawables)." << endl;

	// if no measurements
	if(frameInfoList.empty()) {
		cout << "      No measurements made." << endl;
		return;
	}

	// get time info
	struct FrameTimeInfo
	{
		size_t frameId;
		double cpuTime;
		double gpuTime;
		double totalTime;
	};
	vector<FrameTimeInfo> frameTimeList;
	frameTimeList.reserve(frameInfoList.size());
	for(FrameInfo& info : frameInfoList) {
		double cpuTime = double(info.endFrameCpu - info.beginFrameCpu) * renderer.cpuTimestampPeriod();
		double gpuTime = double(info.endRenderingGpu - info.beginRenderingGpu) * renderer.gpuTimestampPeriod();
		double totalTime = max(double(info.endRenderingGpu - info.beginFrameGpu) * renderer.gpuTimestampPeriod(),
		                       cpuTime);
		frameTimeList.emplace_back(FrameTimeInfo{info.frameNumber, cpuTime, gpuTime, totalTime});
	}

	// print median times
	cout << "   Median times:" << endl;
	auto median = [](vector<FrameTimeInfo>& l, double (*getElement)(FrameTimeInfo&)) -> double {
		vector<double> v;
		v.reserve(l.size());
		for (FrameTimeInfo& i : l)
			v.push_back(getElement(i));
		std::nth_element(v.begin(), v.begin() + l.size() / 2, v.end());
		return v[l.size() / 2];
	};
	cout << "      Performance: " << size_t(double(requestedNumTriangles) / median(frameTimeList, [](FrameTimeInfo& i) { return i.totalTime; }) / 1e6) << " Mtri/s" << endl;
	cout << fixed << setprecision(3);
	cout << "      Total time:  " << median(frameTimeList, [](FrameTimeInfo& i) { return i.totalTime; }) * 1000 << "ms" << endl;
	cout << "      Cpu time:    " << median(frameTimeList, [](FrameTimeInfo& i) { return i.cpuTime; }) * 1000 << "ms" << endl;
	cout << "      Gpu time:    " << median(frameTimeList, [](FrameTimeInfo& i) { return i.gpuTime; }) * 1000 << "ms" << endl;

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
	cout << "   Test frames rendered: " << frameInfoList.size() << " in " << fixed << setprecision(2) << chrono::duration<double>(realTestTime).count() << " seconds." << endl;
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


int main(int argc, char** argv) {

	try {

		{ App ax(argc, argv); }
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

	return 0;
}
