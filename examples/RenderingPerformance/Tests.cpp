#include "Tests.h"
#include <CadR/Drawable.h>
#include <CadR/Geometry.h>
#include <CadR/StagingData.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace CadR {
class Renderer;
class StateSet;
}

using namespace std;


struct PrimitiveSetGpuData {
	uint32_t count;
	uint32_t first;
};


struct MaterialData {
	glm::vec3 ambient;
	uint32_t materialType;
	glm::vec3 diffuse;
	float alpha;
	glm::vec3 specular;
	float shininess;
	glm::vec3 emission;
	float pointSize;
};
static_assert(sizeof(MaterialData) == 64, "Size of MaterialData is not 64.");


#if 0
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
		drawableList.reserve(drawableList.size() + 1);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak
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
		size_t baseNumDrawables = drawableList.size();
		drawableList.reserve(baseNumDrawables + requestedNumTriangles);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak
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

				if(drawableList.size() - baseNumDrawables == requestedNumTriangles)
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

				if(drawableList.size() - baseNumDrawables == requestedNumTriangles)
					return;
			}
		}
	}
	else if (testType == TestType::PrimitiveSetPerformance)
	{
		drawableList.reserve(drawableList.size() + requestedNumTriangles);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak
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
		drawableList.reserve(drawableList.size() + 1);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak
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
		drawableList.reserve(drawableList.size() + numPrimitiveSets);  // this ensures that exception cannot be thrown during emplacing which would result in memory leak
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
#endif


static const uint32_t boxIndices[36] =
{
	0, 2, 1,  1, 2, 3,  // face x,y
	0, 1, 4,  4, 1, 5,  // face x,z
	0, 4, 2,  2, 4, 6,  // face y,z
	4, 5, 6,  6, 5, 7,  // face x,y
	2, 6, 3,  3, 6, 7,  // face x,z
	1, 3, 5,  5, 3, 7,  // face y,z
};


static void generateBoxesScene(
	glm::uvec3 numBoxes,
	glm::vec3 center,
	glm::vec3 boxToBoxDistance,
	glm::vec3 boxSize,
	bool instanced,
	bool singleGeometry,
	size_t numMaterials,
	bool doNotCreateDrawables,
	CadR::Renderer& renderer,
	CadR::StateSet& stateSetRoot,
	vector<CadR::Geometry>& geometryList,
	vector<CadR::Drawable>& drawableList,
	vector<CadR::MatrixList>& matrixLists,
	vector<CadR::DataAllocation>& materialList)
{
	// make sure valid parameters were passed in
	if(instanced == true && singleGeometry == false)
		throw runtime_error("RenderingPerformance::generateBoxScene(): Invalid function arguments. "
		                    "If parameter instanced is true, singleGeometry parameter must be true as well.");
	if(numBoxes.x == 0 || numBoxes.y == 0 || numBoxes.z == 0)
		return;

	// generate geometries
	size_t boxesCount = numBoxes.x * numBoxes.y * numBoxes.z;
	size_t numGeometries = (singleGeometry) ? 1 : boxesCount;
	size_t geometryBaseIndex = geometryList.size();
	geometryList.reserve(geometryBaseIndex + numGeometries);
	for(size_t i=0; i<numGeometries; i++) {

		// create Geometry
		geometryList.emplace_back(renderer);
		CadR::Geometry& geometry = geometryList.back();

		// alloc positions
		CadR::StagingData positionStagingData = geometry.createVertexStagingData(8 * sizeof(glm::vec3));
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
		CadR::StagingData indexStagingData = geometry.createIndexStagingData(sizeof(boxIndices));
		uint32_t* indices = indexStagingData.data<uint32_t>();
		memcpy(indices, boxIndices, sizeof(boxIndices));

		// primitiveSet
		CadR::StagingData primitiveSetStagingData = geometry.createPrimitiveSetStagingData(sizeof(PrimitiveSetGpuData));
		PrimitiveSetGpuData* primitiveSet = primitiveSetStagingData.data<PrimitiveSetGpuData>();
		primitiveSet->count = 36;
		primitiveSet->first = 0;
	}

	// create materials
	size_t materialBaseIndex = materialList.size();
	materialList.reserve(materialList.size() + numMaterials);
	for(size_t i=0; i<numMaterials; i++) {
		CadR::DataAllocation& a = materialList.emplace_back(renderer);
		MaterialData& m = a.editNewContent<MaterialData>();
		m.ambient = { 0.f, 0.f, 0.f };
		m.materialType = 0;
		float angle = float(i) / numMaterials;
		m.diffuse = { cos(angle-glm::pi<float>()/3.f), cos(angle), cos(angle+glm::pi<float>()/3.f) };
		m.alpha = 1.f;
		m.specular = { 0.f, 0.f, 0.f };
		m.shininess = 0.f;
		m.emission = { 0.f, 0.f, 0.f };
		m.pointSize = 0.f;
	}

	// create matrix lists
	size_t baseMatrixListsIndex = matrixLists.size();
	if(instanced)
	{
		// reserve space in vector
		// (this ensures that no exception is thrown during emplacing which would result in memory leak)
		matrixLists.reserve(matrixLists.size() + 1);

		// generate transformations
		CadR::MatrixList& ml = matrixLists.emplace_back(renderer);
		glm::mat4* m = ml.editNewContent(boxesCount);
		glm::vec3 origin = center - (boxToBoxDistance * glm::vec3(numBoxes.x-1, numBoxes.y-1, numBoxes.z-1) / 2.f);
		for(uint32_t k=0; k<numBoxes.z; k++) {
			auto planeZ = origin.z + (k * boxToBoxDistance.z);
			for(uint32_t j=0; j<numBoxes.y; j++) {
				auto lineY = origin.y + (j * boxToBoxDistance.y);
				for(uint32_t i=0; i<numBoxes.x; i++)
				{
					*m = glm::translate(glm::mat4(1.f),
						glm::vec3(origin.x + (i*boxToBoxDistance.x), lineY, planeZ));
					m++;
				}
			}
		}
	}
	else
	{
		// reserve space in vector
		// (this ensures that no exception is thrown during emplacing which would result in memory leak)
		matrixLists.reserve(matrixLists.size() + boxesCount);

		// generate transformations
		glm::vec3 origin = center - (boxToBoxDistance * glm::vec3(numBoxes.x-1, numBoxes.y-1, numBoxes.z-1) / 2.f);
		for(uint32_t k=0; k<numBoxes.z; k++) {
			auto planeZ = origin.z + (k * boxToBoxDistance.z);
			for(uint32_t j=0; j<numBoxes.y; j++) {
				auto lineY = origin.y + (j * boxToBoxDistance.y);
				for(uint32_t i=0; i<numBoxes.x; i++)
				{
					// create transformation matrix list
					CadR::MatrixList& ml = matrixLists.emplace_back(renderer);
					glm::mat4* m = ml.editNewContent(1);
					*m = glm::translate(glm::mat4(1.f),
						glm::vec3(origin.x + (i*boxToBoxDistance.x), lineY, planeZ));
				}
			}
		}
	}

	if(doNotCreateDrawables)
		return;

	// create Drawables
	if(instanced) {

		// reserve space in vector
		// (this ensures that no exception is thrown during emplacing which would result in memory leak)
		drawableList.reserve(drawableList.size() + 1);

		// append single MatrixList and single Drawable
		CadR::MatrixList& ml = matrixLists[baseMatrixListsIndex];
		drawableList.emplace_back(geometryList.front(), 0, ml, materialList[materialBaseIndex], stateSetRoot);

	}
	else {

		// reserve space in vector
		// (this ensures that no exception is thrown during emplacing which would result in memory leak)
		drawableList.reserve(drawableList.size() + boxesCount);

		size_t materialIndex = 0;
		for(size_t i=baseMatrixListsIndex,e=matrixLists.size(); i<e; i++) {
			
			// create Drawable
			CadR::MatrixList& ml = matrixLists[i];
			CadR::Geometry& g = (singleGeometry) ? geometryList.front() : geometryList[geometryBaseIndex++];
			drawableList.emplace_back(g, 0, ml, materialList[materialBaseIndex + materialIndex], stateSetRoot);

			// update material index
			materialIndex++;
			if(materialIndex >= numMaterials)
				materialIndex = 0;
		}
	}
}


static void updateDrawablesForShowHideScene(
	size_t frameNumber,
	glm::uvec3 numBoxes,
	glm::vec3 center,
	glm::vec3 boxToBoxDistance,
	bool singleGeometry,
	bool instanced,
	CadR::StateSet& stateSetRoot,
	size_t baseGeometryIndex,
	size_t baseDrawableIndex,
	size_t baseMatrixListIndex,
	vector<CadR::Geometry>& geometryList,
	vector<CadR::Drawable>& drawableList,
	vector<CadR::MatrixList>& matrixLists,
	vector<CadR::DataAllocation>& materialList)
{
	if(instanced)
	{
		// change visibility between even and odd frames
		uint32_t startI = frameNumber & 0x01;

		// new matrix list content
		CadR::MatrixList& ml = matrixLists[baseMatrixListIndex];
		glm::mat4* m = ml.editNewContent(((numBoxes.x - startI) / 2) * numBoxes.y * numBoxes.z);

		// update matrices
		glm::vec3 origin = center - (boxToBoxDistance * glm::vec3(numBoxes.x-1, numBoxes.y-1, numBoxes.z-1) / 2.f);
		for(uint32_t k=0; k<numBoxes.z; k++) {
			auto planeZ = origin.z + (k * boxToBoxDistance.z);
			for(uint32_t j=0; j<numBoxes.y; j++) {
				auto lineY = origin.y + (j * boxToBoxDistance.y);
				for(uint32_t i=startI; i<numBoxes.x; i+=2)
				{
					*m = glm::translate(glm::mat4(1.f),
						glm::vec3(origin.x + (i*boxToBoxDistance.x), lineY, planeZ));
					m++;
				}
			}
		}
	}
	else
	{
		// delete old drawables
		drawableList.erase(drawableList.begin() + baseDrawableIndex, drawableList.end());

		// create new drawables
		if(singleGeometry) {
			CadR::Geometry& g = geometryList[baseGeometryIndex];
			for(size_t i=baseMatrixListIndex+(frameNumber&0x01), c=matrixLists.size()-i; i<c; i+=2)
				drawableList.emplace_back(g, 0, matrixLists[i], materialList.back(), stateSetRoot);
		}
		else {
			size_t geometryIndex = baseGeometryIndex;
			for(size_t i=baseMatrixListIndex+(frameNumber&0x01), c=matrixLists.size()-i; i<c; i+=2)
				drawableList.emplace_back(geometryList[geometryIndex++], 0, matrixLists[i], materialList.back(), stateSetRoot);
		}
	}
}


static void generateBakedBoxesScene(
	glm::uvec3 numBoxes,
	glm::vec3 center,
	glm::vec3 boxToBoxDistance,
	glm::vec3 boxSize,
	CadR::Renderer& renderer,
	CadR::StateSet& stateSetRoot,
	vector<CadR::Geometry>& geometryList,
	vector<CadR::Drawable>& drawableList,
	vector<CadR::MatrixList>& matrixLists,
	vector<CadR::DataAllocation>& materialList)
{
	// make sure valid parameters were passed in
	if(numBoxes.x == 0 || numBoxes.y == 0 || numBoxes.z == 0)
		return;

	// create Geometry
	geometryList.reserve(1);
	geometryList.emplace_back(renderer);
	CadR::Geometry& geometry = geometryList.back();

	// alloc positions
	size_t numPositions = numBoxes.x * numBoxes.y * numBoxes.z * 8;
	CadR::StagingData positionStagingData = geometry.createVertexStagingData(numPositions * sizeof(glm::vec3));
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
	CadR::StagingData indexStagingData = geometry.createIndexStagingData(numIndices * sizeof(uint32_t));
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
	CadR::StagingData primitiveSetStagingData = geometry.createPrimitiveSetStagingData(sizeof(PrimitiveSetGpuData));
	PrimitiveSetGpuData* primitiveSet = primitiveSetStagingData.data<PrimitiveSetGpuData>();
	primitiveSet->count = numIndices;
	primitiveSet->first = 0;

	// create matrix list
	matrixLists.reserve(matrixLists.size() + 1);  // this ensures that no exception is thrown during emplacing which would result in memory leak
	CadR::MatrixList& ml = matrixLists.emplace_back(renderer);
	*ml.editNewContent(1) = glm::mat4(1.f);

	// create material
	materialList.reserve(materialList.size() + 1);  // this ensures that no exception is thrown during emplacing which would result in memory leak
	CadR::DataAllocation& materialData = materialList.emplace_back(renderer);
	MaterialData& m = materialData.editNewContent<MaterialData>();
	m.ambient = { 0.f, 0.f, 0.f };
	m.materialType = 0;
	m.diffuse = { 1.f, 0.5f, 1.f };
	m.alpha = 1.f;
	m.specular = { 0.f, 0.f, 0.f };
	m.shininess = 0.f;
	m.emission = { 0.f, 0.f, 0.f };
	m.pointSize = 0.f;

	// create Drawable
	drawableList.reserve(drawableList.size() + 1);  // this ensures that no exception is thrown during emplacing which would result in memory leak
	drawableList.emplace_back(geometry, 0, ml, materialData, stateSetRoot);
}


void createTestScene(
	TestType testType,
	int imageWidth,
	int imageHeight,
	CadR::Renderer& renderer,
	CadR::StateSet& stateSetRoot,
	vector<CadR::Geometry>& geometryList,
	vector<CadR::Drawable>& drawableList,
	vector<CadR::MatrixList>& matrixLists,
	vector<CadR::DataAllocation>& materialList)
{
	switch(testType) {
	case TestType::TrianglePerformance:
	case TestType::TriangleStripPerformance:
	case TestType::DrawablePerformance:
	case TestType::PrimitiveSetPerformance:
	case TestType::BakedBoxesPerformance:
	case TestType::InstancedBoxesPerformance:
	case TestType::IndependentBoxesPerformance:
		//generateInvisibleTriangleScene(glm::vec2(imageExtent.width, imageExtent.height), requestedNumTriangles, testType);
		break;
	case TestType::BakedBoxesScene: {
		float maxSize = float(min(imageWidth, imageHeight)) * 0.9f;
		generateBakedBoxesScene(
			glm::uvec3(100, 100, 100),  // numBoxes
			glm::vec3(0.f, 0.f, 0.f),  // center
			glm::vec3(maxSize / 100.f),  // boxToBoxDistance
			glm::vec3(maxSize / 200.f),  // boxSize
			renderer,
			stateSetRoot,
			geometryList,
			drawableList,
			matrixLists,
			materialList);
		break;
	}
	case TestType::InstancedBoxesScene: {
		float maxSize = float(min(imageWidth, imageHeight)) * 0.9f;
		generateBoxesScene(
			glm::uvec3(100, 100, 100),  // numBoxes
			glm::vec3(0.f, 0.f, 0.f),  // center
			glm::vec3(maxSize / 100.f),  // boxToBoxDistance
			glm::vec3(maxSize / 200.f),  // boxSize
			true,  // instanced
			true,  // singleGeometry
			1,  // numMaterials
			false,  // doNotCreateDrawables
			renderer,
			stateSetRoot,
			geometryList,
			drawableList,
			matrixLists,
			materialList);
		break;
	}
	case TestType::IndependentBoxesScene: {
		float maxSize = float(min(imageWidth, imageHeight)) * 0.9f;
		generateBoxesScene(
			glm::uvec3(100, 100, 100),  // numBoxes
			glm::vec3(0.f, 0.f, 0.f),  // center
			glm::vec3(maxSize / 100.f),  // boxToBoxDistance
			glm::vec3(maxSize / 200.f),  // boxSize
			false,  // instanced
			false,  // singleGeometry
			1,  // numMaterials
			false,  // doNotCreateDrawables
			renderer,
			stateSetRoot,
			geometryList,
			drawableList,
			matrixLists,
			materialList);
		break;
	}
	case TestType::IndependentBoxes1000MaterialsScene: {
		float maxSize = float(min(imageWidth, imageHeight)) * 0.9f;
		generateBoxesScene(
			glm::uvec3(100, 100, 100),  // numBoxes
			glm::vec3(0.f, 0.f, 0.f),  // center
			glm::vec3(maxSize / 100.f),  // boxToBoxDistance
			glm::vec3(maxSize / 200.f),  // boxSize
			false,  // instanced
			false,  // singleGeometry
			1000,  // numMaterials
			false,  // doNotCreateDrawables
			renderer,
			stateSetRoot,
			geometryList,
			drawableList,
			matrixLists,
			materialList);
		break;
	}
	case TestType::IndependentBoxes1000000MaterialsScene: {
		float maxSize = float(min(imageWidth, imageHeight)) * 0.9f;
		generateBoxesScene(
			glm::uvec3(100, 100, 100),  // numBoxes
			glm::vec3(0.f, 0.f, 0.f),  // center
			glm::vec3(maxSize / 100.f),  // boxToBoxDistance
			glm::vec3(maxSize / 200.f),  // boxSize
			false,  // instanced
			false,  // singleGeometry
			1000000,  // numMaterials
			false,  // doNotCreateDrawables
			renderer,
			stateSetRoot,
			geometryList,
			drawableList,
			matrixLists,
			materialList);
		break;
	}
	case TestType::IndependentBoxesShowHideScene: {
		float maxSize = float(min(imageWidth, imageHeight)) * 0.9f;
		generateBoxesScene(
			glm::uvec3(100, 100, 100),  // numBoxes
			glm::vec3(0.f, 0.f, 0.f),  // center
			glm::vec3(maxSize / 100.f),  // boxToBoxDistance
			glm::vec3(maxSize / 200.f),  // boxSize
			false,  // instanced
			false,  // singleGeometry
			1,  // numMaterials
			true,  // doNotCreateDrawables
			renderer,
			stateSetRoot,
			geometryList,
			drawableList,
			matrixLists,
			materialList);
		break;
	}
	default: break;
	};
}


void updateTestScene(
	TestType testType,
	int imageWidth,
	int imageHeight,
	CadR::Renderer& renderer,
	CadR::StateSet& stateSetRoot,
	vector<CadR::Geometry>& geometryList,
	vector<CadR::Drawable>& drawableList,
	vector<CadR::MatrixList>& matrixLists,
	vector<CadR::DataAllocation>& materialList)
{
	switch(testType) {
	case TestType::IndependentBoxesShowHideScene: {
		float maxSize = float(min(imageWidth, imageHeight)) * 0.9f;
		updateDrawablesForShowHideScene(
			renderer.frameNumber(),  // frameNumber
			glm::uvec3(100, 100, 100),  // numBoxes
			glm::vec3(0.f, 0.f, 0.f),  // center
			glm::vec3(maxSize / 100.f),  // boxToBoxDistance
			false,  // singleGeometry
			false, // instanced
			stateSetRoot,  // stateSetRoot
			0,  // baseGeometryIndex
			0,  // baseDrawableIndex
			0,  // baseMatrisListIndex
			geometryList,
			drawableList,
			matrixLists,
			materialList);
		break;
	}
	default:;
	}
}
