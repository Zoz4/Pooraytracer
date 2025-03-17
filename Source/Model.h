#pragma once

#include "Triangle.h"

namespace Pooraytracer {

	class Model {
	public:
		Model() = default;
		Model(const std::string& modelPath);

		std::vector< std::shared_ptr<Mesh>> meshes;
	private:
		bool ProcessObjFile(const std::string& modelPath);
	};
}