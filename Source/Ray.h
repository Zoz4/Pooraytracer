#pragma once

#include <glm/vec3.hpp>

namespace Pooraytracer {
	using glm::vec3;
	class Ray {
	public:
		Ray() = default;
		Ray(const vec3& origin, const vec3& direction) :
			origin(origin), direction(direction) {
		}
		vec3 operator()(float t) const { return origin + direction * t; }
	public:
		vec3 origin;
		vec3 direction;
	};
}