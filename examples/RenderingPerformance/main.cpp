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
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.hpp>
#include <iomanip>
#include <iostream>
#include <tuple>

using namespace std;
using namespace CadR;


// constants
static const vk::Extent2D imageExtent(1920, 1080);
static const constexpr auto shortTestDuration = chrono::milliseconds(2000);
static const constexpr auto longTestDuration = chrono::seconds(20);
static const string appName = "RenderingPerformance";
static const string vulkanAppName = "CadR performance test";
static const uint32_t vulkanAppVersion = VK_MAKE_VERSION(0, 0, 0);
static const string engineName = "CADR";
static const uint32_t engineVersion = VK_MAKE_VERSION(0, 0, 0);
static const uint32_t vulkanApiVersion = VK_API_VERSION_1_2;


enum class TestType
{
	Undefined,
	TrianglePerformance,
	TriangleStripPerformance,
	DrawablePerformance,
	PrimitiveSetPerformance,
	BakedBoxesPerformance,
	BakedBoxesScene,
	InstancedBoxesPerformance,
	InstancedBoxesScene,
	IndependentBoxesPerformance,
	IndependentBoxesScene,
};


enum class RenderingSetup {
	Performance = 0,
	Picking = 1,
};
constexpr size_t RenderingSetupCount = 2;


struct SceneGpuData {
	glm::mat4 viewMatrix;   // current camera view matrix
	float p11,p22,p33,p43;  // projectionMatrix - members that depend on zNear and zFar clipping planes
};


struct PrimitiveSetGpuData {
	uint32_t count;
	uint32_t first;
};


struct ShaderData {
	glm::vec3 ambient;
	uint32_t materialType;
	glm::vec3 diffuse;
	float alpha;
	glm::vec3 specular;
	float shininess;
	glm::vec3 emission;
	float pointSize;
};
static_assert(sizeof(ShaderData) == 64, "Size of ShaderData is not 64.");


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
	void mainLoop();
	void frame(bool collectInfo);
	void printResults();
	void generateInvisibleTriangleScene(glm::uvec2 screenSize,
		size_t requestedNumTriangles, TestType testType);
	void generateBoxesScene(glm::uvec3 numBoxes, glm::vec3 center, glm::vec3 boxToBoxDistance,
		glm::vec3 boxSize, bool instanced, bool singleGeometry);
	void generateBakedBoxesScene(glm::uvec3 numBoxes,
		glm::vec3 center, glm::vec3 boxToBoxDistance, glm::vec3 boxSize);
	void deleteScene();

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
	RenderingSetup renderingSetup = RenderingSetup::Performance;
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

	TestType testType = TestType::Undefined;
	bool longTest = false;
	bool printFrameTimes = false;
	int deviceIndex = -1;
	string deviceNameFilter;
	size_t requestedNumTriangles = 0;
	bool calibratedTimestampsSupported = false;
	vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;
	vk::Format depthFormat = vk::Format::eUndefined;
	glm::vec4 backgroundColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
	list<FrameInfo> frameInfoList;
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
	vector<Geometry*> geometryList;
	vector<Drawable> drawableList;

};



void App::deleteScene()
{
	//for(Drawable* d : drawableList)  delete d;
	for(Geometry* g : geometryList)  delete g;
	drawableList.clear();
	geometryList.clear();
}


