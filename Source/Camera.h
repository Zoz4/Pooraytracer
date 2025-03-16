#pragma once

#include <glm/vec3.hpp>

namespace Pooraytracer {
	using glm::vec3;

	class Ray;
	class Camera {
	public:
		int imageWidth = 100;
		int imageHeight = 100;
		int samplesPerPixel = 10;	// Count of random samples for each pixel
		int maxDepth = 10.f;

		float fovy = 90.f;
		vec3 eye = vec3(0.f, 0.f, 0.f);
		vec3 lookAt = vec3(0.f, 0.f, -1.f);
		vec3 up = vec3(0.f, 1.f, 0.f);

		void Render();

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


	};
}