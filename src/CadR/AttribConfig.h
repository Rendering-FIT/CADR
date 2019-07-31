#pragma once

#include <vector>

namespace cd {


/** AttribConfig class represents particular attribute configuration.
 *
 *  Each Drawable uses particular attribute configuration. For instance,
 *  it uses coordinates on the attribute index 0 using vector of three floats,
 *  color attribute on the index 2 using vector of four unsigned bytes,
 *  and indexed rendering. AttribConfig holds indexed flag and list of sizes.
 *  The size on index i represents space occupied by single vertex inside attribute i,
 *  e.g. vector of three floats will use value of 12.
 */
class AttribConfig { // header-only class, no RI_EXPORT
protected:

	std::vector<uint8_t> _sizeList;  ///< List of sizes that is occupied by each particular attribute.
	bool _indexed;                   ///< True if index buffer and indexed rendering are used.

public:

	inline AttribConfig();
	inline AttribConfig(const std::vector<uint8_t>& sizeList,bool indexed);  ///< Construct AttribConfig. The parameter sizeList must not contain any trailing zeros at the end of list.
	inline AttribConfig(const AttribConfig& ac) = default;
	inline AttribConfig(AttribConfig&& ac) = default;

	inline const std::vector<uint8_t>& sizeList() const;
	inline unsigned numAttribs() const;
	inline bool indexed() const;

	inline AttribConfig& operator=(const AttribConfig& ac) = default;
	inline AttribConfig& operator=(AttribConfig&& ac) = default;
	inline bool operator==(const AttribConfig& rhs) const;
	inline bool operator!=(const AttribConfig& rhs) const;
	inline bool operator<=(const AttribConfig& rhs) const;
	inline bool operator>=(const AttribConfig& rhs) const;
	inline bool operator<(const AttribConfig& rhs) const;
	inline bool operator>(const AttribConfig& rhs) const;

};


}

// inline methods
#include <cassert>
namespace cd {

inline AttribConfig::AttribConfig()  {}
inline AttribConfig::AttribConfig(const std::vector<uint8_t>& sizeList,bool indexed) : _sizeList(sizeList), _indexed(indexed)  { assert(sizeList.size()==0||sizeList.back()!=0 || !"No trailing zeros allowed for sizeList."); }
inline const std::vector<uint8_t>& AttribConfig::sizeList() const  { return _sizeList; }
inline unsigned AttribConfig::numAttribs() const  { return unsigned(_sizeList.size()); }
inline bool AttribConfig::indexed() const  { return _indexed; }

inline bool AttribConfig::operator==(const AttribConfig& rhs) const  { return _sizeList==rhs._sizeList && _indexed==rhs._indexed; }
inline bool AttribConfig::operator!=(const AttribConfig& rhs) const  { return _sizeList!=rhs._sizeList || _indexed!=rhs._indexed; }
inline bool AttribConfig::operator<=(const AttribConfig& rhs) const  { return (_sizeList>rhs._sizeList)?false:_indexed<=rhs._indexed; }
inline bool AttribConfig::operator>=(const AttribConfig& rhs) const  { return (_sizeList<rhs._sizeList)?false:_indexed>=rhs._indexed; }
inline bool AttribConfig::operator<(const AttribConfig& rhs) const  { return (_sizeList>rhs._sizeList)?false:_indexed<rhs._indexed; }
inline bool AttribConfig::operator>(const AttribConfig& rhs) const  { return (_sizeList<rhs._sizeList)?false:_indexed>rhs._indexed; }

}