void App::generateInvisibleTriangleScene(glm::uvec2 screenSize, size_t requestedNumTriangles, TestType testType)
{
	deleteScene();

	if(screenSize.x <= 1 || screenSize.y <= 1)
		return;

	// initialize variables
	// (final number of triangles stored in numTriangles is always equal or a little higher
	// than requestedNumTriangles, but at the end we create primitive set to render only
	// requestedNumTriangles)
	uint32_t numSeries = uint32_t(screenSize.x - 1 + screenSize.y - 1);
	size_t numTrianglesInSeries = (requestedNumTriangles + numSeries - 1) / numSeries;
	if(testType == TestType::BakedBoxesPerformance || testType == TestType::IndependentBoxesPerformance ||
	   testType == TestType::InstancedBoxesPerformance)
	{
		numTrianglesInSeries = ((numTrianglesInSeries + 11) / 12) * 12;  // make it divisible by 12
	}
	size_t numTriangles = numTrianglesInSeries * numSeries;
	float triangleDistanceX = float(screenSize.x) / numTrianglesInSeries;
	float triangleDistanceY = float(screenSize.y) / numTrianglesInSeries;

	// create geometry
	geometryList.reserve(1);
	geometryList.emplace_back(new Geometry(renderer));
	Geometry* geometry = geometryList.back();

	// numVertices
	size_t numVertices;
	switch (testType)
	{
		case TestType::TrianglePerformance:
		case TestType::PrimitiveSetPerformance:
			numVertices = numTriangles * 3;
			break;
		case TestType::TriangleStripPerformance:
			numVertices = numSeries * (numTrianglesInSeries + 2);
			break;
		case TestType::DrawablePerformance:
			numVertices = 3;
			break;
		case TestType::BakedBoxesPerformance:
		case TestType::IndependentBoxesPerformance:
			numVertices = numTriangles / 12 * 8;
			break;
		case TestType::InstancedBoxesPerformance:
			numVertices = 8;
			break;
		default:
			numVertices = 0;
	}

	// generate vertex positions,
	// first horizontal strips followed by vertical strips
	StagingData positionStagingData = geometry->createVertexStagingData(numVertices * sizeof(glm::vec3));
	glm::vec3* positions = positionStagingData.data<glm::vec3>();
	size_t i = 0;
	if (testType == TestType::TrianglePerformance || testType == TestType::PrimitiveSetPerformance)
	{
		for (unsigned y = 0, c = screenSize.y - 1; y < c; y++)
			for (size_t x = 0; x < numTrianglesInSeries; x++)
			{
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.6f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.8f, 0.9f);
				positions[i++] = glm::vec3((float(x) + 0.5f) * triangleDistanceX, float(y) + 0.6f, 0.9f);
			}
		for (unsigned x = 0, c = screenSize.x - 1; x < c; x++)
			for (size_t y = 0; y < numTrianglesInSeries; y++)
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
			for (x = 0; x <= numTrianglesInSeries; x += 2)
			{
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.6f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.8f, 0.9f);
			}
			if (x == numTrianglesInSeries + 1)
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.6f, 0.9f);
		}
		for (unsigned x = 0, c = screenSize.x - 1; x < c; x++)
		{
			size_t y;
			for (y = 0; y <= numTrianglesInSeries; y += 2)
			{
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY, 0.9f);
			}
			if (y == numTrianglesInSeries + 1)
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY, 0.9f);
		}
	}
	else if (testType == TestType::DrawablePerformance)
	{
		positions[i++] = glm::vec3(0.f,  0.6f, 0.9f);
		positions[i++] = glm::vec3(0.f,  0.8f, 0.9f);
		positions[i++] = glm::vec3(0.2f, 0.6f, 0.9f);
	}
	else if (testType == TestType::BakedBoxesPerformance || testType == TestType::IndependentBoxesPerformance)
	{
		// we generate boxes in horizontal and vertical strips
		// (numTrianglesPerStrip divided by 12 gives number of boxes in the strip)
		for (unsigned y = 0, c = screenSize.y - 1; y < c; y++)
		{
			size_t x;
			for (x = 0; x < numTrianglesInSeries; x += 12)
			{
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.6f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.8f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.6f, 0.8f);
				positions[i++] = glm::vec3(x * triangleDistanceX, float(y) + 0.8f, 0.8f);
				positions[i++] = glm::vec3(x * triangleDistanceX + 0.2f, float(y) + 0.6f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX + 0.2f, float(y) + 0.8f, 0.9f);
				positions[i++] = glm::vec3(x * triangleDistanceX + 0.2f, float(y) + 0.6f, 0.8f);
				positions[i++] = glm::vec3(x * triangleDistanceX + 0.2f, float(y) + 0.8f, 0.8f);
			}
		}
		for (unsigned x = 0, c = screenSize.x - 1; x < c; x++)
		{
			size_t y;
			for (y = 0; y < numTrianglesInSeries; y += 12)
			{
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY, 0.8f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY, 0.8f);
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY + 0.2f, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY + 0.2f, 0.9f);
				positions[i++] = glm::vec3(float(x) + 0.6f, y * triangleDistanceY + 0.2f, 0.8f);
				positions[i++] = glm::vec3(float(x) + 0.8f, y * triangleDistanceY + 0.2f, 0.8f);
			}
		}
	}
	else if (testType == TestType::InstancedBoxesPerformance)
	{
		// generate single box
		positions[i++] = glm::vec3(0.f,  0.6f, 0.9f);
		positions[i++] = glm::vec3(0.f,  0.8f, 0.9f);
		positions[i++] = glm::vec3(0.f,  0.6f, 0.8f);
		positions[i++] = glm::vec3(0.f,  0.8f, 0.8f);
		positions[i++] = glm::vec3(0.2f, 0.6f, 0.9f);
		positions[i++] = glm::vec3(0.2f, 0.8f, 0.9f);
		positions[i++] = glm::vec3(0.2f, 0.6f, 0.8f);
		positions[i++] = glm::vec3(0.2f, 0.8f, 0.8f);
	}

	// generate indices
	size_t numIndices;
	switch (testType) {
	case TestType::DrawablePerformance: numIndices = 3; break;
	case TestType::InstancedBoxesPerformance: numIndices = 36; break;
	default: numIndices = numTriangles * 3; break;
	}
	StagingData indexStagingData = geometry->createIndexStagingData(numIndices * sizeof(uint32_t));
	uint32_t* indices = indexStagingData.data<uint32_t>();
	if (testType == TestType::TrianglePerformance || testType == TestType::PrimitiveSetPerformance ||
		testType == TestType::DrawablePerformance)
	{
		for (uint32_t i = 0, c = uint32_t(numIndices); i < c; i++)
			indices[i] = i;
	}
	else if (testType == TestType::TriangleStripPerformance)
	{
		size_t i = 0;
		for (uint32_t s = 0; s < numSeries; s++)
		{
			uint32_t si = s * uint32_t(numTrianglesInSeries + 2);
			for (uint32_t ti = 0; ti < uint32_t(numTrianglesInSeries); ti++)
			{
				indices[i++] = si + ti;
				indices[i++] = si + ti + 1;
				indices[i++] = si + ti + 2;
			}
		}
	}
	else if (testType == TestType::BakedBoxesPerformance || testType == TestType::IndependentBoxesPerformance ||
	         testType == TestType::InstancedBoxesPerformance)
	{
		for (uint32_t i = 0, c = uint32_t(numIndices), j = 0; i < c;)
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

	// numPrimitiveSets
	size_t numPrimitiveSets;
	if (testType == TestType::TrianglePerformance || testType == TestType::TriangleStripPerformance ||
	    testType == TestType::DrawablePerformance || testType == TestType::BakedBoxesPerformance ||
	    testType == TestType::InstancedBoxesPerformance)
	{
		numPrimitiveSets = 1;
	}
	else if (testType == TestType::PrimitiveSetPerformance)
		numPrimitiveSets = requestedNumTriangles;  // only requestedNumTriangles is rendered, although numTriangles were generated
	else if (testType == TestType::IndependentBoxesPerformance)
		numPrimitiveSets = (requestedNumTriangles + 11) / 12;

	// generate primitiveSets
	StagingData primitiveSetStagingData = geometry->createPrimitiveSetStagingData(numPrimitiveSets * sizeof(PrimitiveSetGpuData));
	PrimitiveSetGpuData* primitiveSets = primitiveSetStagingData.data<PrimitiveSetGpuData>();
	if (testType == TestType::TrianglePerformance || testType == TestType::TriangleStripPerformance ||
	    testType == TestType::BakedBoxesPerformance)
	{
		primitiveSets[0] = { uint32_t(requestedNumTriangles)*3, 0 };  // only requestedNumTriangles is rendered, although numTriangles were generated
	}
	else if (testType == TestType::DrawablePerformance)
	{
		primitiveSets[0] = { 3, 0 };
	}
	else if (testType == TestType::PrimitiveSetPerformance)
	{
		for (size_t i = 0; i < numPrimitiveSets; i++)
			primitiveSets[i] = { 3, uint32_t(i) * 3 };
	}
	else if (testType == TestType::IndependentBoxesPerformance)
	{
		for (size_t i = 0; i < numPrimitiveSets; i++)
			primitiveSets[i] = { 36, uint32_t(i) * 36 };
	}
	else if (testType == TestType::InstancedBoxesPerformance)
	{
		primitiveSets[0] = { 36, 0 };
	}


	// create drawable(s)
	StagingData shaderStagingData;
	if (testType == TestType::TrianglePerformance || testType == TestType::TriangleStripPerformance ||
	    testType == TestType::BakedBoxesPerformance)
	{
		drawableList.reserve(1);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak
		drawableList.emplace_back(*geometry, 0, shaderStagingData, 128, 1, stateSetRoot);

		uint8_t* b = shaderStagingData.data<uint8_t>();
		ShaderData* d = reinterpret_cast<ShaderData*>(b);
		memset(d, 0, sizeof(ShaderData));
		d->diffuse = glm::vec3(1.f, 1.f, 0.f);
		d->alpha = 1.f;
		constexpr const glm::mat4 identityMatrix(1.f);
		memcpy(b+64, &identityMatrix, 64);
	}
	else if (testType == TestType::DrawablePerformance)
	{
		drawableList.reserve(requestedNumTriangles);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak
		for (unsigned y = 0, c = screenSize.y - 1; y < c; y++)
		{
			size_t x;
			for (x = 0; x < numTrianglesInSeries; x++)
			{
				drawableList.emplace_back(*geometry, 0, shaderStagingData, 128, 1, stateSetRoot);

				uint8_t* b = shaderStagingData.data<uint8_t>();
				ShaderData* d = reinterpret_cast<ShaderData*>(b);
				memset(d, 0, sizeof(ShaderData));
				d->diffuse = glm::vec3(1.f, 1.f, 0.f);
				d->alpha = 1.f;
				*reinterpret_cast<glm::mat4*>(b+64) = glm::translate(glm::mat4(1.f),
					glm::vec3(x * triangleDistanceX, y, 0.f));

				if(drawableList.size() == requestedNumTriangles)
					return;
			}
		}
		for (unsigned x = 0, c = screenSize.x - 1; x < c; x++)
		{
			size_t y;
			for (y = 0; y < numTrianglesInSeries; y++)
			{
				drawableList.emplace_back(*geometry, 0, shaderStagingData, 128, 1, stateSetRoot);

				uint8_t* b = shaderStagingData.data<uint8_t>();
				ShaderData* d = reinterpret_cast<ShaderData*>(b);
				memset(d, 0, sizeof(ShaderData));
				d->diffuse = glm::vec3(1.f, 1.f, 0.f);
				d->alpha = 1.f;
				*reinterpret_cast<glm::mat4*>(b+64) = glm::translate(glm::mat4(1.f),
					glm::vec3(float(x) + 0.6f, y * triangleDistanceY - 0.6f, 0.f));

				if(drawableList.size() == requestedNumTriangles)
					return;
			}
		}
	}
	else if (testType == TestType::PrimitiveSetPerformance)
	{
		drawableList.reserve(requestedNumTriangles);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak
		for (uint32_t offset=0, e=uint32_t(requestedNumTriangles)*sizeof(PrimitiveSetGpuData); offset<e; offset+=sizeof(PrimitiveSetGpuData))
		{
			drawableList.emplace_back(*geometry, offset, shaderStagingData, 128, 1, stateSetRoot);

			uint8_t* b = shaderStagingData.data<uint8_t>();
			ShaderData* d = reinterpret_cast<ShaderData*>(b);
			memset(d, 0, sizeof(ShaderData));
			d->diffuse = glm::vec3(1.f, 1.f, 0.f);
			d->alpha = 1.f;
			constexpr const glm::mat4 identityMatrix(1.f);
			memcpy(b+64, &identityMatrix, 64);
		}
	}
	else if (testType == TestType::InstancedBoxesPerformance)
	{
		size_t numInstances = numTriangles / 12;
		drawableList.reserve(1);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak
		drawableList.emplace_back(*geometry, 0, shaderStagingData, 64+(64*numInstances), uint32_t(numInstances), stateSetRoot);

		uint8_t* b = shaderStagingData.data<uint8_t>();
		ShaderData* d = reinterpret_cast<ShaderData*>(b);
		memset(d, 0, sizeof(ShaderData));
		d->diffuse = glm::vec3(1.f, 1.f, 0.f);
		d->alpha = 1.f;
		b += 64;
		for (unsigned y = 0, c = screenSize.y - 1; y < c; y++)
		{
			size_t x;
			for (x = 0; x < numTrianglesInSeries; x += 12)
			{
				*reinterpret_cast<glm::mat4*>(b) = glm::translate(glm::mat4(1.f),
					glm::vec3(x * triangleDistanceX, y, 0.f));
				b += 64;
			}
		}
		for (unsigned x = 0, c = screenSize.x - 1; x < c; x++)
		{
			size_t y;
			for (y = 0; y < numTrianglesInSeries; y += 12)
			{
				*reinterpret_cast<glm::mat4*>(b) = glm::translate(glm::mat4(1.f),
					glm::vec3(float(x) + 0.6f, y * triangleDistanceY - 0.6f, 0.f));
				b += 64;
			}
		}
	}
	else if (testType == TestType::IndependentBoxesPerformance)
	{
		drawableList.reserve(numPrimitiveSets);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak
		for (uint32_t offset=0, e=uint32_t(numPrimitiveSets)*sizeof(PrimitiveSetGpuData); offset<e; offset+=sizeof(PrimitiveSetGpuData))
		{
			drawableList.emplace_back(*geometry, offset, shaderStagingData, 128, 1, stateSetRoot);

			uint8_t* b = shaderStagingData.data<uint8_t>();
			ShaderData* d = reinterpret_cast<ShaderData*>(b);
			memset(d, 0, sizeof(ShaderData));
			d->diffuse = glm::vec3(1.f, 1.f, 0.f);
			d->alpha = 1.f;
			constexpr const glm::mat4 identityMatrix(1.f);
			memcpy(b+64, &identityMatrix, 64);
		}
	}
}


