#pragma once

#include "Triangle.h"
#include "Material.h"
#include <unordered_map>

namespace tinyobj {
	struct material_t;
}

namespace Pooraytracer {

	class Model {
	public:
		Model() = default;
		Model(const std::string& modelDirectory, const std::string& modelName);

		std::vector< std::shared_ptr<Mesh>> meshes;
	private:
		std::string modelDirectory;
		std::string modelName;
		bool ProcessObjFile(const std::string& modelPath);
		std::shared_ptr<Material> CreateMaterial(const tinyobj::material_t& materialRaw) const;
		std::unordered_map<std::string, color> lightRadianceMap;
		void InitializeLightsRadiance();

		static const std::unordered_map<std::string, MaterialType> materialTypeMap;
	};
}