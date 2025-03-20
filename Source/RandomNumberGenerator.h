#pragma once

#include<cstdlib>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

namespace Pooraytracer {

	const float InvPi = 0.31830988618379067154;
	const float Pi = 3.14159265358979323846;
	const float PiOver2 = 1.57079632679489661923;
	const float PiOver4 = 0.78539816339744830961;

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
			glm::vec3 p = glm::vec3(RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f));
			float lensq = glm::dot(p, p);
			if (1e-160 < lensq && lensq <= 1.f) {
				return glm::normalize(p);
			}
		}
	}
	inline glm::vec2 SampleUniformDiskConcentric(glm::vec2 u)
	{
		glm::vec2 uOffset = 2.f * u - glm::vec2(1.f, 1.f);
		if (uOffset.x == 0.f && uOffset.y == 0)
		{
			return { 0.f, 0.f };
		}

		float theta, r;
		if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
			r = uOffset.x;
			theta = PiOver4 * (uOffset.y / uOffset.x);
		}
		else {
			r = uOffset.y;
			theta = PiOver2 - PiOver4 * (uOffset.x / uOffset.y);
		}
		return r * glm::vec2(std::cos(theta), std::sin(theta));
	}
	inline glm::vec3 SampleCosineHemisphere()
	{
		glm::vec2 u = glm::vec2(RandomFloat(), RandomFloat());
		glm::vec2 d = SampleUniformDiskConcentric(u);
		float z = std::sqrt(std::max(0.0f, 1 - d.x * d.x - d.y * d.y));

		return glm::vec3(d.x, d.y, z);

		//auto r1 = RandomFloat();
		//auto r2 = RandomFloat();

		//auto phi = 2 * Pi * r1;
		//auto x = std::cos(phi) * std::sqrt(r2);
		//auto y = std::sin(phi) * std::sqrt(r2);
		//auto z = std::sqrt(1 - r2);

		//return glm::vec3(x, y, z);
	}

	inline glm::vec2 SampleSquare() {
		// Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
		return glm::vec2(RandomFloat() - 0.5, RandomFloat() - 0.5);
	}
}