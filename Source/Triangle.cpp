#include "Triangle.h"
#include "Material.h"
#include "Ray.h"
#include <glm/geometric.hpp>

namespace Pooraytracer {

	using glm::normalize, glm::cross, glm::length, glm::dot;

	Triangle::Triangle(const std::array<vec3, 3>& vertices, const std::array<vec2, 3> texCoords, std::shared_ptr<Material> material) :
		vertices(vertices), texCoords(texCoords), material(material)
	{
		edges[0] = vertices[1] - vertices[0];
		edges[1] = vertices[2] - vertices[0];

		vec3 n = cross(edges[0], edges[1]);

		normal = normalize(n);

		vec2 deltaUV0 = texCoords[1] - texCoords[0];
		vec2 deltaUV1 = texCoords[2] - texCoords[0];
		float f = 1.0f / (deltaUV0.x * deltaUV1.y - deltaUV1.x * deltaUV0.y);
		tangent.x = f * (deltaUV1.y * edges[0].x - deltaUV0.y * edges[1].x);
		tangent.y = f * (deltaUV1.y * edges[0].y - deltaUV0.y * edges[1].y);
		tangent.z = f * (deltaUV1.y * edges[0].z - deltaUV0.y * edges[1].z);
		tangent = glm::normalize(tangent);

		area = length(n) * 0.5f;

		D = dot(normal, vertices[0]);
		w = n / dot(n, n);
		SetBoundingBox();

	}
	bool Triangle::Hit(const Ray& ray, Interval domain, HitRecord& record) const
	{
		float denom = dot(normal, ray.direction);

		// No hit if the ray is parallel to the plane.
		if (std::fabs(denom) < 1e-8) {
			return false;
		}
		// Return false if the hit point parameter t is outside the ray interval.
		float t = (D - dot(normal, ray.origin)) / denom;
		if (!domain.Contains(t)) {
			return false;
		}
		// Determine if the hit point lies within the planar shape using its plane coordinates.
		vec3 p = ray(t);
		vec3 v0p = p - vertices[0];
		float alpha = dot(w, cross(v0p, edges[1])); // barycentric of v1
		float beta = dot(w, cross(edges[0], v0p));	// barycentric of v2
		if (!IsInterior(alpha, beta, record)) {
			return false;
		}

		record.position = p;
		record.time = t;
		record.material = material;
		record.tangent = tangent;
		record.SetFaceNormal(ray, normal);

		return true;
	}
	void Triangle::SetBoundingBox()
	{
		AABB bboxEdge0 = AABB(vertices[0], vertices[1]);
		AABB bboxEdge1 = AABB(vertices[0], vertices[2]);
		bbox = AABB(bboxEdge0, bboxEdge1);
	}
	bool Triangle::IsInterior(float alpha, float beta, HitRecord& record) const
	{
		if ((alpha < 0) || (beta < 0) || (alpha + beta > 1))
			return false;

		record.uv = (1.f - alpha - beta) * texCoords[0] + alpha * texCoords[1] + beta * texCoords[2];
		return true;
	}

	Mesh::Mesh(const std::string& name, const std::vector<std::shared_ptr<Hittable>>& triangles, shared_ptr<Material> material) :name(name), material(material)
	{
		for (const auto& primitive : triangles)
		{
			Add(primitive);
		}
	}
}