static const uint32_t boxIndices[36] =
{
	0, 2, 1,  1, 2, 3,  // face x,y
	0, 1, 4,  4, 1, 5,  // face x,z
	0, 4, 2,  2, 4, 6,  // face y,z
	4, 5, 6,  6, 5, 7,  // face x,y
	2, 6, 3,  3, 6, 7,  // face x,z
	1, 3, 5,  5, 3, 7,  // face y,z
};


void App::generateBoxesScene(
	glm::uvec3 numBoxes,
	glm::vec3 center,
	glm::vec3 boxToBoxDistance,
	glm::vec3 boxSize,
	bool instanced,
	bool singleGeometry)
{
	deleteScene();

	// make sure valid parameters were passed in
	if(instanced == true && singleGeometry == false)
		throw runtime_error("RenderingPerformance::generateBoxScene(): Invalid function arguments. "
		                    "If parameter instanced is true, singleGeometry parameter must be true as well.");
	if(numBoxes.x == 0 || numBoxes.y == 0 || numBoxes.z == 0)
		return;

	// generate Geometries
	size_t boxesCount = numBoxes.x * numBoxes.y * numBoxes.z;
	size_t numGeometries = (singleGeometry) ? 1 : boxesCount;
	size_t geometryIndex = geometryList.size();
	geometryList.reserve(numGeometries);
	for(size_t i=0; i<numGeometries; i++) {

		// create Geometry
		geometryList.emplace_back(new Geometry(renderer));
		Geometry* geometry = geometryList.back();

		// alloc positions
		StagingData positionStagingData = geometry->createVertexStagingData(8 * sizeof(glm::vec3));
		glm::vec3* positions = positionStagingData.data<glm::vec3>();

		// generate vertex positions
		glm::vec3 d = boxSize / 2.f;
		positions[0] = glm::vec3(-d.x, -d.y, -d.z);
		positions[1] = glm::vec3( d.x, -d.y, -d.z);
		positions[2] = glm::vec3(-d.x,  d.y, -d.z);
		positions[3] = glm::vec3( d.x,  d.y, -d.z);
		positions[4] = glm::vec3(-d.x, -d.y,  d.z);
		positions[5] = glm::vec3( d.x, -d.y,  d.z);
		positions[6] = glm::vec3(-d.x,  d.y,  d.z);
		positions[7] = glm::vec3( d.x,  d.y,  d.z);

		// indices
		StagingData indexStagingData = geometry->createIndexStagingData(sizeof(boxIndices));
		uint32_t* indices = indexStagingData.data<uint32_t>();
		memcpy(indices, boxIndices, sizeof(boxIndices));

		// primitiveSet
		StagingData primitiveSetStagingData = geometry->createPrimitiveSetStagingData(sizeof(PrimitiveSetGpuData));
		PrimitiveSetGpuData* primitiveSet = primitiveSetStagingData.data<PrimitiveSetGpuData>();
		primitiveSet->count = 36;
		primitiveSet->first = 0;
	}

	// create Drawables
	StagingData shaderStagingData;
	if(instanced)
	{
		drawableList.reserve(1);  // this ensures that no exception is thrown during emplacing which would result in memory leak
		drawableList.emplace_back(*geometryList.front(), 0, shaderStagingData, 64+(64*boxesCount), uint32_t(boxesCount), stateSetRoot);

		// shader staging data
		uint8_t* b = shaderStagingData.data<uint8_t>();
		ShaderData* d = reinterpret_cast<ShaderData*>(b);
		memset(d, 0, sizeof(ShaderData));
		d->diffuse = glm::vec3(1.f, 0.5f, 1.f);
		d->alpha = 1.f;
		b += 64;

		// generate transformations
		glm::vec3 origin = center - (boxToBoxDistance * glm::vec3(numBoxes.x-1, numBoxes.y-1, numBoxes.z-1) / 2.f);
		for(uint32_t k=0; k<numBoxes.z; k++) {
			auto planeZ = origin.z + (k * boxToBoxDistance.z);
			for(uint32_t j=0; j<numBoxes.y; j++) {
				auto lineY = origin.y + (j * boxToBoxDistance.y);
				for(uint32_t i=0; i<numBoxes.x; i++)
				{
					*reinterpret_cast<glm::mat4*>(b) = glm::translate(glm::mat4(1.f),
						glm::vec3(origin.x + (i*boxToBoxDistance.x), lineY, planeZ));
					b += 64;
				}
			}
		}
	}
	else
	{
		drawableList.reserve(boxesCount);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak

		glm::vec3 origin = center - (boxToBoxDistance * glm::vec3(numBoxes.x-1, numBoxes.y-1, numBoxes.z-1) / 2.f);
		for(uint32_t k=0; k<numBoxes.z; k++) {
			auto planeZ = origin.z + (k * boxToBoxDistance.z);
			for(uint32_t j=0; j<numBoxes.y; j++) {
				auto lineY = origin.y + (j * boxToBoxDistance.y);
				for(uint32_t i=0; i<numBoxes.x; i++)
				{
					Geometry& g = (singleGeometry) ? *geometryList.front() : *geometryList[geometryIndex++];
					drawableList.emplace_back(g, 0, shaderStagingData, 128, 1, stateSetRoot);

					// shader staging data
					uint8_t* b = shaderStagingData.data<uint8_t>();
					ShaderData* d = reinterpret_cast<ShaderData*>(b);
					memset(d, 0, sizeof(ShaderData));
					d->diffuse = glm::vec3(1.f, 0.5f, 1.f);
					d->alpha = 1.f;
					*reinterpret_cast<glm::mat4*>(b+64) = glm::translate(glm::mat4(1.f),
						glm::vec3(origin.x + (i*boxToBoxDistance.x), lineY, planeZ));
				}
			}
		}
	}
}


