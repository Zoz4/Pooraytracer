#pragma once

#include <array>
#include <string>
#include "Hittable.h"
#include "HittableList.h"

namespace Pooraytracer {

	class Triangle :public Hittable {

	public:
		Triangle(const std::array<vec3, 3>& vertices, const std::array<vec2, 3> texCoords, std::shared_ptr<Material> material);
		bool Hit(const Ray& ray, Interval domain, HitRecord& record) const override;
		AABB BoundingBox() const override { return bbox; };

	public:
		std::array<vec3, 3> vertices; // vertices v0, v1, v2, right-handed coordinate system
		std::array<vec3, 2> edges;	  // e0: v1-v0, e1: v2-v0 
		std::array<vec2, 3> texCoords;// texture coords
		vec3 normal;
		float area;
		AABB bbox;
		std::shared_ptr<Material> material; // [TODO]

	private:
		float D;
		vec3 w;
		void SetBoundingBox();
		bool IsInterior(float alpha, float beta, HitRecord& record) const;
	};

	class Mesh : public HittableList {
	public:
		Mesh() = default;
		Mesh(const std::string& name, const std::vector<std::shared_ptr<Hittable>>& triangles, shared_ptr<Material> material);
	public:
		std::string name;
		std::shared_ptr<Material> material;
	};
}