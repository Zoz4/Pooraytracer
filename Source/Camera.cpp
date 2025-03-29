#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <tinyxml2.h>

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
			for (uint32_t j = yMin; j < yMax && j<imageHeight; j++) {
				int m = j * imageWidth;
				for (uint32_t i = 0; i < imageWidth; i++) {
					Ray ray = GetRay(i, j);
					for (int sample = 0; sample < samplesPerPixel; ++sample)
					{
						colorAttachment[m] += RayColor(ray, maxDepth, world, lights) * pixelSamplesScale;
					}
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
		if (!world.Hit(ray, Interval(0.0001, std::numeric_limits<double>::infinity()), record))
		{
			return background;
		}
		if (record.material->HasEmission())
		{
			return record.material->GetEmission();
		}
		const point3& ps = record.position; // shade point

		color direct{ 0.,0.,0. }, scatter{ 0.,0.,0. };

		if (bSampleLights && !record.material->SkipLightSampling())
		{
			double pdfLights=0.0;
			HitRecord lightsSamplePointRecord;
			lights.Sample(ps, lightsSamplePointRecord, pdfLights); // sample lights from shade point
			const point3& pl = lightsSamplePointRecord.position; // light sample point
			vec3 lightDirection = glm::normalize(pl - ps);		 // shade point to light sample point
			vec3 lightNormal = lightsSamplePointRecord.normal;
			std::shared_ptr<Material> lightMaterial = lightsSamplePointRecord.material;

			HitRecord lightRayHitRecord;
			double distance = glm::length(pl - ps);
			Ray shadePoint2LightRay(ps, lightDirection);
			world.Hit(shadePoint2LightRay, Interval(0.001, std::numeric_limits<double>::max()), lightRayHitRecord);

			const point3& pNearest = lightRayHitRecord.position;
			if (glm::dot(record.normal, lightDirection) > 0.0 && // light direction is in the same side of eye's ray
				lightsSamplePointRecord.bFrontFace && // light area is front to shade point
				distance - glm::length(ps - pNearest) < 0.001) { // shade point is visible to light

				color emission = lightMaterial->GetEmission();
				MaterialEvalContext context;
				context.p = record.position;
				context.uv = record.uv;
				context.n = record.normal;
				context.dpdus = record.tangent;
				context.wo = Material::WorldToLocal(-ray.direction, record);

				// Transform all vector to shade point's local space.
				const vec3& localWi = Material::WorldToLocal(lightDirection, record);
				const vec3& localLightNormal = Material::WorldToLocal(lightNormal, record);
				vec3 fr = record.material->Eval(localWi, context);
				double cosTheta = localWi.z; // θ: the angle of light direction and face normal
				double cosThetaBar = glm::dot(localLightNormal, -localWi);  // θ': the angle of light area normal and light direcction

				direct = emission * fr * cosTheta * cosThetaBar / (distance * distance) / pdfLights;
			}
		}

		Ray scatteredRay;
		color attenuation; // attenuation =  fr * cosθ / pdf(wi) : Indirect illumination
		HitRecord scatteredRecord;
		// russian roulette
		if (RandomDouble() < russianRoulette)
		{
			if (record.material->Scatter(ray, record, attenuation, scatteredRay))
			{
				if (bSampleLights)
				{
					HitRecord scatterRayHitRecord;
					if (world.Hit(scatteredRay, Interval(0.0001, std::numeric_limits<double>::infinity()), scatterRayHitRecord)) {
						if (!scatterRayHitRecord.material->HasEmission()) {
							scatter = attenuation * RayColor(scatteredRay, depth - 1, world, lights)/russianRoulette;
						}
						else {
							if (record.material->SkipLightSampling()) { // Perfect Specular or Phone Reflectance Ns > 1
								scatter = attenuation * RayColor(scatteredRay, depth - 1, world, lights) / russianRoulette;
							}
						}
					}
				}
				else {
					scatter = attenuation * RayColor(scatteredRay, depth - 1, world, lights) / russianRoulette;
				}
			}
		}
		return direct + scatter;
	}

	color Camera::LinearToSRGB(color linearColor) const
	{
		color srgbColor = color(
			LinearToSRGB(linearColor.r),
			LinearToSRGB(linearColor.g),
			LinearToSRGB(linearColor.b)
		);
		return srgbColor;
	}

	double Camera::LinearToSRGB(double linearColorComponent) const
	{
		if (linearColorComponent <= 0.0031308)
			return 12.92 * linearColorComponent;
		return 1.055 * std::pow(linearColorComponent, (1. / 2.4)) - 0.055;
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

				color srgbColor = LinearToSRGB(linearColor);
				static const Interval intensity(0.0000, 0.9999);
				rawImage[idx * 3] = (uint8_t)(intensity.Clamp(srgbColor.r) * 256);
				rawImage[idx * 3 + 1] = (uint8_t)(intensity.Clamp(srgbColor.g) * 256);
				rawImage[idx * 3 + 2] = (uint8_t)(intensity.Clamp(srgbColor.b) * 256);
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

	void Camera::SetViewParametersByXmlFile(const std::string& xmlFilePath)
	{
		tinyxml2::XMLDocument doc;
		if (doc.LoadFile((xmlFilePath).c_str()) != tinyxml2::XML_SUCCESS) {
			LOGE("Failed to load XML file: {}", xmlFilePath);
		}
		tinyxml2::XMLElement* camera = doc.FirstChildElement("camera");
		if (camera) {
			const char* type = camera->Attribute("type");
			int width, height;
			double fovy;
			camera->QueryIntAttribute("width", &width);
			camera->QueryIntAttribute("height", &height);
			camera->QueryDoubleAttribute("fovy", &fovy);
			LOGI("Camera Type: {}", (type ? type : "Unknown"));
			LOGI("Resolution: {}x{}", width, height);
			LOGI("FOV Y: {}", fovy);

			this->imageWidth = width;
			this->imageHeight = height;
			this->fovy = fovy;

			tinyxml2::XMLElement* eye = camera->FirstChildElement("eye");
			if (eye) {
				double x, y, z;
				eye->QueryDoubleAttribute("x", &x);
				eye->QueryDoubleAttribute("y", &y);
				eye->QueryDoubleAttribute("z", &z);
				LOGI("Eye Position: ({}, {}, {})", x, y, z);
				this->eye = vec3(x, y, z);
			}
			tinyxml2::XMLElement* lookat = camera->FirstChildElement("lookat");
			if (lookat) {
				double x, y, z;
				lookat->QueryDoubleAttribute("x", &x);
				lookat->QueryDoubleAttribute("y", &y);
				lookat->QueryDoubleAttribute("z", &z);
				LOGI("lookat: ({}, {}, {})", x, y, z);
				this->lookAt = vec3(x, y, z);
			}
			tinyxml2::XMLElement* up = camera->FirstChildElement("up");
			if (up) {
				double x, y, z;
				up->QueryDoubleAttribute("x", &x);
				up->QueryDoubleAttribute("y", &y);
				up->QueryDoubleAttribute("z", &z);
				LOGI("up: ({}, {}, {})", x, y, z);
				this->up = vec3(x, y, z);
			}
		}
	}

	void ShowProgress(int progress)
	{
		LOGI("Scanlines remaining : {}", progress);
	}
}