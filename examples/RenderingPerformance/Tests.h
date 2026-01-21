#include <vector>

namespace CadR {
class DataAllocation;
class Drawable;
class Geometry;
class MatrixList;
class Renderer;
class StateSet;
}


enum class TestType
{
	Undefined,
	TrianglePerformance,
	TriangleStripPerformance,
	DrawablePerformance,
	PrimitiveSetPerformance,
	BakedBoxesPerformance,
	InstancedBoxesPerformance,
	IndependentBoxesPerformance,

	BakedBoxesScene,
	InstancedBoxesScene,
	IndependentBoxesScene,
	IndependentBoxes1000MaterialsScene,
	IndependentBoxes1000000MaterialsScene,
	IndependentBoxesShowHideScene,
};


void createTestScene(
	TestType testType,
	int imageWidth,
	int imageHeight,
	CadR::Renderer& renderer,
	CadR::StateSet& stateSetRoot,
	std::vector<CadR::Geometry>& geometryList,
	std::vector<CadR::Drawable>& drawableList,
	std::vector<CadR::MatrixList>& matrixLists,
	std::vector<CadR::DataAllocation>& materialList);
void updateTestScene(
	TestType testType,
	int imageWidth,
	int imageHeight,
	CadR::Renderer& renderer,
	CadR::StateSet& stateSetRoot,
	std::vector<CadR::Geometry>& geometryList,
	std::vector<CadR::Drawable>& drawableList,
	std::vector<CadR::MatrixList>& matrixLists,
	std::vector<CadR::DataAllocation>& materialList);
