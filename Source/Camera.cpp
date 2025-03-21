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

	std::mutex mtx;
	void Camera::Render(Hittable& world)
	{
		Initialize();

		LOGI("Render Start...");

		/*
		for (int j = 0; j < imageHeight; ++j) {
			LOGI("Scanlines remaining : {}", imageHeight - j);
			for (int i = 0; i < imageWidth; ++i) {
		      Ray ray = GetRay(i, j);
				color pixelColor(0.f, 0.f, 0.f);
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
		aspectRatio = float(imageWidth) / float(imageHeight);

		colorAttachment.resize(imageWidth * imageHeight, color(0.f,0.f,0.f));

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
		//vec2 offset = SampleSquare();
		//vec3 pixelSample = pixel00Location + ((float)i+offset.x) * pixelDeltaU + ((float)j+offset.y) * pixelDeltaV;
		vec3 pixelSample = pixel00Location + ((float)i) * pixelDeltaU + ((float)j) * pixelDeltaV;
		vec3 origin = center;
		vec3 direction = pixelSample - origin;

		return Ray(origin, direction);
	}

	color Camera::RayColor(const Ray& ray, int depth, const Hittable& world)
	{
		if (depth < 0) {
			return color(0.f, 0.f, 0.f);
		}
		HitRecord record;
		if (!world.Hit(ray, Interval(0.001, std::numeric_limits<float>::infinity()), record))
		{
			return background;
		}
		if (record.material->HasEmission())
		{
			return record.material->GetEmission();
		}
		
		Ray scatteredRay;
		color attenuation; // attenuation =  fr * cosθ / pdf(wi)
		color scatter{0.f};
		HitRecord scatteredRecord;
		if (record.material->Scatter(ray, record, attenuation, scatteredRay))
		{
			scatter = attenuation * RayColor(scatteredRay, depth - 1, world);
		}
		return scatter;

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
				auto &r = linearColor.x;
				auto &g = linearColor.y;
				auto &b = linearColor.z;

				if (r != r) r = 0.0f;
				if (g != g) g = 0.0f;
				if (b != b) b = 0.0f;

				color gammaColor = LinearToGamma(linearColor);
				static const Interval intensity(0.0000f, 0.9999f);
				rawImage[idx * 3] = (uint8_t)(intensity.Clamp(gammaColor.r) * 256);
				rawImage[idx * 3 + 1] = (uint8_t)(intensity.Clamp(gammaColor.g) * 256);
				rawImage[idx * 3 + 2] = (uint8_t)(intensity.Clamp(gammaColor.b) * 256);
			}
		}
		stbi_write_png(outputPath.c_str(), imageWidth, imageHeight, 3, rawImage.data(), imageWidth * 3);
	}

	void ShowProgress(int progress)
	{
		LOGI("Scanlines remaining : {}", progress);
	}
}