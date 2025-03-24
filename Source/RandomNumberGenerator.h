#pragma once

#include <cstdlib>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

namespace Pooraytracer {

	const double Pi = 3.14159265358979323846;
	const double InvPi = 0.31830988618379067154;
	const double Inv2Pi = 0.15915494309189533577;
	const double PiOver2 = 1.57079632679489661923;
	const double PiOver4 = 0.78539816339744830961;

	inline double RandomDouble() {
		// Returns a random real in [0, 1).
		return std::rand() / (RAND_MAX + 1.0);
	}
	inline double RandomDouble(double min, double max) {
		// Returns a random real in [min,max).
		return min + (max - min) * RandomDouble();
	}
	inline int RandomInt(int min, int max) {
		// Returns a random integer in [min,max].
		return int(RandomDouble(min, max + 1));
	}
	inline glm::dvec3 RandomUnitVector3()
	{
		while (true)
		{
			glm::dvec3 p = glm::dvec3(RandomDouble(-1.0, 1.0), RandomDouble(-1.0, 1.0), RandomDouble(-1.0, 1.0));
			double lensq = glm::dot(p, p);
			if (1e-160 < lensq && lensq <= 1.) {
				return glm::normalize(p);
			}
		}
	}
	inline glm::dvec2 SampleUniformDiskConcentric(glm::dvec2 u)
	{
		glm::dvec2 uOffset = 2. * u - glm::dvec2(1., 1.);
		if (uOffset.x == 0. && uOffset.y == 0.)
		{
			return { 0., 0. };
		}
		double theta, r;
		if (std::fabs(uOffset.x) > std::fabs(uOffset.y)) {
			r = uOffset.x;
			theta = PiOver4 * (uOffset.y / uOffset.x);
		}
		else {
			r = uOffset.y;
			theta = PiOver2 - PiOver4 * (uOffset.x / uOffset.y);
		}
		return r * glm::dvec2(std::cos(theta), std::sin(theta));
	}
	inline glm::dvec3 SampleCosineHemisphere()
	{
		glm::dvec2 u = glm::dvec2(RandomDouble(), RandomDouble());
		glm::dvec2 d = SampleUniformDiskConcentric(u);
		double z = std::sqrt(std::max(0.0, 1. - d.x * d.x - d.y * d.y));

		return glm::dvec3(d.x, d.y, z);
	}
	inline glm::dvec2 SampleSquare() {
		// Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
		return glm::dvec2(RandomDouble() - 0.5, RandomDouble() - 0.5);
	}
}