#include "Texture.h"
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Logger.h"

namespace Pooraytracer {
	ImageTexture::ImageTexture(const std::string& imagePath)
	{
		unsigned char* rawData = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
		if (!rawData){
			LOGE("Loading Texture: {} Failed!!", imagePath);
			return;
		}
		size_t dataSize = width * height * channels;
		data = std::make_shared<std::vector<unsigned char>>(rawData, rawData + dataSize);

		stbi_image_free(rawData);
	}
	color ImageTexture::Value(double u, double v, const point3& p) const
	{
		if (!data || data->empty()) return color(0., 1., 1.);

		u = std::clamp(u, 0.0, 1.0);
		v = std::clamp(v, 0.0, 1.0);

		double x = u * (width - 1.);
		double y = (1. - v) * (height - 1.);

		int x0 = static_cast<int>(x);
		int y0 = static_cast<int>(y);
		int x1 = std::min(x0 + 1, width - 1);
		int y1 = std::min(y0 + 1, height - 1);

		double tx = x - x0;
		double ty = y - y0;

		color c00 = GetPixel(x0, y0);
		color c10 = GetPixel(x1, y0);
		color c01 = GetPixel(x0, y1);
		color c11 = GetPixel(x1, y1);

		color c0 = c00 * (1 - tx) + c10 * tx;
		color c1 = c01 * (1 - tx) + c11 * tx;
		return c0 * (1 - ty) + c1 * ty;

	}
	color ImageTexture::GetPixel(int x, int y) const
	{
		const double colorScale = 1.0 / 255.0;
		int idx = (y * width + x) * channels;

		if (channels >= 3) {
			return color(
				SRGBToLinear(colorScale * (*data)[idx]),
				SRGBToLinear(colorScale * (*data)[idx + 1]),
				SRGBToLinear(colorScale * (*data)[idx + 2])
			);
		}
		else {
			return color(colorScale * (*data)[idx]);
		}
	}
	double ImageTexture::SRGBToLinear(double colorComponent) const
	{
		if (colorComponent <= 0.04045)
			return colorComponent * (1. / 12.92);
		return std::pow((colorComponent + 0.055) * (1. / 1.055), 2.4);
	}
}