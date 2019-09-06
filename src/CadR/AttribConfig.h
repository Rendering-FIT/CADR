#pragma once

#include <vector>

namespace CadR {


/** AttribConfig class describes attribute configuration.
 *
 *  Each Mesh uses particular attribute configuration.
 *  AttribConfig holds list of sizes of the attributes for a vertex. In other words,
 *  how much attribute offsets advance from one vertex to the following one.
 *
 *  For instance, a Mesh uses coordinates on the attribute index 0 using vector of three floats,
 *  and color attribute on the index 2 using vector of four unsigned bytes.
 *  Such Mesh uses AttribConfig { 12, 0, 4 }.
 *
 *  AttribConfig is used, for instance, by Mesh to select appropriate AttribStorage, as
 *  the AttribConfig of Mesh and of the AttribStorage must match.
 */
class AttribConfig { // header-only class, no CADR_EXPORT
protected:

	std::vector<uint8_t> _sizeList;  ///< List of sizes that is occupied by each particular attribute.

public:

	AttribConfig() = default;  ///< Constructs empty AttribConfig with zero attributes.
	AttribConfig(const std::vector<uint8_t>& sizeList);  ///< Constructs AttribConfig from attribute sizes (in bytes), e.g. amount consumed per-vertex. The parameter sizeList must not contain any trailing zeros at the end of list.
	AttribConfig(const std::vector<uint8_t>&& sizeList);  ///< Constructs AttribConfig from attribute sizes (in bytes), e.g. amount consumed per-vertex. The parameter sizeList must not contain any trailing zeros at the end of list.
	AttribConfig(std::initializer_list<uint8_t> init);  ///< Constructs AttribConfig from initializer list. The initializer_list must not contain any trailing zeros at the end of list.
	AttribConfig(const AttribConfig& ac) = default;  ///< Copy constructor.
	AttribConfig(AttribConfig&& ac) = default;  ///< Move constructor.

	const std::vector<uint8_t>& sizeList() const;
	uint8_t operator[](size_t attribIndex) const;
	uint8_t at(size_t attribIndex) const;
	unsigned numAttribs() const;

	AttribConfig& operator=(const AttribConfig& ac) = default;
	AttribConfig& operator=(AttribConfig&& ac) = default;
	bool operator==(const AttribConfig& rhs) const;
	bool operator!=(const AttribConfig& rhs) const;
	bool operator<=(const AttribConfig& rhs) const;
	bool operator>=(const AttribConfig& rhs) const;
	bool operator<(const AttribConfig& rhs) const;
	bool operator>(const AttribConfig& rhs) const;

};


}

// inline methods
#include <cassert>
namespace CadR {

inline AttribConfig::AttribConfig(const std::vector<uint8_t>& sizeList) : _sizeList(sizeList)  { assert(_sizeList.size()==0||_sizeList.back()!=0 || !"No trailing zeros allowed for sizeList."); }
inline AttribConfig::AttribConfig(const std::vector<uint8_t>&& sizeList) : _sizeList(move(sizeList))  { assert(_sizeList.size()==0||_sizeList.back()!=0 || !"No trailing zeros allowed for sizeList."); }
inline AttribConfig::AttribConfig(std::initializer_list<uint8_t> init) : _sizeList(init)  { assert(_sizeList.size()==0||_sizeList.back()!=0 || !"No trailing zeros allowed for sizeList."); }
inline const std::vector<uint8_t>& AttribConfig::sizeList() const  { return _sizeList; }
inline uint8_t AttribConfig::operator[](size_t attribIndex) const  { return _sizeList[attribIndex]; }
inline uint8_t AttribConfig::at(size_t attribIndex) const  { return _sizeList.at(attribIndex); }
inline unsigned AttribConfig::numAttribs() const  { return unsigned(_sizeList.size()); }

inline bool AttribConfig::operator==(const AttribConfig& rhs) const  { return _sizeList==rhs._sizeList; }
inline bool AttribConfig::operator!=(const AttribConfig& rhs) const  { return _sizeList!=rhs._sizeList; }
inline bool AttribConfig::operator<=(const AttribConfig& rhs) const  { return _sizeList<=rhs._sizeList; }
inline bool AttribConfig::operator>=(const AttribConfig& rhs) const  { return _sizeList>=rhs._sizeList; }
inline bool AttribConfig::operator<(const AttribConfig& rhs) const  { return _sizeList<rhs._sizeList; }
inline bool AttribConfig::operator>(const AttribConfig& rhs) const  { return _sizeList>rhs._sizeList; }

}
