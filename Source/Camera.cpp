#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

#include "Camera.h"
#include "Ray.h"

namespace Pooraytracer {

	void Camera::Initialize()
	{
		imageWidth = (imageWidth < 1) ? 1 : imageWidth;
		imageHeight = (imageHeight < 1) ? 1 : imageHeight;
		aspectRatio = float(imageWidth) / float(imageHeight);

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

}