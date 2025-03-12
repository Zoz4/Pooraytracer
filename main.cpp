#include <glm/gtx/string_cast.hpp>
#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <tinyxml2.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vector>
#include <cmath>
#include <string>

#include "Source/Logger.h"

int main()
{
	spdlog::set_level(spdlog::level::level_enum(SPDLOG_ACTIVE_LEVEL));

	LOGI("Hello Pooraytracer !");
	{
		glm::vec3 vec(1.0f, 2.0f, 3.0f);
		LOGI("Vector: {}", glm::to_string(vec));
		
		std::string fileName = "cornell-box";
		std::string filePath = RESOURCES_DIR+fileName+"/";
		

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (filePath+fileName+".obj").c_str(), filePath.c_str());

		if (!warn.empty()) LOGW(warn.c_str());
		if (!err.empty())  LOGW(err.c_str());
		if (!ret){
			LOGI("Failed to load OBJ file!");
		} else {
			LOGI("Loaded {} vertices", attrib.vertices.size() / 3);
			for (int i = 0; i < std::min(3ull, attrib.vertices.size() / 3); ++i) {
				glm::vec3 position = { attrib.vertices[i * 3 + 0], attrib.vertices[i * 3 + 1], attrib.vertices[i * 3 + 2] };
				LOGI("Position[{}]: {}", i, glm::to_string(position));
			}
			LOGI("Loaded {} materials", materials.size());
		}


		tinyxml2::XMLDocument doc;
		if (doc.LoadFile( (filePath+ fileName+".xml").c_str() ) != tinyxml2::XML_SUCCESS) {
			LOGE("Failed to load XML file: {}", RESOURCES_DIR"cornell-box/cornell-box.xml");
		}
		else {
			
			tinyxml2::XMLElement* camera = doc.FirstChildElement("camera");
			if (camera) {
				const char* type = camera->Attribute("type");
				int width, height;
				float fovy;
				camera->QueryIntAttribute("width", &width);
				camera->QueryIntAttribute("height", &height);
				camera->QueryFloatAttribute("fovy", &fovy);
				LOGI("Camera Type: {}", (type ? type : "Unknown"));
				LOGI("Resolution: {}x{}", width, height);
				LOGI("FOV Y: {}", fovy);

				tinyxml2::XMLElement* eye = camera->FirstChildElement("eye");
				if (eye) {
					float x, y, z;
					eye->QueryFloatAttribute("x", &x);
					eye->QueryFloatAttribute("y", &y);
					eye->QueryFloatAttribute("z", &z);
					LOGI("Eye Position: ({}, {}, {})", x, y, z);
				}
				tinyxml2::XMLElement* lookat = camera->FirstChildElement("lookat");
				if (lookat) {
					float x, y, z;
					lookat->QueryFloatAttribute("x", &x);
					lookat->QueryFloatAttribute("y", &y);
					lookat->QueryFloatAttribute("z", &z);
					LOGI("lookat: ({}, {}, {})", x, y, z);
				}
				tinyxml2::XMLElement* up = camera->FirstChildElement("up");
				if (up) {
					float x, y, z;
					up->QueryFloatAttribute("x", &x);
					up->QueryFloatAttribute("y", &y);
					up->QueryFloatAttribute("z", &z);
					LOGI("up: ({}, {}, {})", x, y, z);
				}
			}


			tinyxml2::XMLElement* light = doc.FirstChildElement("light");
			if (light) {
				const char* mtlname = light->Attribute("mtlname");
				const char* radianceStr = light->Attribute("radiance");
				LOGI("Light Material : {}", (mtlname ? mtlname : "Unknown"));
				if (radianceStr) {
					float r, g, b;
					std::stringstream ss(radianceStr);
					char comma;
					ss >> r >> comma >> g >> comma >> b;
					LOGI("Radiance: ({}, {}, {})",r, g, b);
				}
			}
		}

		int width, height, channels;
		const char* filename = RESOURCES_DIR"bathroom2/textures/Label.png";
		unsigned char* img = stbi_load(filename, &width, &height, &channels, 0);
		if (!img) {
			LOGI("Failed to load image.");
		}
		else {
			LOGI("Image:    {}", filename);
			LOGI("width:    {}", width);
			LOGI("height:   {}", height);
			LOGI("channels: {}",channels);
		}
	}

	return 0;
}
