#pragma once

#include <bitset>
#include <map>
#include <tuple>
#include <vector>
#include <boost/intrusive/splay_set.hpp>
#include <CadR/StateSet.h>
#include <CadPL/PipelineLibrary.h>
#include <CadPL/ShaderLibrary.h>

namespace CadPL {


class CADPL_EXPORT PipelineSceneGraph {
protected:

	PipelineLibrary* _pipelineLibrary;
	ShaderLibrary* _shaderLibrary;
	bool _deleteLibraries;
	CadR::StateSet* _root;  //< The root StateSet under which uber-shader StateSet and all kinds of optimized StateSets are placed.

	// StateSet map
	struct StateSetMapItem {
		boost::intrusive::bs_set_member_hook<> memberHook;
		ShaderState shaderState;
		PipelineState pipelineState;
		CadR::StateSet stateSet;
		SharedPipeline sharedPipeline;

		StateSetMapItem(const ShaderState& shaderState, const PipelineState& pipelineState,
		                CadR::Renderer& renderer);
	};
	struct mapKey {
		using type = std::tuple<const ShaderState&,const PipelineState&>;
		type operator()(const StateSetMapItem& item) const  { return type{item.shaderState, item.pipelineState}; }
	};
	boost::intrusive::splay_set<StateSetMapItem,
		boost::intrusive::key_of_value<mapKey>,
		boost::intrusive::member_hook<StateSetMapItem, boost::intrusive::bs_set_member_hook<>,
			&StateSetMapItem::memberHook>> _stateSetMap;
	static StateSetMapItem& stateSetToStateSetMapItem(CadR::StateSet& ss);

	// optimization levels
	std::vector<std::bitset<ShaderState::numOptimizeFlags>> _optimizationLevels;
	static inline const std::vector<std::bitset<ShaderState::numOptimizeFlags>> defaultOptimizationLevels = {
		ShaderState::OptimizeNone, ShaderState::OptimizeAttribs, ShaderState::OptimizeAll,
	};

	// internal functions
	PipelineSceneGraph(nullptr_t, CadR::StateSet& root,
	                   const std::vector<std::bitset<ShaderState::numOptimizeFlags>>& optimizationLevels);
	CadR::StateSet& createStateSet(const ShaderState& shaderState, const PipelineState& pipelineState,
	                               decltype(_stateSetMap)::insert_commit_data& insertData);

public:

	// construction and destruction
	PipelineSceneGraph() noexcept;
	PipelineSceneGraph(CadR::StateSet& root,
		const std::vector<std::bitset<ShaderState::numOptimizeFlags>>& optimizationLevels = defaultOptimizationLevels,
		vk::PipelineCache pipelineCache = nullptr, uint32_t maxTextures = 250000);
	PipelineSceneGraph(PipelineLibrary& pipelineLibrary, ShaderLibrary& shaderLibrary, CadR::StateSet& root,
		const std::vector<std::bitset<ShaderState::numOptimizeFlags>>& optimizationLevels = defaultOptimizationLevels);
	~PipelineSceneGraph() noexcept;
	void init(CadR::StateSet& root,
		const std::vector<std::bitset<ShaderState::numOptimizeFlags>>& optimizationLevels = defaultOptimizationLevels,
		vk::PipelineCache pipelineCache = nullptr, uint32_t maxTextures = 250000);
	void destroy() noexcept;

	// synchronous API to get, create and delete StateSets
	CadR::StateSet& getOrCreateStateSet(const ShaderState& shaderState, const PipelineState& pipelineState);
	CadR::StateSet* getStateSet(const ShaderState& shaderState, const PipelineState& pipelineState);
	void deleteStateSet(CadR::StateSet& ss) noexcept;

	// viewport and scissor for pipelines
	void setViewportAndScissor(const vk::Viewport& viewport, const vk::Rect2D& scissor);
	void setViewportAndScissor(const std::vector<vk::Viewport>& viewportList,
		const std::vector<vk::Rect2D>& scissorList);

	// getters
	CadR::VulkanDevice& device() const;
	PipelineLibrary& pipelineLibrary() const;
	ShaderLibrary& shaderLibrary() const;
	vk::PipelineCache pipelineCache() const;
	vk::PipelineLayout pipelineLayout() const;
	vk::DescriptorSetLayout descriptorSetLayout() const;
	std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList();
	const std::vector<vk::DescriptorSetLayout>* descriptorSetLayoutList() const;

};


}


