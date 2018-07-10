#pragma once

#include <memory>
#include <CADR/Export.h>

namespace cd {

//class RenderingWindow;
class Renderer;
class Drawable;
class Geode;
//class MarkerSet;
//class StateSet;
//class Transformation;


class CADR_EXPORT Factory {
public:

	static const std::shared_ptr<Factory>& get();
	static void set(const std::shared_ptr<Factory>& f);

	virtual ~Factory() {}

	//virtual RenderingWindow* createRenderingWindow(QWidget* parent,Gui::Viewer* viewer) = 0;
	//virtual Scene* createScene(Gui::GuiDocument* guiDoc) = 0;
	virtual Drawable* createDrawable(Renderer* renderer) = 0;
	virtual Geode* createGeode(Renderer* renderer) = 0;
	//virtual MarkerSet* createMarkerSet(Scene* scene,Gui::ViewProviderElementInterface* vp) = 0;
	//virtual StateSet* createStateSet(Scene* scene) = 0;
	//virtual Transformation* createTransformation(Scene* scene) = 0;
	//virtual std::shared_ptr<RenderingWindow> makeRenderingWindow(QWidget* parent,Gui::Viewer* viewer) = 0;
	//virtual std::shared_ptr<Scene> makeScene(Gui::GuiDocument* guiDoc) = 0;
	virtual std::shared_ptr<Drawable> makeDrawable(Renderer* renderer) = 0;
	virtual std::shared_ptr<Geode> makeGeode(Renderer* renderer) = 0;
	//virtual std::shared_ptr<MarkerSet> makeMarkerSet(Scene* scene,Gui::ViewProviderElementInterface* vp) = 0;
	//virtual std::shared_ptr<StateSet> makeStateSet(Scene* scene) = 0;
	//virtual std::shared_ptr<Transformation> makeTransformation(Scene* scene) = 0;

};


}
