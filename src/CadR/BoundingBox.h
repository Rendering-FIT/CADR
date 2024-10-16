#pragma once

#include <glm/vec3.hpp>
#include <limits>

namespace CadR {


struct BoundingBox {
	glm::vec3 min;
	glm::vec3 max;

	glm::vec3 getCenter() const;
	void setByCenterAndHalfExtents(glm::vec3 center, glm::vec3 halfExtents);

	static BoundingBox createByCenterAndHalfExtents(glm::vec3 center, glm::vec3 halfExtents);
	static BoundingBox empty();
	void makeEmpty();
	void extendBy(const BoundingBox& bb);
};


// inline functions
inline glm::vec3 BoundingBox::getCenter() const  { return (min + max) / 2.f; }
inline void BoundingBox::setByCenterAndHalfExtents(glm::vec3 center, glm::vec3 halfExtents) {
	min = center - halfExtents;
	max = center + halfExtents;
}
inline BoundingBox BoundingBox::createByCenterAndHalfExtents(glm::vec3 center, glm::vec3 halfExtents) {
	return BoundingBox{
		.min = center - halfExtents,
		.max = center + halfExtents,
	};
}
inline BoundingBox BoundingBox::empty() {
	return {
		{ +std::numeric_limits<float>::infinity(), +std::numeric_limits<float>::infinity(), +std::numeric_limits<float>::infinity() },
		{ -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() },
	};
}
inline void BoundingBox::makeEmpty()  { *this = empty(); }
inline void BoundingBox::extendBy(const BoundingBox& bb) {
	if(bb.min.x < min.x)
		min.x = bb.min.x;
	if(bb.min.y < min.y)
		min.y = bb.min.y;
	if(bb.min.z < min.z)
		min.z = bb.min.z;
	if(bb.max.x > max.x)
		max.x = bb.max.x;
	if(bb.max.y > max.y)
		max.y = bb.max.y;
	if(bb.max.z > max.z)
		max.z = bb.max.z;
}

}