// inline functions
#include <CadR/Renderer.h>
namespace CadPL {

inline PipelineSceneGraph::StateSetMapItem::StateSetMapItem(const ShaderState& shaderState_, const PipelineState& pipelineState_, CadR::Renderer& renderer)  : shaderState(shaderState_), pipelineState(pipelineState_), stateSet(renderer) {}
inline PipelineSceneGraph::StateSetMapItem& PipelineSceneGraph::stateSetToStateSetMapItem(CadR::StateSet& ss)  { return *reinterpret_cast<StateSetMapItem*>(reinterpret_cast<char*>(&ss) - reinterpret_cast<char*>(&static_cast<StateSetMapItem*>(nullptr)->stateSet)); }

inline PipelineSceneGraph::PipelineSceneGraph() noexcept  : _deleteLibraries(false) {}
inline PipelineSceneGraph::PipelineSceneGraph(nullptr_t, CadR::StateSet& root, const std::vector<std::bitset<ShaderState::numOptimizeFlags>>& optimizationLevels)  : _pipelineLibrary(nullptr), _shaderLibrary(nullptr), _deleteLibraries(true), _root(&root), _optimizationLevels(optimizationLevels) {}
inline PipelineSceneGraph::PipelineSceneGraph(CadR::StateSet& root, const std::vector<std::bitset<ShaderState::numOptimizeFlags>>& optimizationLevels, vk::PipelineCache pipelineCache, uint32_t maxTextures)  : PipelineSceneGraph(nullptr, root, optimizationLevels) { _shaderLibrary=new ShaderLibrary(root.renderer().device(), maxTextures); _pipelineLibrary=new PipelineLibrary(*_shaderLibrary, pipelineCache); }
inline PipelineSceneGraph::PipelineSceneGraph(PipelineLibrary& pipelineLibrary, ShaderLibrary& shaderLibrary, CadR::StateSet& root, const std::vector<std::bitset<ShaderState::numOptimizeFlags>>& optimizationLevels)  : _pipelineLibrary(&pipelineLibrary), _shaderLibrary(&shaderLibrary), _deleteLibraries(false), _root(&root), _optimizationLevels(optimizationLevels) {}
inline PipelineSceneGraph::~PipelineSceneGraph() noexcept  { _stateSetMap.clear_and_dispose([](StateSetMapItem* item){ delete item; }); if(_deleteLibraries) { delete _pipelineLibrary; delete _shaderLibrary; } }
inline void PipelineSceneGraph::init(CadR::StateSet& root, const std::vector<std::bitset<ShaderState::numOptimizeFlags>>& optimizationLevels, vk::PipelineCache pipelineCache, uint32_t maxTextures)  { destroy(); _deleteLibraries=true; _root=&root; _shaderLibrary=new ShaderLibrary(root.renderer().device()); _pipelineLibrary=new PipelineLibrary(*_shaderLibrary, pipelineCache); }
inline void PipelineSceneGraph::destroy() noexcept  { _stateSetMap.clear_and_dispose([](StateSetMapItem* item){ delete item; }); if(_deleteLibraries) { delete _pipelineLibrary; delete _shaderLibrary; _pipelineLibrary=nullptr; _shaderLibrary=nullptr; } }
inline CadR::StateSet& PipelineSceneGraph::getOrCreateStateSet(const ShaderState& shaderState, const PipelineState& pipelineState)  { decltype(_stateSetMap)::insert_commit_data insertData; auto [it, canInsert]=_stateSetMap.insert_check(std::tuple{shaderState, pipelineState}, insertData); return (canInsert) ? createStateSet(shaderState, pipelineState, insertData) : it->stateSet; }
inline CadR::StateSet* PipelineSceneGraph::getStateSet(const ShaderState& shaderState, const PipelineState& pipelineState)  { auto it=_stateSetMap.find(std::tuple{shaderState, pipelineState}); return (it!=_stateSetMap.end()) ? &it->stateSet : nullptr; }
inline void PipelineSceneGraph::deleteStateSet(CadR::StateSet& ss) noexcept  { _stateSetMap.erase_and_dispose(decltype(_stateSetMap)::s_iterator_to(stateSetToStateSetMapItem(ss)), [](StateSetMapItem* item){ delete item; }); }
inline void PipelineSceneGraph::setViewportAndScissor(const vk::Viewport& viewport, const vk::Rect2D& scissor)  { _pipelineLibrary->setViewportAndScissor(viewport, scissor); }
inline void PipelineSceneGraph::setViewportAndScissor(const std::vector<vk::Viewport>& viewportList, const std::vector<vk::Rect2D>& scissorList)  { _pipelineLibrary->setViewportAndScissor(viewportList, scissorList); }
inline CadR::VulkanDevice& PipelineSceneGraph::device() const  { return _pipelineLibrary->device(); }
inline PipelineLibrary& PipelineSceneGraph::pipelineLibrary() const  { return *_pipelineLibrary; }
inline ShaderLibrary& PipelineSceneGraph::shaderLibrary() const  { return *_shaderLibrary; }
inline vk::PipelineCache PipelineSceneGraph::pipelineCache() const  { return _pipelineLibrary->pipelineCache(); }
inline vk::PipelineLayout PipelineSceneGraph::pipelineLayout() const  { return _shaderLibrary->pipelineLayout(); }
inline vk::DescriptorSetLayout PipelineSceneGraph::descriptorSetLayout() const  { return _shaderLibrary->descriptorSetLayout(); }
inline std::vector<vk::DescriptorSetLayout>* PipelineSceneGraph::descriptorSetLayoutList()  { return _shaderLibrary->descriptorSetLayoutList(); }
inline const std::vector<vk::DescriptorSetLayout>* PipelineSceneGraph::descriptorSetLayoutList() const  { return _shaderLibrary->descriptorSetLayoutList(); }

}
