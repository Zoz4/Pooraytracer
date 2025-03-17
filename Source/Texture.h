#pragma once

#include <glm/vec3.hpp>

namespace Pooraytracer {

	using color = glm::vec3;
	using point3 = glm::vec3;

	class Texture {
	public:
		virtual ~Texture() = default;

		virtual color Value(float u, float v, const point3& p) const = 0;
	};

	class SolidColor : public Texture {
	public:

		SolidColor(const color& albedo) :albedo(albedo) {}
		color Value(float u, float v, const point3& p) const override {
			return albedo;
		}
	private:
		color albedo;
	};
}