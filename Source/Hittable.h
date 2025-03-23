#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include "AABB.h"

namespace Pooraytracer {

	using vec3 = glm::dvec3;
	using vec2 = glm::dvec2;

	class Ray;
	class Material;

	class HitRecord {
	public:
		vec3 position;
		double time;
		vec3 normal; // in the same side with the ray
		vec3 tangent;
		vec2 uv;
		std::shared_ptr<Material> material; // [TODO]
		bool bFrontFace;

		void SetFaceNormal(const Ray& ray, const vec3& outwordNormal);
	};


	class Hittable {
	public:
		virtual ~Hittable() = default;
		virtual bool Hit(const Ray& ray, Interval domain, HitRecord& record) const = 0;
		virtual AABB BoundingBox() const = 0;
	};
}