#pragma once

#include <CadR/Export.h>
#include <CadR/DrawCommand.h>
#include <boost/intrusive/list.hpp>

namespace CadR {

class MatrixList;
class StateSet;


class CADR_EXPORT Drawable {
public:
	boost::intrusive::list_member_hook<> drawableListHook;
	StateSet* stateSet;
	MatrixList* matrixList;
	DrawCommand drawCommandList[1];
};


typedef boost::intrusive::list<
	Drawable,
	boost::intrusive::member_hook<
		Drawable,
		boost::intrusive::list_member_hook<>,
		&Drawable::drawableListHook>
> DrawableList;


}
