#include <CadPL/PipelineSceneGraph.h>

using namespace std;
using namespace CadPL;


CadR::StateSet& PipelineSceneGraph::createStateSet(const ShaderState& shaderState,
	const PipelineState& pipelineState, decltype(_stateSetMap)::insert_commit_data& insertData)
{
	StateSetMapItem* item = new StateSetMapItem(shaderState, pipelineState, _root->renderer());
	_stateSetMap.insert_commit(*item, insertData);
	item->sharedPipeline = _pipelineLibrary->getOrCreatePipeline(shaderState, pipelineState);
	item->stateSet.pipeline = item->sharedPipeline.cadrPipeline();
	_root->childList.append(item->stateSet);

	struct {
		array<uint16_t,ShaderState::maxNumAttribs> attribAccessInfo;
		uint32_t attribSetup;
		uint32_t materialSetup;
		float pointSize;
	} pushData = {
		shaderState.attribAccessInfo,
		shaderState.attribSetup,
		shaderState.materialSetup,
		shaderState.pointSize,
	};
	item->stateSet.recordCallList.emplace_back(
		[pushData](CadR::StateSet& ss, vk::CommandBuffer commandBuffer, vk::PipelineLayout currentPipelineLayout) {
			ss.renderer().device().cmdPushConstants(
				commandBuffer,  // commandBuffer
				currentPipelineLayout,  // pipelineLayout
				vk::ShaderStageFlagBits::eAllGraphics,  // stageFlags
				16,  // offset
				sizeof(pushData),  // size
				&pushData  // pValues
			);
		}
	);

	return item->stateSet;
}
