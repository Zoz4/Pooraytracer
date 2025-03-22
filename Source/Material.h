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

	struct MaterialEvalContext {
	public:
		vec3 p;     // intersection point
		vec2 uv;    // texcoords of hit point

		vec3 wo;	// point->eye: local space
		vec3 n;		// world space normal
		vec3 dpdus; // world space tangent
	};

	enum class SampleFlags : unsigned int
	{
		Unset = 0,
		Diffuse = 1 << 0,
		Specular = 1 << 1
	};

	struct MaterialSampleContext {
	public:
		vec3 wi;		// local space wi
		vec3 f;			// brdf
		float pdf;
		color radiance; // for light sampler
		SampleFlags flags;
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
		virtual MaterialSampleContext Sample(const MaterialEvalContext& context) const {
			return MaterialSampleContext{};
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
		vec3 WorldToLocal(const vec3& world, const HitRecord& record) const
		{
			const vec3& normal = record.normal;
			const vec3& tangent = record.tangent;
			vec3 bitangent = glm::cross(tangent, normal);

			float x = glm::dot(world, tangent);
			float y = glm::dot(world, bitangent);
			float z = glm::dot(world, normal);

			return vec3(x, y, z);
		}
	};

	class Lambertian : public Material {
	public:
		Lambertian(const color& albedo) :texture(make_shared<SolidColor>(albedo)) {}
		Lambertian(shared_ptr<Texture> texture) :texture(texture) {}

		MaterialSampleContext Sample(const MaterialEvalContext& context) const override
		{
			MaterialSampleContext sampleContext{};
			// wi: local space, (sinθcosφ，sinθsinφ, cosθ)
			vec3 wi = SampleCosineHemisphere();
			sampleContext.wi = wi;
			sampleContext.pdf = PDF(wi, context);
			sampleContext.f = Eval(wi, context);
			return sampleContext;
		}
		float PDF(const vec3& wi, const MaterialEvalContext& context) const override
		{   
			float cosTheta = wi.z;
			return cosTheta * InvPi;
		}
		vec3 Eval(const vec3& wi, const MaterialEvalContext& context) const override {
			return texture->Value(context.uv[0], context.uv[1], context.p) * InvPi; // albedo / pi
		}
		bool Scatter(const Ray& rayIn, const HitRecord& record, color& attenuation, Ray& scatteredRay)
			const override {

			MaterialEvalContext context;
			context.p = record.position;
			context.uv = record.uv;
			context.n = record.normal;
			context.wo = WorldToLocal(-rayIn.direction, record);
			context.dpdus = record.tangent;
		
			MaterialSampleContext sampleContext = Sample(context);
			const vec3& wi = sampleContext.wi;
			const float cosTheta = sampleContext.wi.z;
			const vec3& fr = sampleContext.f;
			const float& pdf = sampleContext.pdf;

			scatteredRay = Ray(record.position, LocalToWorld(wi, context));
			attenuation = fr*cosTheta/pdf; // attenuation = f_r * cos{θ_i} / pdf

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

		MaterialSampleContext Sample(const MaterialEvalContext& context) const override {
			MaterialSampleContext sampleContext{};

			float u = RandomFloat();
			if (u < pkd) 
			{
				// Sample Diffuse
				vec3 wi = SampleCosineHemisphere();
				sampleContext.wi = wi;
				sampleContext.pdf = DiffusePDF(wi, context);
				// f_r_diffuse = kd / pi
				sampleContext.f = Kd->Value(context.uv[0], context.uv[1], context.p) * InvPi;
				sampleContext.flags = SampleFlags::Diffuse;
			}
			else if (pkd <= u && u < pkd + pks)
			{
				// Sample Specular
				float u1 = RandomFloat(), u2 = RandomFloat();
				float alpha = glm::acos(glm::pow(u1, 1.0 / (Ns + 1.0)));
				float phi = 2.0f * Pi * u2;
				float sinAlpha = glm::sin(alpha), cosAlpha = glm::cos(alpha),
					sinPhi = glm::sin(phi), cosPhi = glm::cos(phi);
				vec3 reflectWi = vec3(sinAlpha * cosPhi, sinAlpha * sinPhi, cosAlpha);

				sampleContext.wi = ReflectiveSpaceToLocal(reflectWi, context);
				sampleContext.pdf = SpecularPDF(sampleContext.wi, context);
				vec3 localReflect = glm::normalize( Reflect(context.wo, vec3(0, 0, 1)) );
				float localCosAlpha = glm::dot(sampleContext.wi, localReflect);
				// f_r_specular = ks*(Ns+2)/(2*Pi)*(cosα)^n
				sampleContext.f = Ks->Value(context.uv[0], context.uv[1], context.p) * (Ns + 2) * Inv2Pi * glm::pow(localCosAlpha, Ns);
				sampleContext.flags = SampleFlags::Specular;
			}

			return sampleContext;
		}
		float DiffusePDF(const vec3& wi, const MaterialEvalContext& context) const
		{  
			// wi : local space.
			float cosTheta = wi.z;
			return cosTheta * InvPi;
		}
		float SpecularPDF(const vec3& wi, const MaterialEvalContext& context) const
		{
			vec3 localReflect = glm::normalize( Reflect(context.wo, vec3(0, 0, 1)) );
			float cosAlpha = glm::dot(wi, localReflect);
			return (Ns+1.0) * Inv2Pi * glm::pow(cosAlpha, Ns);
		}
		bool Scatter(const Ray& rayIn, const HitRecord& record, color& attenuation, Ray& scatteredRay)
			const override {

			MaterialEvalContext context{};
			context.p = record.position;
			context.uv = record.uv;
			context.n = record.normal;
			context.wo = WorldToLocal(-rayIn.direction, record);
			context.dpdus = record.tangent;

			MaterialSampleContext sampleContext = Sample(context);
			const vec3& wi = sampleContext.wi;
			float cosTheta = wi.z;
			const vec3& fr = sampleContext.f;
			const float& pdf = sampleContext.pdf;

			scatteredRay = Ray(record.position, LocalToWorld(wi, context));
			attenuation = fr * cosTheta / pdf;

			return true;
		}

		vec3 Reflect(const vec3 &wo, const vec3 &n) const {
			return -wo + 2 * dot(wo, n) * n;
		}
		vec3 LocalToReflectiveSpace(const vec3& local, const MaterialEvalContext& context) const
		{
			// Local Space to Reflective Space
			vec3 localR = glm::normalize(Reflect(context.wo, vec3(0, 0, 1)));
			vec3 V = (std::fabs(localR.x) > 0.9f ? vec3(0, 1, 0) : vec3(1, 0, 0));

			vec3 T = glm::normalize(glm::cross(V, localR));
			vec3 B = glm::cross(localR, T);

			float x = glm::dot(local, T);
			float y = glm::dot(local, B);
			float z = glm::dot(local, localR);

			return vec3(x, y, z);
		}
		vec3 ReflectiveSpaceToLocal(const vec3& reflect, const MaterialEvalContext& context) const
		{
			// Local Space to Reflective Space
			vec3 localR = glm::normalize(Reflect(context.wo, vec3(0, 0, 1)));
			vec3 V = (std::fabs(localR.x) > 0.9f ? vec3(0, 1, 0) : vec3(1, 0, 0));

			vec3 T = glm::normalize(glm::cross(V, localR));
			vec3 B = glm::cross(localR, T);

			return reflect.x * T + reflect.y * B + reflect.z * localR;
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