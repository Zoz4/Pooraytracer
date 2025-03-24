#include "Model.h"
#include "Logger.h"


#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <tinyxml2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>

namespace Pooraytracer {

	const std::unordered_map<std::string, MaterialType> Model::materialTypeMap = {
		{"material0", MaterialType::PhoneReflectance },
		{"material1", MaterialType::PhoneReflectance },
		{"material2", MaterialType::PhoneReflectance },
		{"material3", MaterialType::PhoneReflectance },
		{"material4", MaterialType::PhoneReflectance },
		{"light1", MaterialType::DiffuseLight},
		{"light2", MaterialType::DiffuseLight},
		{"light3", MaterialType::DiffuseLight},
		{"light4", MaterialType::DiffuseLight},

		{"DiffuseWhite", MaterialType::Lambertian }, // Top, Back & Bottom
		{"DiffuseBall", MaterialType::Lambertian },
		{"DiffuseYellow", MaterialType::Lambertian }, // Not Used!
		{"LeftWall", MaterialType::Lambertian },
		{"RightWall", MaterialType::Lambertian },
		{"Light", MaterialType::DiffuseLight },

		{"Wall", MaterialType::PhoneReflectance },
		{"quad1", MaterialType::PhoneReflectance },
		{"Mirror", MaterialType::PhoneReflectance },
		{"StainlessRough", MaterialType::PhoneReflectance },
		{"Towel", MaterialType::PhoneReflectance },
		{"BlackWoodLacquer", MaterialType::PhoneReflectance },
		{"Wood", MaterialType::PhoneReflectance },
		{"WoodFloor", MaterialType::PhoneReflectance },
		{"RoughGlass", MaterialType::PhoneReflectance },
		{"Plastic", MaterialType::PhoneReflectance },
		{"DarkPlastic", MaterialType::PhoneReflectance },
		{"Bin", MaterialType::PhoneReflectance },
		{"WallRight", MaterialType::PhoneReflectance },
		{"DarkBorder", MaterialType::PhoneReflectance },
		{"Trims", MaterialType::PhoneReflectance },
		{"Ceramic", MaterialType::PhoneReflectance }
	};

