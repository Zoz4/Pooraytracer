#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Camera.h"
#include "Ray.h"
#include "Logger.h"
#include "Material.h"
#include "RandomNumberGenerator.h"

#include <thread>
#include <mutex>

namespace Pooraytracer {

	static std::mutex mtx;

	void Camera::Render(Hittable& world, Hittable& lights)
	{
		Initialize();

		LOGI("Render Start...");

		/*
		for (int j = 0; j < imageHeight; ++j) {
			LOGI("Scanlines remaining : {}", imageHeight - j);
			for (int i = 0; i < imageWidth; ++i) {
			  Ray ray = GetRay(i, j);
				color pixelColor(0., 0., 0.);
				for (int sample = 0; sample < samplesPerPixel; ++sample)
				{
					pixelColor += RayColor(ray, maxDepth, world);
				}
				size_t idx = i + j * imageWidth;
				pixelColor *= pixelSamplesScale;
				colorAttachment[idx * 3] = pixelColor.x * 255;
				colorAttachment[idx * 3 + 1] = pixelColor.y * 255;
				colorAttachment[idx * 3 + 2] = pixelColor.z * 255;
			}
		}
		*/

		int process = imageHeight;
		int times = imageHeight / threadNums;
		std::vector<std::thread> threads(threadNums);
		auto castRayMultiThread = [&](uint32_t yMin, uint32_t yMax) {
			for (uint32_t j = yMin; j < yMax; j++) {
				int m = j * imageWidth;
				for (uint32_t i = 0; i < imageWidth; i++) {
					Ray ray = GetRay(i, j);
					for (int sample = 0; sample < samplesPerPixel; ++sample)
					{
						colorAttachment[m] += RayColor(ray, maxDepth, world, lights);
					}
					colorAttachment[m] *= pixelSamplesScale;
					++m;
				}
				mtx.lock();
				process--;
				ShowProgress(process);
				mtx.unlock();
			}
			};
		for (int i = 0; i < threadNums; i++) {
			threads[i] = std::thread(castRayMultiThread, i * times, (i + 1) * times);
		}
		for (auto& th : threads) {
			th.join();
		}
		LOGI("Render End...");
	}

	void Camera::Initialize()
	{
		imageWidth = (imageWidth < 1) ? 1 : imageWidth;
		imageHeight = (imageHeight < 1) ? 1 : imageHeight;
		aspectRatio = double(imageWidth) / double(imageHeight);

		colorAttachment.resize(imageWidth * imageHeight, color(0., 0., 0.));

		pixelSamplesScale = 1.0 / samplesPerPixel;

		center = eye;

		double focalLength = glm::length(eye - lookAt);
		double theta = glm::radians(fovy);
		double h = std::tan(theta / 2.0);

		double viewportHeight = 2. * h * focalLength;
		double viewportWidth = viewportHeight * aspectRatio;

		w = glm::normalize(eye - lookAt);
		u = glm::normalize(glm::cross(up, w));
		v = glm::cross(w, u);

		vec3 viewportU = viewportWidth * u;
		vec3 viewportV = viewportHeight * -v;

		pixelDeltaU = viewportU / (double)imageWidth;
		pixelDeltaV = viewportV / (double)imageHeight;
		vec3 viewportUpperLeft = center - (focalLength * w) - viewportU / 2. - viewportV / 2.;
		pixel00Location = viewportUpperLeft + 0.5 * (pixelDeltaU + pixelDeltaV);

	}

	Ray Camera::GetRay(int i, int j) const
	{
		//vec2 offset = SampleSquare();
		//vec3 pixelSample = pixel00Location + ((double)i+offset.x) * pixelDeltaU + ((double)j+offset.y) * pixelDeltaV;
		vec3 pixelSample = pixel00Location + ((double)i) * pixelDeltaU + ((double)j) * pixelDeltaV;
		vec3 origin = center;
		vec3 direction = pixelSample - origin;

		return Ray(origin, direction);
	}

