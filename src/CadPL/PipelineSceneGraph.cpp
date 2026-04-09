// SPDX-FileCopyrightText: 2025-2026 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT

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
	} pushData1 = {
		shaderState.attribAccessInfo,
		shaderState.attribSetup,
		shaderState.materialSetup,
		shaderState.pointSize,
	};
	struct {
		array<uint32_t,10> textureSetup;
		array<uint8_t,8> lightSetup;
	} pushData2 = {
		shaderState.textureSetup,
		shaderState.lightSetup,
	};
	item->stateSet.recordCallList.emplace_back(
		[pushData1,pushData2](CadR::StateSet& ss, vk::CommandBuffer commandBuffer, vk::PipelineLayout currentPipelineLayout) {
			CadR::VulkanDevice& device = ss.renderer().device();
			device.cmdPushConstants(
				commandBuffer,  // commandBuffer
				currentPipelineLayout,  // pipelineLayout
				vk::ShaderStageFlagBits::eAllGraphics,  // stageFlags
				16,  // offset
				sizeof(pushData1),  // size
				&pushData1  // pValues
			);
			device.cmdPushConstants(
				commandBuffer,  // commandBuffer
				currentPipelineLayout,  // pipelineLayout
				vk::ShaderStageFlagBits::eAllGraphics,  // stageFlags
				64,  // offset
				sizeof(pushData2),  // size
				&pushData2  // pValues
			);
		}
	);

	return item->stateSet;
}