void App::generateBakedBoxesScene(
	glm::uvec3 numBoxes,
	glm::vec3 center,
	glm::vec3 boxToBoxDistance,
	glm::vec3 boxSize)
{
	deleteScene();

	// make sure valid parameters were passed in
	if(numBoxes.x == 0 || numBoxes.y == 0 || numBoxes.z == 0)
		return;

	// create Geometry
	geometryList.reserve(1);
	geometryList.emplace_back(new Geometry(renderer));
	Geometry* geometry = geometryList.back();

	// alloc positions
	size_t numPositions = numBoxes.x * numBoxes.y * numBoxes.z * 8;
	StagingData positionStagingData = geometry->createVertexStagingData(numPositions * sizeof(glm::vec3));
	glm::vec3* positions = positionStagingData.data<glm::vec3>();

	// generate vertex positions
	glm::vec3 origin = center - (boxToBoxDistance * glm::vec3(numBoxes.x-1, numBoxes.y-1, numBoxes.z-1) / 2.f);
	glm::vec3 d = boxSize / 2.f;
	size_t index = 0;
	for(uint32_t k=0; k<numBoxes.z; k++) {
		auto planeZ = origin.z + (k * boxToBoxDistance.z);
		for(uint32_t j=0; j<numBoxes.y; j++) {
			auto lineY = origin.y + (j * boxToBoxDistance.y);
			for(uint32_t i=0; i<numBoxes.x; i++)
			{
				glm::vec3 t = glm::vec3(origin.x + (i * boxToBoxDistance.x), lineY, planeZ);
				positions[index++] = t + glm::vec3(-d.x, -d.y, -d.z);
				positions[index++] = t + glm::vec3( d.x, -d.y, -d.z);
				positions[index++] = t + glm::vec3(-d.x,  d.y, -d.z);
				positions[index++] = t + glm::vec3( d.x,  d.y, -d.z);
				positions[index++] = t + glm::vec3(-d.x, -d.y,  d.z);
				positions[index++] = t + glm::vec3( d.x, -d.y,  d.z);
				positions[index++] = t + glm::vec3(-d.x,  d.y,  d.z);
				positions[index++] = t + glm::vec3( d.x,  d.y,  d.z);
			}
		}
	}
	assert(index == numPositions);

	// alloc indices
	uint32_t numIndices = numBoxes.x * numBoxes.y * numBoxes.z * 36;
	StagingData indexStagingData = geometry->createIndexStagingData(numIndices * sizeof(uint32_t));
	uint32_t* indices = indexStagingData.data<uint32_t>();

	// generate indices
	index = 0;
	uint32_t boxIndexOffset = 0;
	for(uint32_t k=0; k<numBoxes.z; k++) {
		for(uint32_t j=0; j<numBoxes.y; j++) {
			for(uint32_t i=0; i<numBoxes.x; i++)
			{
				// face x,y
				indices[index++] = boxIndexOffset + 0;
				indices[index++] = boxIndexOffset + 2;
				indices[index++] = boxIndexOffset + 1;
				indices[index++] = boxIndexOffset + 1;
				indices[index++] = boxIndexOffset + 2;
				indices[index++] = boxIndexOffset + 3;

				// face x,z
				indices[index++] = boxIndexOffset + 0;
				indices[index++] = boxIndexOffset + 1;
				indices[index++] = boxIndexOffset + 4;
				indices[index++] = boxIndexOffset + 4;
				indices[index++] = boxIndexOffset + 1;
				indices[index++] = boxIndexOffset + 5;

				// face y,z
				indices[index++] = boxIndexOffset + 0;
				indices[index++] = boxIndexOffset + 4;
				indices[index++] = boxIndexOffset + 2;
				indices[index++] = boxIndexOffset + 2;
				indices[index++] = boxIndexOffset + 4;
				indices[index++] = boxIndexOffset + 6;

				// face x,y
				indices[index++] = boxIndexOffset + 4;
				indices[index++] = boxIndexOffset + 5;
				indices[index++] = boxIndexOffset + 6;
				indices[index++] = boxIndexOffset + 6;
				indices[index++] = boxIndexOffset + 5;
				indices[index++] = boxIndexOffset + 7;

				// face x,z
				indices[index++] = boxIndexOffset + 2;
				indices[index++] = boxIndexOffset + 6;
				indices[index++] = boxIndexOffset + 3;
				indices[index++] = boxIndexOffset + 3;
				indices[index++] = boxIndexOffset + 6;
				indices[index++] = boxIndexOffset + 7;

				// face y,z
				indices[index++] = boxIndexOffset + 1;
				indices[index++] = boxIndexOffset + 3;
				indices[index++] = boxIndexOffset + 5;
				indices[index++] = boxIndexOffset + 5;
				indices[index++] = boxIndexOffset + 3;
				indices[index++] = boxIndexOffset + 7;

				boxIndexOffset += 8;
			}
		}
	}
	assert(index == numIndices);

	// primitiveSet
	StagingData primitiveSetStagingData = geometry->createPrimitiveSetStagingData(sizeof(PrimitiveSetGpuData));
	PrimitiveSetGpuData* primitiveSet = primitiveSetStagingData.data<PrimitiveSetGpuData>();
	primitiveSet->count = numIndices;
	primitiveSet->first = 0;

	// create Drawables
	StagingData shaderStagingData;
	drawableList.reserve(1);  // this ensures that no exception is thrown during emplacing which would result in memory leak
	drawableList.emplace_back(*geometry, 0, shaderStagingData, 128, 1, stateSetRoot);

	// shader staging data
	uint8_t* b = shaderStagingData.data<uint8_t>();
	ShaderData* sd = reinterpret_cast<ShaderData*>(b);
	memset(sd, 0, sizeof(ShaderData));
	sd->diffuse = glm::vec3(1.f, 0.5f, 1.f);
	sd->alpha = 1.f;
	*reinterpret_cast<glm::mat4*>(b+64) = glm::mat4(1.f);
}


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
				else {
					cout << "Invalid test type \"" << start << "\"." << endl;
					printHelp = true;
				}
			}

			// select rendering setup
			if(strcmp(argv[i], "-r") == 0 || strncmp(argv[i], "--rendering-setup=", 18) == 0)
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
			if(strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--long") == 0)
				longTest = true;
			if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--print-frame-times") == 0)
				printFrameTimes = true;
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
		        "   -p or --print-frame-times - print times of each rendered frame\n"
		        "   --help or -h - prints this usage information" << endl;
		exit(99);
	}

	// projection and view matrix
	matrix.projection =
		glm::orthoLH_ZO(
			0.f,  // left
			float(imageExtent.width),  // right
			0.f,  // bottom
			float(imageExtent.height),  // top
			0.5f,  // near
			100.f  // far
		);
	matrix.view = glm::mat4(1.f);
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
		for(Geometry* g : geometryList)  delete g;
		geometryList.clear();

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
	case TestType::IndependentBoxesPerformance: cout << "   IndependentBoxesPerformance" << endl; break;
	case TestType::IndependentBoxesScene:     cout << "   IndependentBoxesScene" << endl; break;
	default: cout << "   Undefined" << endl;
	};
	cout << "Rendering setup:" << endl;
	switch(renderingSetup) {
	case RenderingSetup::Performance: cout << "   Performance" << endl; break;
	case RenderingSetup::Picking:     cout << "   Picking" << endl; break;
	default: cout << "   Undefined" << endl;
	};
	cout << endl;

	// create device
	device.create(
		instance,  // instance
		physicalDevice,
		graphicsQueueFamily,
		graphicsQueueFamily,
	#if 0 // enable to use debugPrintfEXT in shader code
		vector<const char*>{"VK_KHR_shader_non_semantic_info"},  //extensions
	#else
		nullptr,  // extensions
	#endif
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
	auto pipelineLayoutUnique =
		device.createPipelineLayoutUnique(
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

	// _sceneDataAllocation
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

	// fence
	renderingFinishedFence =
		device.createFence(
			vk::FenceCreateInfo{
				vk::FenceCreateFlags()  // flags
			}
		);

	// scene
	stateSetRoot.pipeline = &pipeline;
	switch(testType) {
	case TestType::TrianglePerformance:
	case TestType::TriangleStripPerformance:
	case TestType::DrawablePerformance:
	case TestType::PrimitiveSetPerformance:
	case TestType::BakedBoxesPerformance:
	case TestType::InstancedBoxesPerformance:
	case TestType::IndependentBoxesPerformance:
		generateInvisibleTriangleScene(glm::vec2(imageExtent.width, imageExtent.height), requestedNumTriangles, testType);
		break;
	case TestType::BakedBoxesScene: {
		float maxSize = float(min(imageExtent.width, imageExtent.height)) * 0.9f;
		generateBakedBoxesScene(
			glm::uvec3(100, 100, 100),  // numBoxes
			glm::vec3(maxSize / 2.f),  // center
			glm::vec3(2.f, 2.f, 2.f),  // boxToBoxDistance
			glm::vec3(1.f, 1.f, 1.f));  // boxSize
		break;
	}
	case TestType::InstancedBoxesScene: {
		float maxSize = float(min(imageExtent.width, imageExtent.height)) * 0.9f;
		generateBoxesScene(
			glm::uvec3(100, 100, 100),  // numBoxes
			glm::vec3(maxSize / 2.f),  // center
			glm::vec3(2.f, 2.f, 2.f),  // boxToBoxDistance
			glm::vec3(1.f, 1.f, 1.f),  // boxSize
			true,  // instanced
			true);  // singleGeometry
		break;
	}
	case TestType::IndependentBoxesScene: {
		float maxSize = float(min(imageExtent.width, imageExtent.height)) * 0.9f;
		generateBoxesScene(
			glm::uvec3(100, 100, 100),  // numBoxes
			glm::vec3(maxSize / 2.f),  // center
			glm::vec3(2.f, 2.f, 2.f),  // boxToBoxDistance
			glm::vec3(1.f, 1.f, 1.f),  // boxSize
			false,  // instanced
			false);  // singleGeometry
		break;
	}
	default: break;
	};
}


void App::frame(bool collectInfo)
{
	// begin the frame
	renderer.setCollectFrameInfo(collectInfo, calibratedTimestampsSupported);
	renderer.beginFrame();

	// submit all copy operations that were not submitted yet
	renderer.executeCopyOperations();

	// begin command buffer recording
	renderer.beginRecording(commandBuffer);

	// update SceneGpuData
	CadR::StagingData stagingSceneData = sceneDataAllocation.createStagingData();
	SceneGpuData* sceneData = stagingSceneData.data<SceneGpuData>();
	assert(sizeof(SceneGpuData) == sceneDataAllocation.size());
	//sceneData->projectionMatrix = projectionMatrix;
	sceneData->viewMatrix = matrix.view;
	sceneData->p11 = matrix.projection[0][0];
	sceneData->p22 = matrix.projection[1][1];
	sceneData->p33 = matrix.projection[2][2];
	sceneData->p43 = matrix.projection[3][2];

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
	chrono::steady_clock::duration testTime = longTest ? longTestDuration : shortTestDuration;
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
	cout << "      Performance: " << size_t(double(requestedNumTriangles) / median(frameTimeList, [](FrameTimeInfo& i) { return i.totalTime; }) / 1e6) << " Mtri/s" << endl;
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


int main(int argc, char** argv) {

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

	return 0;
}
