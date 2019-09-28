#pragma once

#include <vector>

namespace CadR {


/** AttribSizeList class holds information about number of bytes per vertex that each attribute uses.
 *  The size of AttribSizeList indicates number of attributes used by the vertex.
 *
 *  AttribSizeList is used, for instance, by Mesh to select appropriate AttribStorage.
 *  AttribSizeList of the Mesh and AttribSizeList of the AttribStorage must match,
 *  otherwise the Mesh attributes can not be stored in the AttribStorage.
 */
class AttribSizeList : public std::vector<uint8_t> { // header-only class, no CADR_EXPORT
	typedef std::vector<uint8_t> inherited;
public:
	using inherited::inherited;
};


}
