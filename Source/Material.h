#pragma once


#include "Ray.h"
#include "Hittable.h"
#include "Texture.h"
#include "RandomNumberGenerator.h"
#include "MaterialUtils.h"
#include "Logger.h"

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/geometric.hpp>

namespace Pooraytracer {
	using vec3 = glm::dvec3;
	using color = glm::dvec3;
	using point3 = glm::dvec3;
	using glm::normalize, glm::cross, glm::length, glm::dot;

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
		Specular = 1 << 1,
		GlossyReflection = 1 << 2
	};

	struct MaterialSampleContext {
	public:
		vec3 wi;		// local space wi
		vec3 wm;		// microfacet normal
		vec3 f;			// brdf
		double pdf;
		SampleFlags flags;
	};

	enum class MaterialType
	{
		Lambertian, PhoneReflectance, PerfectMirror, CookTorrance, DiffuseLight, DebugMaterial
	};

	class Material {
	public:
		virtual ~Material() = default;
		virtual color Emmited(double u, double v, const point3& p) const {
			return color(0., 0., 0.);
		}
		virtual bool Scatter(const Ray& rayIn, const HitRecord& record, color& attenuation, Ray& scatteredRay) const {
			return false;
		}
		virtual MaterialSampleContext Sample(const MaterialEvalContext& context) const {
			return MaterialSampleContext{};
		};
		virtual vec3 Eval(const vec3& wi, const MaterialEvalContext& context) const {
			return vec3(0., 0., 0.);
		};
		virtual double PDF(const vec3& wi, const MaterialEvalContext& context) const {
			return 1.0;
		}
		virtual bool HasEmission() const { return false; }
		virtual color GetEmission() const { return color(0., 0., 0.); }
		virtual bool SkipLightSampling() const { return false; }

	public:
		static vec3 LocalToWorld(const vec3& local, const MaterialEvalContext& context)
		{
			const vec3& normal = context.n;
			const vec3& tangent = context.dpdus;
			vec3 bitangent = glm::cross(tangent, normal);

			return glm::normalize(local.x * tangent + local.y * bitangent + local.z * normal);
		}
		static vec3 WorldToLocal(const vec3& world, const HitRecord& record)
		{
			const vec3& normal = record.normal;
			const vec3& tangent = record.tangent;
			vec3 bitangent = glm::cross(tangent, normal);

			double x = glm::dot(world, tangent);
			double y = glm::dot(world, bitangent);
			double z = glm::dot(world, normal);

			return vec3(x, y, z);
		}
		static vec3 Reflect(const vec3& wo, const vec3& n) {
			return -wo + 2. * dot(wo, n) * n;
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
			while (wi.z <= 0.) {
				// wi.z == 0.0 -> pdf = 0.
				// which will cause cosθ / pdf == NAN
				// so we simply resample a wi
				wi = SampleCosineHemisphere();
			}
			sampleContext.wi = wi;
			sampleContext.pdf = PDF(wi, context);
			sampleContext.f = Eval(wi, context);
			sampleContext.flags = SampleFlags::Diffuse;
			return sampleContext;
		}
		double PDF(const vec3& wi, const MaterialEvalContext& context) const override
		{
			double cosTheta = wi.z;
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
			const double cosTheta = sampleContext.wi.z;
			const vec3& fr = sampleContext.f;
			const double& pdf = sampleContext.pdf;

			scatteredRay = Ray(record.position, LocalToWorld(wi, context));
			attenuation = fr * cosTheta / pdf; // attenuation = f_r * cos{θ_i} / pdf

			return true;
		}

	private:
		shared_ptr<Texture> texture;
	};

	class DiffuseLight : public Material {
	public:

		DiffuseLight(shared_ptr<Texture> texture) :texture(texture) {}
		DiffuseLight(const color& emit) :texture(make_shared<SolidColor>(emit)) {}

		color Emmited(double u, double v, const point3& p) const override {
			return texture->Value(u, v, p);
		}
		bool HasEmission() const override { return true; }
		color GetEmission() const override { return Emmited(0., 0., point3(0.)); }
	private:
		shared_ptr<Texture> texture;
	};

	class PhoneReflectance : public Material {
	public:
		PhoneReflectance(const color& Kd, const color& Ks, double Ns) :
			Kd(make_shared<SolidColor>(Kd)), Ks(make_shared<SolidColor>(Ks)), Ns(Ns) {
			SetProbabilitiesByNs();
		}
		PhoneReflectance(shared_ptr<Texture> mapKd, const color& Ks, double Ns) :
			Kd(mapKd), Ks(mapKd), Ns(Ns) {
			SetProbabilitiesByNs();
		}

		MaterialSampleContext Sample(const MaterialEvalContext& context) const override {
			MaterialSampleContext sampleContext{};

			double u = RandomDouble();
			if (u < pkd)
			{
				// Sample Diffuse
				vec3 wi = SampleCosineHemisphere();
				while (wi.z <= 0.) {
					wi = SampleCosineHemisphere();
				}
				sampleContext.wi = wi;
				sampleContext.pdf = DiffusePDF(wi, context);
				// f_r_diffuse = kd / pi
				sampleContext.f = Kd->Value(context.uv[0], context.uv[1], context.p) * InvPi;
				sampleContext.flags = SampleFlags::Diffuse;
			}
			else if (pkd <= u && u < pkd + pks)
			{
				// Sample Specular
				vec3 wi;
				double u1 = RandomDouble(), u2 = RandomDouble();
				double alpha = glm::acos(glm::pow(u1, 1.0 / (Ns + 1.0)));
				double phi = 2.0 * Pi * u2;
				double sinAlpha = glm::sin(alpha), cosAlpha = glm::cos(alpha),
					sinPhi = glm::sin(phi), cosPhi = glm::cos(phi);
				vec3 reflectWi = vec3(sinAlpha * cosPhi, sinAlpha * sinPhi, cosAlpha);
				wi = ReflectiveSpaceToLocal(reflectWi, context);

				sampleContext.wi = wi;
				sampleContext.pdf = SpecularPDF(sampleContext.wi, context);
				vec3 localReflect = glm::normalize(Reflect(context.wo, vec3(0., 0., 1.)));
				double localCosAlpha = std::max(0.0, glm::dot(sampleContext.wi, localReflect));

				// f_r_specular = ks*(Ns+2)/(2*Pi)*(cosα)^n
				if (wi.z > 0. && localCosAlpha > 0.) {
					sampleContext.f = Ks->Value(context.uv[0], context.uv[1], context.p) * (Ns + 2.) * Inv2Pi * glm::pow(localCosAlpha, Ns);
				}

				sampleContext.flags = SampleFlags::Specular;
			}

			return sampleContext;
		}
		vec3 Eval(const vec3& wi, const MaterialEvalContext& context) const override {
			double u = RandomDouble();
			if (u < pkd) {
				if (wi.z <= 0) {
					return vec3(0.0, 0.0, 0.0);
				}
				return Kd->Value(context.uv[0], context.uv[1], context.p) * InvPi;
			}
			else if (pkd <= u && u < pkd + pks) {
				if (wi.z <= 0) {
					return vec3(0.0, 0.0, 0.0);
				}
				vec3 localReflect = glm::normalize(Reflect(context.wo, vec3(0., 0., 1.)));
				double localCosAlpha = std::max(0., glm::dot(wi, localReflect));
				if (localCosAlpha <= 0.) {
					return vec3(0.0, 0.0, 0.0);
				}

				return Ks->Value(context.uv[0], context.uv[1], context.p) * (Ns + 2.) * Inv2Pi * glm::pow(localCosAlpha, Ns);
			}
			return vec3(0.);
		}
		double DiffusePDF(const vec3& wi, const MaterialEvalContext& context) const
		{
			// wi : local space.
			double cosTheta = wi.z;
			return cosTheta * InvPi;
		}
		double SpecularPDF(const vec3& wi, const MaterialEvalContext& context) const
		{
			if (wi.z <= 0.) return 0.0;
			vec3 localReflect = glm::normalize(Reflect(context.wo, vec3(0., 0., 1.)));
			double cosAlpha = glm::dot(wi, localReflect);
			return (Ns + 1.0) * Inv2Pi * glm::pow(cosAlpha, Ns);
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
			double cosTheta = wi.z;
			const vec3& fr = sampleContext.f;
			const double& pdf = sampleContext.pdf;

			scatteredRay = Ray(record.position, LocalToWorld(wi, context));

			if (pdf > 0. && wi.z > 0) {
				attenuation = fr * cosTheta / pdf;
			}

			return true;
		}

		vec3 LocalToReflectiveSpace(const vec3& local, const MaterialEvalContext& context) const
		{
			// Local Space to Reflective Space
			vec3 localR = glm::normalize(Reflect(context.wo, vec3(0., 0., 1.)));
			vec3 V = (std::fabs(localR.x) > 0.9 ? vec3(0., 1., 0.) : vec3(1., 0., 0.));

			vec3 T = glm::normalize(glm::cross(V, localR));
			vec3 B = glm::cross(localR, T);

			double x = glm::dot(local, T);
			double y = glm::dot(local, B);
			double z = glm::dot(local, localR);

			return vec3(x, y, z);
		}
		vec3 ReflectiveSpaceToLocal(const vec3& reflect, const MaterialEvalContext& context) const
		{
			// Local Space to Reflective Space
			vec3 localR = glm::normalize(Reflect(context.wo, vec3(0., 0., 1.)));
			vec3 V = (std::fabs(localR.x) > 0.9 ? vec3(0., 1., 0.) : vec3(1., 0., 0.));

			vec3 T = glm::normalize(glm::cross(V, localR));
			vec3 B = glm::cross(localR, T);

			return reflect.x * T + reflect.y * B + reflect.z * localR;
		}
	private:
		shared_ptr<Texture> Kd;
		shared_ptr<Texture> Ks;
		double Ns;
		double pkd, pks;
		void SetProbabilitiesByNs() {
			if (Ns <= 1.) {
				pkd = 1.0;
				pks = 0.0;
			}
			else {
				pkd = 0.6;
				pks = 0.4;
			}
		}
		bool SkipLightSampling() const override { return Ns > 1.; }

	};

	class PerfectMirror :public Material {
	public:
		MaterialSampleContext Sample(const MaterialEvalContext& context) const override
		{
			const vec3& wo = context.wo;
			MaterialSampleContext sampleContext;
			sampleContext.wi = Reflect(wo, vec3(0., 0., 1.));
			double cosTheta = sampleContext.wi.z;
			sampleContext.f = color(1.0, 1.0, 1.0) / cosTheta;
			sampleContext.pdf = 1;
			return sampleContext;
		}
		bool Scatter(const Ray& rayIn, const HitRecord& record, color& attenuation, Ray& scatteredRay) const override
		{
			MaterialEvalContext context{};
			context.p = record.position;
			context.uv = record.uv;
			context.n = record.normal;
			context.wo = WorldToLocal(-rayIn.direction, record);
			context.dpdus = record.tangent;

			MaterialSampleContext sampleContext = Sample(context);
			const vec3& wi = sampleContext.wi;
			double cosTheta = wi.z;
			const vec3& fr = sampleContext.f;// fr = vec3(1., 1., 1.)
			const double& pdf = sampleContext.pdf; // pdf ~ δ function

			scatteredRay = Ray(record.position, LocalToWorld(wi, context));
			attenuation = fr * cosTheta / pdf;

			return true;
		}

		bool SkipLightSampling() const override { return true; }
	};

	class CookTorrance : public Material {
	public:
		CookTorrance(const color& Kd, double alphaX=0.3, double alphaY=0.3, vec3 eta=vec3(1.0), vec3 k=vec3(0.0)) :texture(make_shared<SolidColor>(Kd)), alphaX(alphaX), alphaY(alphaY),
			eta(eta), k(k) {
		}
		double D(const vec3& wm) const {
			double tan2Theta = Tan2Theta(wm);
			if (IsInf(tan2Theta)) return 0;
			double cos4Theta = Sqr(Cos2Theta(wm));
			double e = tan2Theta * (Sqr(CosPhi(wm) / alphaX) +
				Sqr(SinPhi(wm) / alphaY));
			return 1 / (Pi * alphaX * alphaY * cos4Theta * Sqr(1 + e));
		}
		double Lambda(const vec3& w) const {
			double tan2Theta = Tan2Theta(w);
			if (IsInf(tan2Theta)) return 0;
			double alpha2 = Sqr(CosPhi(w) * alphaX) + Sqr(SinPhi(w) * alphaY);
			return (std::sqrt(1 + alpha2 * tan2Theta) - 1) / 2;
		}
		double G1(const vec3& w) const { return 1 / (1 + Lambda(w)); }
		double G(const vec3& wo, const vec3& wi) const {
			return 1 / (1 + Lambda(wo) + Lambda(wi));
		}
		double D(const vec3& w, const vec3& wm) const {
			return G1(w) / AbsCosTheta(w) * D(wm) * AbsDot(w, wm);
		}

		double PDF(const vec3& w, const vec3& wm) const {
			return D(w, wm);
		}

		double PDF(const vec3& wi, const MaterialEvalContext& context) const override{
			const vec3& wo = context.wo;
			if (!SameHemisphere(wo, wi))
				return 0;
			vec3 wm = wo + wi;
			if (LengthSquared(wm) == 0)
				return 0;
			wm = FaceForward(normalize(wm), vec3(0, 0, 1));
			return PDF(wo, wm) / (4. * AbsDot(wo, wm));
		}

		vec3 SampleWm(const vec3& w, vec2 u) const {
			// Transform w to hemispherical configuration 
			vec3 wh = glm::normalize(vec3(alphaX * w.x, alphaY * w.y, w.z));
			if (wh.z < 0)
				wh = -wh;
			// Find orthonormal basis for visible normal sampling
			vec3 T1 = (wh.z < 0.99999) ? normalize(cross(vec3(0., 0., 1.), wh))
				: vec3(1, 0, 0);
			vec3 T2 = cross(wh, T1); // an orthonormal basis (T1, T2, wh)
			//  Generate uniformly distributed points on the unit disk
			vec2 p = SampleUniformDiskPolar(u);
			// Warp hemispherical projection for visible normal sampling
			double h = std::sqrt(1 - Sqr(p.x));
			p.y = Lerp((1 + wh.z) / 2, h, p.y);

			// Reproject to hemisphere and transform normal to ellipsoid configuration
			double pz = std::sqrt(std::max<double>(0., 1. - LengthSquared(vec2(p))));
			vec3 nh = p.x * T1 + p.y * T2 + pz * wh;
			return normalize(vec3(alphaX * nh.x, alphaY * nh.y,
				std::max < double > (1e-6, nh.z)));
		}

		MaterialSampleContext Sample(const MaterialEvalContext& context) const override
		{
			MaterialSampleContext sampleContext{};

			const vec3& wo = context.wo;

			// Sample rough conductor BRDF
			// Sample microfacet normal $\wm$ and reflected direction $\wi$
			if (wo.z == 0) {
				return {};
			}
			vec2 u = vec2(RandomDouble(), RandomDouble());
			vec3 wm = SampleWm(wo, u);
			vec3 wi = Reflect(wo, wm);
			if (!SameHemisphere(wo, wi)) {
				return {};
			}
			// Compute PDF of _wi_ for microfacet reflection
			double pdf = PDF(wo, wm) / (4. * AbsDot(wo, wm));
			double cosTheta_o = AbsCosTheta(wo), cosTheta_i = AbsCosTheta(wi);
			if (cosTheta_i == 0 || cosTheta_o == 0)
				return {};

			// Evaluate Fresnel factor _F_ for conductor BRDF
			vec3 F = vec3(FrComplex(AbsDot(wo, wm), Complex<double>(eta.x, k.x)), FrComplex(AbsDot(wo, wm), Complex<double>(eta.y, k.y)), FrComplex(AbsDot(wo, wm), Complex<double>(eta.z, k.z)));

			//double F = FrComplex(AbsDot(wo, wm), Complex<double>(eta, k));
			//color diffuse = (vec3(1.0f) - F) * texture->Value(context.uv[0], context.uv[1], point3(0)) / Pi;
			vec3 f = vec3(D(wm) * F * G(wo, wi) / (4. * cosTheta_i * cosTheta_o));

			sampleContext.f = f;
			sampleContext.pdf = pdf;
			sampleContext.wm = wm;
			sampleContext.wi = wi;
			sampleContext.flags = SampleFlags::GlossyReflection;

			return sampleContext;
		}

		vec3 Eval(const vec3& wi, const MaterialEvalContext& context) const override
		{
			const vec3& wo = context.wo;
			if (!SameHemisphere(wo, wi))
				return  { 0., 0., 0. };
			double cosTheta_o = AbsCosTheta(wo), cosTheta_i = AbsCosTheta(wi);
			if (cosTheta_i == 0 || cosTheta_o == 0)
				return { 0., 0., 0. };

			vec3 wm = wi + wo;

			if (LengthSquared(wm) == 0)
				return {0., 0., 0.};

			wm = normalize(wm);

			vec3 F = vec3(FrComplex(AbsDot(wo, wm), Complex<double>(eta.x, k.x)), FrComplex(AbsDot(wo, wm), Complex<double>(eta.y, k.y)), FrComplex(AbsDot(wo, wm), Complex<double>(eta.z, k.z)));

			//double F = FrComplex(AbsDot(wo, wm), Complex<double>(eta, k));

			//color diffuse = (vec3(1.0f) - F) * texture->Value(context.uv[0], context.uv[1], point3(0)) / Pi;
			return vec3(D(wm)*F*G(wo, wi)/ (4 * cosTheta_i * cosTheta_o));
		}
		bool Scatter(const Ray& rayIn, const HitRecord& record, color& attenuation, Ray& scatteredRay)
			const override {

			MaterialEvalContext context;
			context.p = record.position;
			context.uv = record.uv;
			context.n = record.normal;
			context.wo = normalize(WorldToLocal(-rayIn.direction, record));
			context.dpdus = record.tangent;

			MaterialSampleContext sampleContext = Sample(context);
			if (sampleContext.flags == SampleFlags::Unset) {
				return false;
			}
			const vec3& wi = sampleContext.wi;
			const double& cosTheta = wi.z;
			const vec3& fr = sampleContext.f;
			const double& pdf = sampleContext.pdf;

			attenuation = fr * cosTheta / pdf;
			scatteredRay = Ray(record.position, LocalToWorld(wi, context));
			return true;
		}
	private:
		vec3 eta, k;
		double alphaX = 0.2, alphaY = 0.2;
		shared_ptr<Texture> texture;
	};

	class DebugMaterial : public Material {
	public:

		DebugMaterial(shared_ptr<Texture> texture) :texture(texture) {}
		DebugMaterial(const color& albedo) :texture(make_shared<SolidColor>(albedo)) {}
		bool HasEmission() const override { return true; }
		color GetEmission() const override { return Emmited(0., 0., point3(0.)); }
		color Emmited(double u, double v, const point3& p) const override {
			return texture->Value(u, v, p);
		}
	private:
		shared_ptr<Texture> texture;
	};

}