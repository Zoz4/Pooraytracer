#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Camera.h"
#include "Ray.h"
#include "Logger.h"
#include "Material.h"
#include <thread>
#include <mutex>

namespace Pooraytracer {

	std::mutex mtx;
	void Camera::Render(Hittable& world)
	{
		Initialize();

		LOGI("Render Start...");

		//for (int j = 0; j < imageHeight; ++j) {
		//	LOGI("Scanlines remaining : {}", imageHeight - j);
		//	for (int i = 0; i < imageWidth; ++i) {
		//      Ray ray = GetRay(i, j);
		//		color pixelColor(0.f, 0.f, 0.f);
		//		for (int sample = 0; sample < samplesPerPixel; ++sample)
		//		{
		//			pixelColor += RayColor(ray, maxDepth, world);
		//		}
		//		size_t idx = i + j * imageWidth;
		//		pixelColor *= pixelSamplesScale;
		//		colorAttachment[idx * 3] = pixelColor.x * 255;
		//		colorAttachment[idx * 3 + 1] = pixelColor.y * 255;
		//		colorAttachment[idx * 3 + 2] = pixelColor.z * 255;
		//	}
		//}

		int process = imageHeight;
		const int thred = 20;
		int times = imageHeight / thred;
		std::thread th[thred];
		auto castRayMultiThread = [&](uint32_t yMin, uint32_t yMax) {
			for (uint32_t j = yMin; j < yMax; j++) {
				int m = j * imageWidth;
				for (uint32_t i = 0; i < imageWidth; i++) {
					Ray ray = GetRay(i, j);
					for (int sample = 0; sample < samplesPerPixel; ++sample)
					{
						colorAttachment[m] += RayColor(ray, maxDepth, world);
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
		for (int i = 0; i < thred; i++) {
			th[i] = std::thread(castRayMultiThread, i * times, (i + 1) * times);
		}
		for (int i = 0; i < thred; i++) {
			th[i].join();
		}
		LOGI("Render End...");
		WriteColor(imageWidth, imageHeight, colorAttachment, std::string(PROJECT_ROOT"Results/output.png") );
	}
	void Camera::Initialize()
	{
		imageWidth = (imageWidth < 1) ? 1 : imageWidth;
		imageHeight = (imageHeight < 1) ? 1 : imageHeight;
		aspectRatio = float(imageWidth) / float(imageHeight);

		colorAttachment.resize(imageWidth * imageHeight,color(0.f,0.f,0.f));

		pixelSamplesScale = 1.0 / samplesPerPixel;

		center = eye;

		float focalLength = glm::length(eye - lookAt);
		float theta = glm::radians(fovy);
		float h = std::tan(theta / 2.0);

		float viewportHeight = 2 * h * focalLength;
		float viewportWidth = viewportHeight * aspectRatio;

		w = glm::normalize(eye - lookAt);
		u = glm::normalize(glm::cross(up, w));
		v = glm::cross(w, u);

		vec3 viewportU = viewportWidth * u;
		vec3 viewportV = viewportHeight * -v;

		pixelDeltaU = viewportU / (float)imageWidth;
		pixelDeltaV = viewportV / (float)imageHeight;
		vec3 viewportUpperLeft = center - (focalLength * w) - viewportU / 2.f - viewportV / 2.f;
		pixel00Location = viewportUpperLeft + 0.5f * (pixelDeltaU + pixelDeltaV);

	}

	Ray Camera::GetRay(int i, int j) const
	{
		vec3 pixelSample = pixel00Location + (float)i * pixelDeltaU + (float)j * pixelDeltaV;

		vec3 origin = center;
		vec3 direction = pixelSample - origin;

		return Ray(origin, direction);
	}

	color Camera::RayColor(const Ray& ray, int depth, const Hittable& world)
	{
		if (depth <= 0) {
			return color(0.f, 0.f, 0.f);
		}
		HitRecord record;
		if (world.Hit(ray, Interval(0.001, std::numeric_limits<float>::infinity()), record))
		{
			Ray scatteredRay;
			color attenuation;
			if (record.material->Scatter(ray, record, attenuation, scatteredRay)) {
				return attenuation * RayColor(scatteredRay, depth - 1, world);
			}
			return color(0.f, 0.f, 0.f);
		}
		vec3 unitDirection = glm::normalize(ray.direction);
		float a = 0.5 * (unitDirection.y + 1.0f);

		return (1.0f-a) * color(1.0f, 1.0f, 1.0f) + a*color(0.5f, 0.7f, 1.0f);
	}

	color Camera::LinearToGamma(color linearColor)
	{
		color grammaColor = glm::sqrt(linearColor);
		return grammaColor;
	}

	void Camera::WriteColor(int imageWidth, int imageHeight, const std::vector<color>& colorAttachment, const std::string& outputPath)
	{
		std::vector<uint8_t> rawImage(imageHeight * imageWidth * 3);
		for (size_t j = 0; j < imageHeight; ++j) {
			for (size_t i = 0; i < imageWidth; ++i) {
				size_t idx = i + j * imageWidth;
				color gammaColor = LinearToGamma(colorAttachment[idx]);
				static const Interval intensity(0.000f, 0.999f);
				rawImage[idx * 3] = (uint8_t)(intensity.Clamp(gammaColor.r) * 256);
				rawImage[idx * 3 + 1] = (uint8_t)(intensity.Clamp(gammaColor.g) * 256);
				rawImage[idx * 3 + 2] = (uint8_t)(intensity.Clamp(gammaColor.b) * 256);
			}
		}
		stbi_write_png(outputPath.c_str(), imageWidth, imageHeight, 3, rawImage.data(), imageWidth * 3);
	}

	void ShowProgress(int progress)
	{
		LOGI("Left lines: {}", progress);
	}
}