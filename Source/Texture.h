#pragma once

#include <string>
#include <memory>
#include <glm/vec3.hpp>

namespace Pooraytracer {

	using color = glm::dvec3;
	using point3 = glm::dvec3;

	class Texture {
	public:
		virtual ~Texture() = default;

		virtual color Value(double u, double v, const point3& p) const = 0;
	};

	class SolidColor : public Texture {
	public:

		SolidColor(const color& albedo) :albedo(albedo) {}
		color Value(double u, double v, const point3& p) const override {
			return albedo;
		}
	private:
		color albedo;
	};

	class ImageTexture :public Texture {

	public:
		ImageTexture(const std::string& imagePath);
		color Value(double u, double v, const point3& p) const override;
	private:
		std::shared_ptr<std::vector<unsigned char>> data;
		int width;
		int height;
		int channels;
		color GetPixel(int x, int y) const;
		double SRGBToLinear(double colorComponent) const;

	};
}