#include <CadR/StateSet.h>
#include <CadR/Pipeline.h>
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>

using namespace std;
using namespace CadR;


const ParentChildListOffsets StateSet::parentChildListOffsets{
	size_t(&reinterpret_cast<StateSet*>(0)->parentList),
	size_t(&reinterpret_cast<StateSet*>(0)->childList)
};


void StateSet::appendDrawableInternal(Drawable& d, DrawableGpuData gpuData)
{
	d._stateSet = this;
	d._indexIntoStateSet = uint32_t(_drawableDataList.size());
	_drawableDataList.emplace_back(gpuData);
	_drawablePtrList.emplace_back(&d);
}


void StateSet::removeDrawableInternal(Drawable& d) noexcept
{
	uint32_t i = d._indexIntoStateSet;
	size_t lastIndex = _drawableDataList.size()-1;
	if(i == lastIndex) {
		// remove last element
		_drawableDataList.pop_back();
		_drawablePtrList.pop_back();
	}
	else {
		// remove element by replacing it with the last element
		_drawableDataList[i] = _drawableDataList[lastIndex];
		_drawableDataList.pop_back();
		Drawable* movedDrawable = _drawablePtrList[lastIndex];
		_drawablePtrList[i] = movedDrawable;
		_drawablePtrList.pop_back();
		movedDrawable->_indexIntoStateSet = i;
	}
}


void StateSet::removeAllDrawables() noexcept
{
	for(Drawable* d : _drawablePtrList)
		d->_indexIntoStateSet = ~0u;
	_drawableDataList.clear();
	_drawablePtrList.clear();
}


void StateSet::allocDescriptorSet(vk::DescriptorType type, vk::DescriptorSetLayout layout)
{
	freeDescriptorSets();

	VulkanDevice& d = _renderer->device();
	_descriptorPool =
		d.createDescriptorPool(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),  // flags
				1,  // maxSets
				1,  // poolSizeCount
				array<vk::DescriptorPoolSize,1>{  // pPoolSizes
					vk::DescriptorPoolSize{
						type,  // type
						1,  // descriptorCount
					},
				}.data()
			)
		);

	_descriptorSetList =
		d.allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				_descriptorPool,  // descriptorPool
				1,  // descriptorSetCount
				&layout  // descriptorSetLayout
			)
		);
}


void StateSet::allocDescriptorSet(uint32_t poolSizeCount, vk::DescriptorPoolSize* poolSizeList, vk::DescriptorSetLayout layout)
{
	freeDescriptorSets();

	VulkanDevice& d = _renderer->device();
	_descriptorPool =
		d.createDescriptorPool(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),  // flags
				1,  // maxSets
				poolSizeCount,  // poolSizeCount
				poolSizeList  // pPoolSizes
			)
		);

	_descriptorSetList =
		d.allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				_descriptorPool,  // descriptorPool
				1,  // descriptorSetCount
				&layout  // descriptorSetLayout
			)
		);
}


void StateSet::allocDescriptorSets(const vk::DescriptorPoolCreateInfo& descriptorPoolCreateInfo,
	uint32_t layoutCount, const vk::DescriptorSetLayout* layoutList, const void* descriptorInfoPNext)
{
	freeDescriptorSets();

	VulkanDevice& d = _renderer->device();
	_descriptorPool = d.createDescriptorPool(descriptorPoolCreateInfo);

	_descriptorSetList =
		d.allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				_descriptorPool,  // descriptorPool
				layoutCount,  // descriptorSetCount
				layoutList  // descriptorSetLayout
			).setPNext(
				descriptorInfoPNext
			)
		);
}


void StateSet::freeDescriptorSets() noexcept
{
	if(_descriptorPool) {
		_renderer->device().destroy(_descriptorPool);
		_descriptorPool = nullptr;
		_descriptorSetList.clear();
	}
}


