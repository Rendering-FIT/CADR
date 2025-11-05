#include <CadPL/PipelineSceneGraph.h>

using namespace std;
using namespace CadPL;


CadR::StateSet& PipelineSceneGraph::createStateSet(const ShaderState& shaderState,
	const PipelineState& pipelineState, decltype(_stateSetMap)::insert_commit_data& insertData)
{
	StateSetMapItem* item = new StateSetMapItem(shaderState, pipelineState, _root->renderer());
	_stateSetMap.insert_commit(*item, insertData);
	//item->sharedPipeline = _pipelineLibrary->getOrCreatePipeline(shaderState, pipelineState);
	//item->stateSet.pipeline = item->sharedPipeline.get();
	return item->stateSet;
}
