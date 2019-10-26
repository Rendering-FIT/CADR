#pragma once

#include <CadR/AttribSizeList.h>
#include <CadR/Export.h>
#include <vulkan/vulkan.hpp>
#include <vector>

namespace CadR {

class AttribSizeList;
class AttribStorage;
class Renderer;


typedef std::vector<vk::Format> AttribFormatList;


class CADR_EXPORT StateSet final {
protected:
	Renderer* _renderer;
	vk::Pipeline _pipeline;
	AttribSizeList _attribSizeList;
	AttribFormatList _attribFormatList;
	std::vector<vk::CommandBuffer> _commandBuffers;
public:

	StateSet();
	StateSet(Renderer* renderer);
	StateSet(vk::Pipeline pipeline,const AttribSizeList& attribSizeList,const AttribFormatList& attribFormatList);
	StateSet(Renderer* renderer,vk::Pipeline pipeline,const AttribSizeList& attribSizeList,const AttribFormatList& attribFormatList);
	StateSet(const StateSet&) = delete;
	StateSet(StateSet&&);
	~StateSet();
	StateSet& operator=(const StateSet&) = delete;
	StateSet& operator=(StateSet&& rhs);

	Renderer* renderer() const;
	vk::Pipeline pipeline() const;
	const AttribSizeList& attribSizeList() const;
	const AttribFormatList& attribFormatList() const;
	const std::vector<vk::CommandBuffer>& commandBuffers() const;
	vk::CommandBuffer commandBuffer(size_t index) const;

	void setPipeline(vk::Pipeline pipeline);
	void setAttribSizeList(const AttribSizeList& a);
	void setAttribSizeList(AttribSizeList&& a);
	void setAttribFormatList(const AttribFormatList& a);
	void setAttribFormatList(AttribFormatList&& a);
	void setPipelineAndAttribProps(vk::Pipeline pipeline,const AttribSizeList& attribSizeList,const AttribFormatList& attribFormatList);
	void setPipelineAndAttribProps(vk::Pipeline pipeline,AttribSizeList&& attribSizeList,AttribFormatList&& attribFormatList);
	void setCommandBuffers(const std::vector<vk::CommandBuffer>& commandBuffers);
	void setCommandBuffers(std::vector<vk::CommandBuffer>&& commandBuffers);
	void setCommandBuffer(size_t index,vk::CommandBuffer commandBuffer);
	void clearCommandBuffers();

};


}


// inline methods
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>
namespace CadR {

inline StateSet::StateSet() : _renderer(Renderer::get()), _pipeline(nullptr)  {}
inline StateSet::StateSet(Renderer* renderer) : _renderer(renderer), _pipeline(nullptr)  {}
inline StateSet::StateSet(vk::Pipeline pipeline,const AttribSizeList& attribSizeList,const AttribFormatList& attribFormatList)
	: _renderer(Renderer::get()), _pipeline(pipeline), _attribSizeList(attribSizeList), _attribFormatList(attribFormatList)  {}
inline StateSet::StateSet(Renderer* renderer,vk::Pipeline pipeline,const AttribSizeList& attribSizeList,const AttribFormatList& attribFormatList)
	: _renderer(renderer), _pipeline(pipeline), _attribSizeList(attribSizeList), _attribFormatList(attribFormatList)  {}
inline StateSet::StateSet(StateSet&& other) : _renderer(other._renderer), _pipeline(other._pipeline), _attribSizeList(other._attribSizeList), _attribFormatList(other._attribFormatList)  { other._pipeline=nullptr; }
inline StateSet& StateSet::operator=(StateSet&& rhs)  { _renderer=rhs._renderer; _pipeline=rhs._pipeline; rhs._pipeline=nullptr; _attribSizeList=rhs._attribSizeList; _attribFormatList=rhs._attribFormatList; return *this; }

inline Renderer* StateSet::renderer() const  { return _renderer; }
inline vk::Pipeline StateSet::pipeline() const  { return _pipeline; }
inline const AttribSizeList& StateSet::attribSizeList() const  { return _attribSizeList; }
inline const AttribFormatList& StateSet::attribFormatList() const  { return _attribFormatList; }
inline const std::vector<vk::CommandBuffer>& StateSet::commandBuffers() const  { return _commandBuffers; }
inline vk::CommandBuffer StateSet::commandBuffer(size_t index) const  { return _commandBuffers[index]; }

inline void StateSet::setPipeline(vk::Pipeline pipeline)  { if(_pipeline) (*_renderer->device())->destroy(_pipeline,nullptr,*_renderer->device()); _pipeline=pipeline; }
inline void StateSet::setAttribSizeList(const AttribSizeList& a)  { _attribSizeList=a; }
inline void StateSet::setAttribSizeList(AttribSizeList&& a)  { _attribSizeList=std::move(a); }
inline void StateSet::setAttribFormatList(const AttribFormatList& a)  { _attribFormatList=a; }
inline void StateSet::setAttribFormatList(AttribFormatList&& a)  { _attribFormatList=std::move(a); }
inline void StateSet::setPipelineAndAttribProps(vk::Pipeline pipeline,const AttribSizeList& attribSizeList,const AttribFormatList& attribFormatList)  { setPipeline(pipeline); _attribSizeList=attribSizeList; _attribFormatList=attribFormatList; }
inline void StateSet::setPipelineAndAttribProps(vk::Pipeline pipeline,AttribSizeList&& attribSizeList,AttribFormatList&& attribFormatList)  { setPipeline(pipeline); _attribSizeList=std::move(attribSizeList); _attribFormatList=std::move(attribFormatList); }
inline void StateSet::setCommandBuffers(const std::vector<vk::CommandBuffer>& commandBuffers)  { clearCommandBuffers(); _commandBuffers=commandBuffers; }
inline void StateSet::setCommandBuffers(std::vector<vk::CommandBuffer>&& commandBuffers)  { clearCommandBuffers(); _commandBuffers=std::move(commandBuffers); }
inline void StateSet::setCommandBuffer(size_t index,vk::CommandBuffer commandBuffer)  { if(_commandBuffers.size()<index) _commandBuffers.resize(index); else (*_renderer->device())->freeCommandBuffers(_renderer->stateSetCommandPool(),_commandBuffers[index],*_renderer->device()); _commandBuffers[index]=commandBuffer; }
inline void StateSet::clearCommandBuffers()  { if(_commandBuffers.size()==0) return; (*_renderer->device())->freeCommandBuffers(_renderer->stateSetCommandPool(),_commandBuffers,*_renderer->device()); _commandBuffers.clear(); }

}
