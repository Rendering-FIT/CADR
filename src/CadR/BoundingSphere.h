#pragma once

#include <CadR/BoundingBox.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/norm.hpp>  // glm::distance2()
#include <algorithm>  // std::max()
#include <cmath>  // std::sqrt()

namespace CadR {


struct BoundingSphere {

	glm::vec3 center;
	float radius;

	BoundingBox getBoundingBox() const;

	void extendRadiusBy(BoundingSphere bs);  //< Extends radius of this BoudingSphere to contain BoundingSphere given by bs parameter.
	void extendRadiusBy(glm::vec3 point);  //< Extends BoundingSphere radius so it contains point given by argument. If you are processing many points, you might consider to use extendRadiusByPointUsingRadius2() to avoid expensive sqrt operation with each point.
	void extendRadiusByPointUsingRadius2(glm::vec3 point);  //< Extends BoundingSphere radius so it contains point given by argument. This is optimized version that goes around high computing cost of sqrt. Instead, it treats radius member as to contain square of radius (e.g. radius*radius), so high const of sqrt is avoided. A typical usage scenario is to convert initial radius to square of radius first. Initial radius is often zero. Then, to process number of points with extendRadiusByPointUsingRadius2() with the final radius stored as square of radius. Finally, to perform sqrt on BoundingSphere.radius to convert it back from square radius to radius. So, this optimized approach does sqrt only once instead on every point.

};

BoundingSphere operator*(const glm::mat4& transformationMatrix, const BoundingSphere bs);


// inline functions
inline BoundingBox BoundingSphere::getBoundingBox() const {
	return BoundingBox{
		.min = center - radius,
		.max = center + radius,
	};
}
inline void BoundingSphere::extendRadiusBy(BoundingSphere bs) {
	// (to avoid expensive sqrt we use square of distance (d2)
	// and square of radius difference (radiusDif*radiusDif);
	// sqrt is performed only if we need to update r)
	float d2 = glm::distance2(center, bs.center);
	float radiusDif = radius - bs.radius;
	if(radiusDif < 0.f || d2 > (radiusDif*radiusDif))
		radius = sqrt(d2) + bs.radius;
}
inline void BoundingSphere::extendRadiusBy(glm::vec3 point) {
	float d2 = sqrt(glm::distance2(point, center));
	if(d2 > radius)
		radius = d2;
}
inline void BoundingSphere::extendRadiusByPointUsingRadius2(glm::vec3 point) {
	float d2 = glm::distance2(point, center);
	if(d2 > radius)
		radius = d2;
}
inline BoundingSphere operator*(const glm::mat4& transformationMatrix, const BoundingSphere bs) {
	return
		BoundingSphere{
			.center = glm::mat3(transformationMatrix) * bs.center + glm::vec3(transformationMatrix[3]),
			.radius =
				[](const glm::vec3 v) -> float {
					return std::max(std::max(v.x, v.y), v.z);
				}(glm::mat3(transformationMatrix) * glm::vec3(bs.radius))
		};
}

}
