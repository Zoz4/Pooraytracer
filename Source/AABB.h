#pragma once

#include "Interval.h"
#include <glm/vec3.hpp>

namespace Pooraytracer {

	class Ray;
	using vec3 = glm::dvec3;

	// Axis-Aligned Bounding Box
	class AABB {

	public:
		Interval x, y, z;
		AABB() = default;
		AABB(const Interval& x, const Interval& y, const Interval& z);
		AABB(const vec3& a, const vec3& b);
		AABB(const AABB& box0, const AABB& box1);

		const Interval& GetAxisInterval(int axis) const;
		bool Hit(const Ray& ray, Interval t) const;
		int LongestAxis() const;

		static const AABB empty, universe;

	private:
		void PadToMinimus();
	};

	AABB operator+(const AABB& bbox, const vec3& offset);
	AABB operator+(const vec3& offset, const AABB& bbox);
}