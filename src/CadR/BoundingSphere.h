// SPDX-FileCopyrightText: 2024-2026 PCJohn (Jan Pečiva, peciva@fit.vut.cz)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <CadR/BoundingBox.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/norm.hpp>  // glm::distance2()
#include <algorithm>  // std::max()
#include <cmath>  // std::sqrt()
#include <limits>

namespace CadR {


struct BoundingSphere {

	glm::vec3 center;
	float radius;

	static BoundingSphere empty();
	void makeEmpty();
	bool isEmpty() const;

	BoundingBox getBoundingBox() const;

	void extendRadiusBy(BoundingSphere bs);  //< Extends radius of this BoudingSphere to contain BoundingSphere given by bs parameter.
	void extendRadiusBy(glm::vec3 point);  //< Extends BoundingSphere radius so it contains point given by argument. If you are processing many points, you might consider to use extendRadiusByPointUsingRadius2() to avoid expensive sqrt operation with each point.
	void extendRadiusByPointUsingRadius2(glm::vec3 point);  //< Extends BoundingSphere radius so it contains point given by argument. This is optimized version that goes around high computing cost of sqrt. Instead, it treats radius member as to contain square of radius (e.g. radius*radius), so high const of sqrt is avoided. A typical usage scenario is to convert initial radius to square of radius first. Initial radius is often zero. Then, to process number of points with extendRadiusByPointUsingRadius2() with the final radius stored as square of radius. Finally, to perform sqrt on BoundingSphere.radius to convert it back from square radius to radius. So, this optimized approach does sqrt only once instead on every point.

};

BoundingSphere operator*(const glm::mat4& transformationMatrix, const BoundingSphere bs);


// inline functions
inline BoundingSphere BoundingSphere::empty() {
	return BoundingSphere { .center = { 0.f, 0.f, 0.f }, .radius = -std::numeric_limits<float>::infinity() };
}
inline void BoundingSphere::makeEmpty()  { *this = empty(); }
inline bool BoundingSphere::isEmpty() const  { return radius == -std::numeric_limits<float>::infinity(); }
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
	// avoid expensive sqrt and compute it only when really updating radius
	float d2 = glm::distance2(point, center);
	if(d2 > radius*radius)
		radius = sqrt(d2);
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
				[&]() -> float {
					auto sqrLength =
						[](const float x, const float y, const float z) -> float {
							return x*x + y*y + z*z;
						};
					float maxSqrScale = std::max(std::max(
						sqrLength(transformationMatrix[0][0], transformationMatrix[0][1], transformationMatrix[0][2]),
						sqrLength(transformationMatrix[1][0], transformationMatrix[1][1], transformationMatrix[1][2])),
						sqrLength(transformationMatrix[2][0], transformationMatrix[2][1], transformationMatrix[2][2]));
					return sqrt(maxSqrScale) * bs.radius;
				}()
		};
}

}