void StateSet::updateDescriptorSet(const vk::WriteDescriptorSet& w)
{
	_renderer->device().updateDescriptorSets(
		1,  // descriptorWriteCount
		&w,  // pDescriptorWrites
		0,  // descriptorCopyCount
		nullptr  // pDescriptorCopies
	);
}


void StateSet::updateDescriptorSet(uint32_t writeCount, vk::WriteDescriptorSet* writes)
{
	_renderer->device().updateDescriptorSets(
		writeCount,  // descriptorWriteCount
		writes,  // pDescriptorWrites
		0,  // descriptorCopyCount
		nullptr  // pDescriptorCopies
	);
}


void StateSet::updateDescriptorSet(uint32_t writeCount, vk::WriteDescriptorSet* writes, uint32_t copyCount, vk::CopyDescriptorSet* copies)
{
	_renderer->device().updateDescriptorSets(
		writeCount,  // descriptorWriteCount
		writes,  // pDescriptorWrites
		copyCount,  // descriptorCopyCount
		copies  // pDescriptorCopies
	);
}


size_t StateSet::prepareRecording()
{
	// call user-registered functions
	_skipRecording = !_forceRecording;
	for(auto& f : prepareCallList)
		f(*this);

	// recursively call child-StateSets and process number of drawables
	size_t numDrawables = _drawableDataList.size();
	for(StateSet& ss : childList) {
		numDrawables += ss.prepareRecording();
		_skipRecording = _skipRecording && ss._skipRecording;
	}
	_skipRecording = _skipRecording && (numDrawables == 0);
	return numDrawables;
}


void StateSet::recordToCommandBuffer(vk::CommandBuffer commandBuffer, vk::PipelineLayout currentPipelineLayout, size_t& drawableCounter)
{
	// optimization
	// to not process StateSet subgraphs that does not have any Drawables
	if(_skipRecording)
		return;

	// bind pipeline
	VulkanDevice& device = _renderer->device();
	if(pipeline) {
		if(pipeline->get())
			device.cmdBindPipeline(commandBuffer, vk::PipelineBindPoint::eGraphics, pipeline->get());
		currentPipelineLayout = pipeline->layout();
	}

	// bind descriptor sets
	if(!_descriptorSetList.empty()) {
		device.cmdBindDescriptorSets(
			commandBuffer,  // commandBuffer
			vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
			currentPipelineLayout,  // layout
			_firstDescriptorSetIndex,  // firstSet
			_descriptorSetList,  // descriptorSets
			_dynamicOffsets  // dynamicOffsets
		);
	}

	// call user-registered functions
	for(auto& f : recordCallList)
		f(*this, commandBuffer, currentPipelineLayout);

	size_t numDrawables = _drawableDataList.size();
	if(numDrawables > 0) {

		// copy drawable data
		memcpy(
			&_renderer->drawableStagingData()[drawableCounter],  // dst
			_drawableDataList.data(),  // src
			numDrawables*sizeof(DrawableGpuData)  // size
		);

		// update address of drawable pointers
		device.cmdPushConstants(
			commandBuffer,  // commandBuffer
			currentPipelineLayout,  // pipelineLayout
			vk::ShaderStageFlagBits::eAllGraphics,  // stageFlags
			8,  // offset
			sizeof(uint64_t),  // size
			array<uint64_t,1>{  // pValues
				_renderer->drawablePointersBufferAddress() + (drawableCounter * Renderer::drawablePointersRecordSize),  // stateSetDrawablePointersPtr
			}.data()
		);

		// draw command
		device.cmdDrawIndirect(
			commandBuffer,  // commandBuffer
			_renderer->drawIndirectBuffer(),  // buffer
			drawableCounter * sizeof(vk::DrawIndirectCommand),  // offset
			uint32_t(numDrawables),  // drawCount
			sizeof(vk::DrawIndirectCommand)  // stride
		);

		// update drawableCounter
		drawableCounter += numDrawables;

	}

	// record child StateSets
	for(StateSet& child : childList)
		child.recordToCommandBuffer(commandBuffer, currentPipelineLayout, drawableCounter);
}
