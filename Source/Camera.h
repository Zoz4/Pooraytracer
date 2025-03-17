#pragma once

#include <glm/vec3.hpp>
#include "HittableList.h"
#include <string>

namespace Pooraytracer {
	using glm::vec3;
	using color = glm::vec3;
	class Ray;
	class Camera {
	public:
		int imageWidth = 100;
		int imageHeight = 100;
		int samplesPerPixel = 1;	// Count of random samples for each pixel
		int maxDepth = 5.f;

		float fovy = 90.f;
		vec3 eye = vec3(0.f, 0.f, 0.f);
		vec3 lookAt = vec3(0.f, 0.f, -1.f);
		vec3 up = vec3(0.f, 1.f, 0.f);

		std::vector <color> colorAttachment;
		void Render(Hittable& world);

	private:
		float aspectRatio;			// Ratio of image width over height
		float pixelSamplesScale;	// 1.0/samplesPerPixel
		vec3 center;
		vec3 pixel00Location;		// Location of pixel 0, 0 (Upper right)
		vec3 pixelDeltaU;			// Offset to pixel to the right
		vec3 pixelDeltaV;			// Offset to pixel below
		vec3 u, v, w;				// Camera frame basis vectors

		void Initialize();
		Ray GetRay(int i, int j) const;
		color RayColor(const Ray& ray, int depth, const Hittable& world);
		
		color LinearToGamma(color linearColor);
		void WriteColor(int imageWidth, int imageHeight, const std::vector<color>& colorAttachment, const std::string& outputPath);
	};

	void ShowProgress(int progress);

}