#pragma once


#include "Ray.h"
#include "Hittable.h"
#include "Texture.h"
#include "RandomNumberGenerator.h"

#include <glm/vec3.hpp>

namespace Pooraytracer {

	using glm::vec3;
	using color = glm::vec3;
	using point3 = glm::vec3;

	class Material {
	public:
		virtual ~Material() = default;
		virtual color Emmited(float u, float v, const point3& p) const {
			return color(0.f, 0.f, 0.f);
		}
		virtual bool Scatter(const Ray& rayIn, const HitRecord& record, color& attenuation, Ray& scatteredRay) const {
			return false;
		}

	};

	class Lambertian : public Material {
	public:
		Lambertian(const color& albedo) :texture(make_shared<SolidColor>(albedo)) {}
		Lambertian(shared_ptr<Texture> texture) :texture(texture) {}

		bool Scatter(const Ray& rayIn, const HitRecord& record, color& attenuation, Ray& scatteredRay)
			const override {
			auto scatter_direction = record.normal + RandomUnitVector3();

			// Catch degenerate scatter direction
			float s = 1e-8;
			if (std::fabs(scatter_direction.x) < s && std::fabs(scatter_direction.y) < s && std::fabs(scatter_direction.z) < s)
				scatter_direction = record.normal;

			scatteredRay = Ray(record.position, scatter_direction);
			attenuation = texture->Value(record.uv[0], record.uv[1], record.position);

			return true;
		}
	private:
		shared_ptr<Texture> texture;
	};
}