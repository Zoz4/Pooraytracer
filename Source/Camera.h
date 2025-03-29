#pragma once

#include <glm/vec3.hpp>
#include "HittableList.h"
#include <string>

namespace Pooraytracer {
	using vec3 = glm::dvec3;
	using color = glm::dvec3;
	class Ray;
	class Camera {

	public:
		int imageWidth = 100;
		int imageHeight = 100;
		int samplesPerPixel = 1;	// Count of random samples for each pixel
		int threadNums = 16;
		int maxDepth = 10;
		color background = color(0.,0.,0.);

		double fovy = 90.;
		vec3 eye = vec3(0., 0., 0.);
		vec3 lookAt = vec3(0., 0., -1.);
		vec3 up = vec3(0., 1., 0.);

		std::vector <color> colorAttachment;
		void Render(Hittable& world, Hittable& lights);
		void WriteColorAttachment(const std::string& outputPath) const;
		std::string GetParametersStr() const;
		void SetViewParametersByXmlFile(const std::string& xmlFilePath);

		bool bSampleLights = true;
		double russianRoulette = 0.8;

	private:
		double aspectRatio;			// Ratio of image width over height
		double pixelSamplesScale;	// 1.0/samplesPerPixel
		vec3 center;
		vec3 pixel00Location;		// Location of pixel 0, 0 (Upper right)
		vec3 pixelDeltaU;			// Offset to pixel to the right
		vec3 pixelDeltaV;			// Offset to pixel below
		vec3 u, v, w;				// Camera frame basis vectors

		void Initialize();
		Ray GetRay(int i, int j) const;
		color RayColor(const Ray& ray, int depth, const Hittable& world, const Hittable& lights);

		color LinearToSRGB(color linearColor) const;
		double LinearToSRGB(double linearColorComponent) const;
	};

	void ShowProgress(int progress);

}