	color Camera::RayColor(const Ray& ray, int depth, const Hittable& world, const Hittable& lights)
	{
		if (depth < 0.) {
			return color(0., 0., 0.);
		}
		HitRecord record;
		if (!world.Hit(ray, Interval(0.001, std::numeric_limits<double>::infinity()), record))
		{
			return background;
		}
		if (record.material->HasEmission())
		{
			return record.material->GetEmission();
		}
		const point3& ps = record.position; // shade point

		double pdfLights;
		HitRecord lightsSamplePointRecord;
		lights.Sample(ps, lightsSamplePointRecord, pdfLights); // sample lights from shade point
		const point3& pl = lightsSamplePointRecord.position; // light sample point
		vec3 lightDirection = glm::normalize(pl - ps);
		vec3 lightNormal = lightsSamplePointRecord.normal;
		std::shared_ptr<Material> lightMaterial = lightsSamplePointRecord.material;

		HitRecord lightRayHitRecord;
		double distance = glm::length(pl - ps);
		Ray shadePoint2LightRay(ps, lightDirection);
		world.Hit(shadePoint2LightRay, Interval(0.001, std::numeric_limits<double>::infinity()), lightRayHitRecord);

		color direct{ 0. }, scatter{ 0. };

		const point3& pNearest = lightRayHitRecord.position;
		if (lightsSamplePointRecord.bFrontFace && // light area is front to shade point
			distance - glm::length(ps - pNearest) < 0.001) { // shade point is visible to light

			color emission = lightMaterial->GetEmission();
			//LOGI("emmision: {}", glm::to_string(emission).c_str());
			MaterialEvalContext context;
			context.p = record.position;
			context.uv = record.uv;
			context.n = record.normal;
			context.wo = Material::WorldToLocal(-ray.direction, record);
			context.dpdus = record.tangent;

			// Transform all vector to shade point's local space.
			const vec3& localWi = Material::WorldToLocal(lightDirection, record);
			const vec3& localLightNormal = Material::WorldToLocal(lightNormal, record);
			vec3 fr = record.material->Eval(localWi, context);
			double cosTheta = localWi.z; // θ: the angle of light direction and face normal
			double cosThetaBar = glm::dot(localLightNormal, -localWi);  // θ': the angle of light area normal and light direcction

			direct = emission * fr * cosTheta * cosThetaBar / (distance * distance) / pdfLights;
		}

		Ray scatteredRay;
		color attenuation; // attenuation =  fr * cosθ / pdf(wi) : Indirect illumination
		HitRecord scatteredRecord;
		if (record.material->Scatter(ray, record, attenuation, scatteredRay))
		{
			scatter = attenuation * RayColor(scatteredRay, depth - 1, world, lights);
		}
		return direct + scatter;
	}

	color Camera::LinearToGamma(color linearColor) const
	{
		color grammaColor = glm::sqrt(linearColor);
		return grammaColor;
	}

	void Camera::WriteColorAttachment(const std::string& outputPath) const
	{
		std::vector<uint8_t> rawImage(imageHeight * imageWidth * 3);
		for (size_t j = 0; j < imageHeight; ++j) {
			for (size_t i = 0; i < imageWidth; ++i) {
				size_t idx = i + j * imageWidth;

				color linearColor = colorAttachment[idx];
				auto& r = linearColor.x;
				auto& g = linearColor.y;
				auto& b = linearColor.z;

				if (r != r) r = 0.0;
				if (g != g) g = 0.0;
				if (b != b) b = 0.0;

				color gammaColor = LinearToGamma(linearColor);
				static const Interval intensity(0.0000, 0.9999);
				rawImage[idx * 3] = (uint8_t)(intensity.Clamp(gammaColor.r) * 256);
				rawImage[idx * 3 + 1] = (uint8_t)(intensity.Clamp(gammaColor.g) * 256);
				rawImage[idx * 3 + 2] = (uint8_t)(intensity.Clamp(gammaColor.b) * 256);
			}
		}
		stbi_write_png(outputPath.c_str(), imageWidth, imageHeight, 3, rawImage.data(), imageWidth * 3);
	}

	std::string Camera::GetParametersStr() const
	{
		std::stringstream ss;
		ss << "spp" << samplesPerPixel << "-depth" << maxDepth;
		return ss.str();
	}

	void ShowProgress(int progress)
	{
		LOGI("Scanlines remaining : {}", progress);
	}
}