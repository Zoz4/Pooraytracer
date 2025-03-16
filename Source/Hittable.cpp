#include "Hittable.h"
#include "Ray.h"

#include <glm/geometric.hpp>

namespace Pooraytracer {

	void HitRecord::SetFaceNormal(const Ray& ray, const vec3& outwordNormal)
	{
		// NOTE: the parameter `outwordNormal` is assumed to have unit length.
		bFrontFace = glm::dot(ray.direction, outwordNormal) < 0;
		normal = bFrontFace ? outwordNormal : -outwordNormal;
	}
}
