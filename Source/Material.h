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

	class PhoneReflectance : public Material {
	public:
		PhoneReflectance(const color& Kd, const color& Ks, float Ns) :
			Kd(make_shared<SolidColor>(Kd)), Ks(make_shared<SolidColor>(Ks)), Ns(Ns) {
			color kdks = Kd + Ks;
			float invDenom = 1.0f/(kdks.r + kdks.g + kdks.b);
			pkd = (Kd.r + Kd.g + Kd.b) * invDenom;
			pks = (Ks.r + Ks.g + Ks.b) * invDenom;
		}
		PhoneReflectance(shared_ptr<Texture> mapKd, const color& Ks, float Ns) :
			Kd(mapKd), Ks(make_shared<SolidColor>(Ks)), Ns(Ns), pkd(0.5), pks(0.5) { }

		vec3 SampleDiffuse(const MaterialEvalContext& context) const
		{
			float u1 = RandomFloat(), u2 = RandomFloat();
			float theta = glm::acos(glm::sqrt(u1));
			float phi = 2.0f * Pi * u2;
			float sinTheta = glm::sin(theta), cosTheta = glm::cos(theta),
				  sinPhi = glm::sin(phi), cosPhi = glm::cos(phi);

			vec3 wi = vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);

			return LocalToWorld(wi, context);
		}
		float DiffusePDF(const vec3& wi, const MaterialEvalContext& context) const
		{
			float cosTheta = glm::dot(wi, context.n);
			return cosTheta * InvPi;
		}
		
		vec3 SampleSpecular(const MaterialEvalContext& context) const
		{
			float u1 = RandomFloat(), u2 = RandomFloat();
			float alpha = glm::acos(glm::pow(u1, 1.0/(Ns+1.0)));
			float phi = 2.0f * Pi * u2;
			float sinAlpha = glm::sin(alpha), cosAlpha = glm::cos(alpha),
				sinPhi = glm::sin(phi), cosPhi = glm::cos(phi);

			vec3 wi = vec3(sinAlpha * cosPhi, sinAlpha * sinPhi, cosAlpha);

			return ReflectiveDirectionLocalToWorld(wi, context);
		}
		vec3 ReflectiveDirectionLocalToWorld(const vec3& local, const MaterialEvalContext& context) const
		{
			// Build Orthonormal Basis R(reflection)->N
			vec3 R = glm::normalize(Reflect(context.wo, context.n));
			vec3 V = (std::fabs(R.x) > 0.9f ? vec3(0, 1, 0) : vec3(1, 0, 0));

			vec3 T = glm::normalize(glm::cross(V, R));
			vec3 B = glm::cross(R, T);

			return local.x * T + local.y * B + local.z * R;
		}
		float SpecularPDF(const vec3& wi, const MaterialEvalContext& context) const
		{
			vec3 R = glm::normalize(Reflect(context.wo, context.n));
			float cosAlpha = glm::dot(wi, R);
			return (Ns+1.0) * Inv2Pi * glm::pow(cosAlpha, Ns);
		}

		bool Scatter(const Ray& rayIn, const HitRecord& record, color& attenuation, Ray& scatteredRay)
			const override {

			MaterialEvalContext context;
			context.n = record.normal;
			context.wo = -rayIn.direction;
			context.dpdus = record.tangent;

			float u = RandomFloat();

			// attenuation = f_r * cos{θ_i} / pdf

			if (u < pkd) // Sample Diffuse
			{
				vec3 wi = SampleDiffuse(context);
				scatteredRay = Ray(record.position, wi);
				// f_r_diffuse = kd/pi
				float cosTheta = glm::dot(wi, context.n);
				attenuation = Kd->Value(record.uv[0], record.uv[1], record.position)*InvPi * cosTheta / DiffusePDF(wi, context);
			}
			else if (pkd <= u && u < pkd + pks) // Sample Specular
			{
				vec3 wi = SampleSpecular(context);
				scatteredRay = Ray(record.position, wi);
				// f_r_specular = ks*(Ns+2)/(2*Pi)*(cosα)^n * cos{θ_i}
				vec3 reflect = glm::normalize(Reflect(context.wo, context.n));
				float cosTheta = glm::dot(wi, context.n);
				float cosAlpha = glm::dot(wi, reflect);
				attenuation = Ks->Value(record.uv[0], record.uv[1], record.position) * (Ns + 2) * Inv2Pi * glm::pow(cosAlpha, Ns) * cosTheta / SpecularPDF(wi, context);
			}
			else {
				return false;
			}
			
			return true;
		}

		vec3 Reflect(const vec3 &wo, const vec3 &n) const {
			return -wo + 2 * dot(wo, n) * n;
		}

	private:
		shared_ptr<Texture> Kd;
		shared_ptr<Texture> Ks;
		float Ns;
		float pkd, pks; 
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