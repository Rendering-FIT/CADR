#pragma once

#include <list>
#include <memory>
#include <vector>
#include <CADR/Export.h>

namespace cd {

class Drawable;
class Scene;
class StateSet;
class Transformation;


/** Geode (GEOmetry noDE)
 */
class CADR_EXPORT Geode {
protected:
	size_t _drawCommandBegin;
	size_t _drawCommandEnd;
	std::vector<size_t> _drawCommandList;
	std::shared_ptr<Drawable> _drawable;
	std::shared_ptr<StateSet> _stateSet;
	std::list<std::shared_ptr<Transformation>> _transformationList;
public:

	static inline Geode* create(Scene* scene);
	static inline std::shared_ptr<Geode> make_shared(Scene* scene);
	inline Geode();
	virtual ~Geode() = default;

	inline size_t drawCommandBegin() const;
	inline size_t drawCommandEnd() const;
	inline const std::vector<size_t>& drawCommandList() const;
	inline const std::shared_ptr<Drawable>& drawable() const;
	inline const std::shared_ptr<StateSet>& stateSet() const;
	inline const std::list<std::shared_ptr<Transformation>> transformationList() const;

	inline void set(const std::shared_ptr<Drawable>& drawable,const std::shared_ptr<StateSet>& stateSet,
	                const std::shared_ptr<Transformation>& transformation);
	inline void set(std::shared_ptr<Drawable>&& drawable,std::shared_ptr<StateSet>&& stateSet,
	                std::shared_ptr<Transformation>&& transformation);
	inline void set(const std::shared_ptr<Drawable>& drawable,size_t drawCommandBegin,size_t drawCommandEnd,
	                std::vector<size_t>& drawCommandList,const std::shared_ptr<StateSet>& stateSet,
	                const std::shared_ptr<Transformation>& transformation);
	inline void set(std::shared_ptr<Drawable>&& drawable,size_t drawCommandBegin,size_t drawCommandEnd,
	                std::vector<size_t>&& drawCommandList,std::shared_ptr<StateSet>&& stateSet,
	                std::shared_ptr<Transformation>&& transformation);
	inline void set(const std::shared_ptr<Drawable>& drawable,size_t drawCommandBegin,size_t drawCommandEnd,
	                std::vector<size_t>& drawCommandList,const std::shared_ptr<StateSet>& stateSet,
	                const std::list<std::shared_ptr<Transformation>>& transformationList);
	virtual void set(std::shared_ptr<Drawable>&& drawable,size_t drawCommandBegin,size_t drawCommandEnd,
	                 std::vector<size_t>&& drawCommandList,std::shared_ptr<StateSet>&& stateSet,
	                 std::list<std::shared_ptr<Transformation>>&& transformationList);

};


}


// inline methods
#include <CADR/Factory.h>
namespace ri {

inline Geode* Geode::create(Scene* scene,Gui::ViewProviderElementInterface* vp)  { return Factory::get()->createGeode(scene,vp); }
inline std::shared_ptr<Geode> Geode::make_shared(Scene* scene,Gui::ViewProviderElementInterface* vp)  { return Factory::get()->makeGeode(scene,vp); }
inline Geode::Geode(Gui::ViewProviderElementInterface* vp) : _viewProvider(vp)  {}
inline size_t Geode::drawCommandBegin() const  { return _drawCommandBegin; }
inline size_t Geode::drawCommandEnd() const  { return _drawCommandEnd; }
inline const std::vector<size_t>& Geode::drawCommandList() const  { return _drawCommandList; }
inline const std::shared_ptr<Drawable>& Geode::drawable() const  { return _drawable; }
inline const std::shared_ptr<StateSet>& Geode::stateSet() const  { return _stateSet; }
inline const std::list<std::shared_ptr<Transformation>> Geode::transformationList() const  { return _transformationList; }
inline void Geode::set(const std::shared_ptr<Drawable>& drawable,const std::shared_ptr<StateSet>& stateSet,const std::shared_ptr<Transformation>& transformation)
	{ set(std::move(std::shared_ptr<Drawable>(drawable)),0,size_t(-1),std::vector<size_t>(),std::move(std::shared_ptr<StateSet>(stateSet)),std::move(std::list<std::shared_ptr<Transformation>>({transformation}))); }
inline void Geode::set(std::shared_ptr<Drawable>&& drawable,std::shared_ptr<StateSet>&& stateSet,std::shared_ptr<Transformation>&& transformation)
	{ set(std::move(drawable),0,size_t(-1),std::vector<size_t>(),std::move(stateSet),std::move(std::list<std::shared_ptr<Transformation>>({transformation}))); }
inline void Geode::set(const std::shared_ptr<Drawable>& drawable,size_t drawCommandBegin,size_t drawCommandEnd,std::vector<size_t>& drawCommandList,const std::shared_ptr<StateSet>& stateSet,const std::shared_ptr<Transformation>& transformation)
	{ set(std::move(std::shared_ptr<Drawable>(drawable)),drawCommandBegin,drawCommandEnd,std::move(std::vector<size_t>(drawCommandList)),std::move(std::shared_ptr<StateSet>(stateSet)),std::move(std::list<std::shared_ptr<Transformation>>({transformation}))); }
inline void Geode::set(std::shared_ptr<Drawable>&& drawable,size_t drawCommandBegin,size_t drawCommandEnd,std::vector<size_t>&& drawCommandList,std::shared_ptr<StateSet>&& stateSet,std::shared_ptr<Transformation>&& transformation)
	{ set(std::move(drawable),drawCommandBegin,drawCommandEnd,std::move(drawCommandList),std::move(stateSet),std::move(std::list<std::shared_ptr<Transformation>>({transformation}))); }
inline void Geode::set(const std::shared_ptr<Drawable>& drawable,size_t drawCommandBegin,size_t drawCommandEnd,std::vector<size_t>& drawCommandList,const std::shared_ptr<StateSet>& stateSet,const std::list<std::shared_ptr<Transformation>>& transformationList)
	{ set(std::move(std::shared_ptr<Drawable>(drawable)),drawCommandBegin,drawCommandEnd,std::move(std::vector<size_t>(drawCommandList)),std::move(std::shared_ptr<StateSet>(stateSet)),std::move(std::list<std::shared_ptr<Transformation>>(transformationList))); }

}