	Model::Model(const std::string& modelDirectory, const std::string& modelName) :modelDirectory(modelDirectory), modelName(modelName)
	{

		const std::string& modelPath = modelDirectory + "/" + modelName + ".obj";

		ProcessObjFile(modelPath);

		InitializeLightsRadiance();

		tinyobj::ObjReaderConfig config;

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(modelPath, config)) {
			if (!reader.Error().empty()) {
				LOGE("TinyObjReader: {}", reader.Error());
			}
		}
		if (!reader.Warning().empty()) {
			LOGI("TinyObjReader: {}", reader.Warning());
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		auto& materials = reader.GetMaterials();

		// Loading Textures...
		for (const auto& material : materials) {
			std::string mtlName = material.name;
			if (!material.diffuse_texname.empty()) {
				std::string texName = material.diffuse_texname;
				std::shared_ptr<Texture> imageTexture = std::make_shared<ImageTexture>(modelDirectory + "/" + texName);
				if (imageTextureInstances.find(texName) == imageTextureInstances.end()) {
					imageTextureInstances.insert({ texName, imageTexture });
				}
			}
			if (!material.specular_texname.empty()) {
				std::string texName = material.diffuse_texname;
				std::shared_ptr<Texture> imageTexture = std::make_shared<ImageTexture>(modelDirectory + "/" + texName);
				if (imageTextureInstances.find(texName) == imageTextureInstances.end()) {
					imageTextureInstances.insert({ texName, imageTexture });
				}
			}
		}

		// Creating Material Instances...
		for (const auto& material : materials) {
			std::string mtlName = material.name;
			if (materialInstances.find(mtlName) == materialInstances.end()) {
				
				std::shared_ptr<Material> materialInstance = CreateMaterial(material);
				materialInstances.insert({ mtlName, materialInstance });
			}
			else
			{
				LOGW("Some Materials Have the Same Name:{} ! ", mtlName);
			}
		}

		// Loop over shapes
		LOGI("Shapes/Meshes Nums: {}", shapes.size());
		LOGI("Materials Nums: {}", materials.size());
		for (size_t s = 0; s < shapes.size(); s++)
		{
			// Each face in the group has a same material
			auto material_idx = shapes[s].mesh.material_ids[0];
			std::string material_name = materials[material_idx].name;
			auto& materialRaw = materials[material_idx];

			LOGI("Shapes/Meshes[{}] Name:{}, Faces Nums:{}, Material Name:{}",
				s, shapes[s].name,
				shapes[s].mesh.num_face_vertices.size(),
				material_name
			);
			std::shared_ptr<Material> material = materialInstances.at(material_name);

			// Loop over faces(polygon)
			size_t index_offset = 0;
			std::vector<std::shared_ptr<Hittable>> meshTriangles;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
				std::array<vec3, 3> vertices;
				std::array<vec2, 3> texCoords;
				if (fv != 3) {
					LOGE("Only Support Triangle Mesh!");
					break;
				}

				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++) {
					// Access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
					tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
					tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
					vertices[v] = { vx, vy, vz };
					//tinyobj::real_t nx, ny, nz;
					//// Check if `normal_index` is zero or positive. negative = no normal data
					//if (idx.normal_index >= 0) {
					//    nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					//    ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					//    nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
					//}

					tinyobj::real_t tx{}, ty{};
					// Check if `texcoord_index` is zero or positive. negative = no texcoord data
					if (idx.texcoord_index >= 0) {
						tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
						ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
					}
					texCoords[v] = { tx, ty };
				}
				if (texCoords[0] == texCoords[1] || texCoords[1] == texCoords[2] || texCoords[0] == texCoords[2])
				{
					//LOGW("The texture coordinates of the triangle's vertices are the same");
					//LOGW("So we set them to a fixed value to ensure the correct calculation of the tangent.")
					texCoords[0] = { 0, 0 }; texCoords[1] = { 1, 0 }; texCoords[2] = { 1, 1 };
				}
				index_offset += fv;

				// per-face material
				//auto material_idx = shapes[s].mesh.material_ids[f];
				//material_name = materials[material_idx].name;
				//auto& materialRaw = materials[material_idx];
				//vec3 albedo = vec3(materialRaw.diffuse[0], materialRaw.diffuse[1], materialRaw.diffuse[2]);
				////LOGI("Material Name: {}", material_name);
				//std::shared_ptr<Material> material = make_shared<Lambertian>(albedo);// [TODO]: Implentment Material Initialize

				std::shared_ptr<Triangle> triangle = make_shared<Triangle>(vertices, texCoords, material);
				meshTriangles.push_back(triangle);

			}// End of a face
			meshes.push_back(make_shared<Mesh>(shapes[s].name, meshTriangles, material));
		}// End of a Shape/Mesh

	}

	bool Model::ProcessObjFile(const std::string& modelPath)
	{

		std::ifstream inFile(modelPath);
		if (!inFile.is_open()) {
			LOGE("Open & Process Obj File Failed");
			return false;
		}

		std::vector<std::string> lines;
		std::string line;
		while (std::getline(inFile, line)) {
			lines.push_back(line);
		}
		inFile.close();

		std::unordered_set<std::string> groupNames;
		int anonymousGroupCount = -1;

		// For each line
		for (size_t i = 0; i < lines.size(); i++) {
			std::string& currentLine = lines[i];

			// If a line defines a group
			if (currentLine.substr(0, 2) == "g " || currentLine == "g") {

				// If group name is empty
				if (currentLine == "g" || currentLine.size() <= 2 ||
					currentLine.find_first_not_of(" ", 2) == std::string::npos) {

					// Generate an unique name
					std::string newGroupName;
					do {
						newGroupName = "Group_" + std::to_string(++anonymousGroupCount);
					} while (groupNames.find(newGroupName) != groupNames.end());

					// Update line
					lines[i] = "g " + newGroupName;
					groupNames.insert(newGroupName);
					LOGI("Rename empty group name to {}", newGroupName);
				}
				else {
					// If group name is not empty 
					std::string groupName = currentLine.substr(2);
					// Remove space
					size_t start = groupName.find_first_not_of(" ");
					if (start != std::string::npos) {
						size_t end = groupName.find_last_not_of(" ");
						groupName = groupName.substr(start, end - start + 1);
						groupNames.insert(groupName);
					}
					else {
						// If group name is ' '
						std::string newGroupName;
						do {
							newGroupName = "Group_" + std::to_string(++anonymousGroupCount);
						} while (groupNames.find(newGroupName) != groupNames.end());

						lines[i] = "g " + newGroupName;
						groupNames.insert(newGroupName);
						LOGI("Rename empty group name to {}", newGroupName);
					}
				}
			}
		}

		if (anonymousGroupCount != -1)
		{
			// Write results
			std::ofstream outFile(modelPath);
			if (!outFile.is_open()) {
				LOGE("Save Failed!");
				return false;
			}
			for (const auto& outputLine : lines) {
				outFile << outputLine << std::endl;
			}
			outFile.close();
		}
		LOGI("Process Success!")
			return true;
	}

	std::shared_ptr<Material> Model::CreateMaterial(const tinyobj::material_t& materialRaw) const
	{
		const std::string mtlname = materialRaw.name;

		MaterialType type = MaterialType::Lambertian;
		if (materialTypeMap.contains(mtlname)) {
			type = materialTypeMap.at(mtlname);
		}

		switch (type)
		{
		case MaterialType::PhoneReflectance: {
			return CreatePhoneReflectanceMaterial(materialRaw);
			break;
		}
		case MaterialType::DiffuseLight: {
			color radiance = lightRadianceMap.at(mtlname);
			return make_shared<DiffuseLight>(radiance);
			break;
		}
		case MaterialType::Lambertian: {
			vec3 albedo = vec3(materialRaw.diffuse[0], materialRaw.diffuse[1], materialRaw.diffuse[2]);
			return make_shared<Lambertian>(albedo);
			break;
		}
		case MaterialType::DebugMaterial: {
			color albedo = vec3(materialRaw.diffuse[0], materialRaw.diffuse[1], materialRaw.diffuse[2]);
			return make_shared<DebugMaterial>(albedo);
			break;
		}
		default:
			return CreatePhoneReflectanceMaterial(materialRaw);
			break;
		}
	}

	void Model::InitializeLightsRadiance()
	{
		const std::string xmlFilePath = modelDirectory + "/" + modelName + ".xml";
		tinyxml2::XMLDocument doc;

		if (doc.LoadFile(xmlFilePath.c_str()) != tinyxml2::XML_SUCCESS) {
			LOGI("Failed to load XML file: {}", xmlFilePath);
		}
		auto ParseRadianceStr = [](const std::string& radianceStr)->color {
			double x = 0., y = 0., z = 0.;
			std::stringstream ss(radianceStr);
			char comma;
			ss >> x >> comma >> y >> comma >> z;
			return color(x, y, z);
			};

		for (tinyxml2::XMLElement* lightElement = doc.FirstChildElement("light"); lightElement != nullptr; lightElement = lightElement->NextSiblingElement("light")) {
			const char* mtlname = lightElement->Attribute("mtlname");
			const char* radianceStr = lightElement->Attribute("radiance");
			if (mtlname && radianceStr) {
				lightRadianceMap[mtlname] = ParseRadianceStr(radianceStr);
				LOGI("Light: {} radiance: {}", mtlname, glm::to_string(lightRadianceMap[mtlname]));
			}
			else {
				LOGI("Something Wrong!");
			}
		}

	}

	std::shared_ptr<Material> Model::CreatePhoneReflectanceMaterial(const tinyobj::material_t& materialRaw) const
	{
		if (!materialRaw.diffuse_texname.empty())
		{
			const std::string& texName = materialRaw.diffuse_texname;
			LOGI("Texture name: {}", materialRaw.diffuse_texname);
			std::shared_ptr<Texture> imageTextureInstance = imageTextureInstances.at(texName);
			color Ks = vec3(materialRaw.specular[0], materialRaw.specular[1], materialRaw.specular[2]);
			double Ns = materialRaw.shininess;
			return make_shared<PhoneReflectance>(imageTextureInstance, Ks, Ns);
		}
		else {
			color Kd = vec3(materialRaw.diffuse[0], materialRaw.diffuse[1], materialRaw.diffuse[2]);
			color Ks = vec3(materialRaw.specular[0], materialRaw.specular[1], materialRaw.specular[2]);
			double Ns = materialRaw.shininess;
			return make_shared<PhoneReflectance>(Kd, Ks, Ns);
		}
	}
}