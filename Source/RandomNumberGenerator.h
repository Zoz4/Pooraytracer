#pragma once

#include<cstdlib>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

namespace Pooraytracer {

	inline float RandomFloat() {
		// Returns a random real in [0, 1).
		return std::rand() / (RAND_MAX + 1.0f);
	}
	inline float RandomFloat(float min, float max) {
		// Returns a random real in [min,max).
		return min + (max - min) * RandomFloat();
	}
	inline int RandomInt(int min, int max) {
		// Returns a random integer in [min,max].
		return int(RandomFloat(min, max + 1));
	}
	
	inline glm::vec3 RandomUnitVector3()
	{
		while (true)
		{
			glm::vec3 p = glm::vec3(RandomFloat(), RandomFloat(), RandomFloat());
			float lensq = glm::dot(p, p);
			if (1e-160 < lensq && lensq <= 1.f) {
				return glm::normalize(p);
			}
		}
	}

}