#pragma once


#include "Ray.h"
#include "Hittable.h"
#include "Texture.h"
#include "RandomNumberGenerator.h"
#include "Logger.h"

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/geometric.hpp>

namespace Pooraytracer {

	using glm::vec3;
	using color = glm::vec3;
	using point3 = glm::vec3;

	class MaterialEvalContext {
	public:
		vec3 wo;	// point->eye: world space
		vec3 n;		// normal
		vec3 dpdus; // tangent
	};

	enum class MaterialType
	{
		Lambertian, PhoneReflectance, DiffuseLight, DebugMaterial
	};
	class Material {
	public:
		virtual ~Material() = default;
		virtual color Emmited(float u, float v, const point3& p) const {
			return color(0.f, 0.f, 0.f);
		}
		virtual bool Scatter(const Ray& rayIn, const HitRecord& record, color& attenuation, Ray& scatteredRay) const {
			return false;
		}
		virtual vec3 Sample(const MaterialEvalContext& context) const {
			return vec3(0.f, 0.f, 0.f);
		};
		virtual vec3 Eval(const vec3& wi, const MaterialEvalContext& context) const {
			return vec3(0.f, 0.f, 0.f);
		};
		virtual float PDF(const vec3& wi, const MaterialEvalContext& context) const {
			return 1.0f;
		}
		virtual bool HasEmission() const { return false; }
		virtual color GetEmission() const { return color(0.f, 0.f, 0.f); }

	protected:
		vec3 LocalToWorld(const vec3 &local, const MaterialEvalContext& context) const
		{
			const vec3& normal = context.n;
			const vec3& tangent = context.dpdus;
			vec3 bitangent = glm::cross(tangent, normal);

			return glm::normalize(local.x * tangent + local.y * bitangent + local.z * normal );
		}
	};

	class Lambertian : public Material {
	public:
		Lambertian(const color& albedo) :texture(make_shared<SolidColor>(albedo)) {}
		Lambertian(shared_ptr<Texture> texture) :texture(texture) {}

		vec3 Sample(const MaterialEvalContext& context) const override
		{
			vec3 wi = SampleCosineHemisphere();
			return LocalToWorld(wi, context);
		}
		float PDF(const vec3& wi, const MaterialEvalContext& context) const override
		{
			float cosTheta = glm::dot(wi, context.n);
			return cosTheta * InvPi;
		}
		vec3 Eval(const vec3& wi, const MaterialEvalContext& context) const override {
			return texture->Value(0,0,vec3(0)) * InvPi; // albedo / pi
		}

		bool Scatter(const Ray& rayIn, const HitRecord& record, color& attenuation, Ray& scatteredRay)
			const override {

			MaterialEvalContext context;
			context.n = record.normal;
			context.wo = -rayIn.direction;
			context.dpdus = record.tangent;

			vec3 wi = Sample(context);
			scatteredRay = Ray(record.position, wi);
			// attenuation = f_r * cos{θ_i} / pdf
			float cosTheta = glm::dot(wi, context.n);
			attenuation = Eval(wi, context)*cosTheta/PDF(wi, context);

			return true;
		}

	private:
		shared_ptr<Texture> texture;
	};

	class DiffuseLight : public Material {
	public:

		DiffuseLight(shared_ptr<Texture> texture) :texture(texture) {}
		DiffuseLight(const color& emit) :texture(make_shared<SolidColor>(emit)) {}
	
		color Emmited(float u, float v, const point3& p) const override {
			return texture->Value(u, v, p);
		}
		bool HasEmission() const override { return true; }
		color GetEmission() const override { return Emmited(0.f, 0.f, point3( 0.f )); }
	private:
		shared_ptr<Texture> texture;
	};

	class DebugMaterial : public Material {
	public:

		DebugMaterial(shared_ptr<Texture> texture) :texture(texture) {}
		DebugMaterial(const color& albedo) :texture(make_shared<SolidColor>(albedo)) {}
		bool HasEmission() const override { return true; }
		color GetEmission() const override { return Emmited(0.f, 0.f, point3(0.f)); }
		color Emmited(float u, float v, const point3& p) const override {
			return texture->Value(u, v, p);
		}
	private:
		shared_ptr<Texture> texture;
	};

}