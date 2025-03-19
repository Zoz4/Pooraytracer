#include "Source/Logger.h"
#include "Source/RandomNumberGenerator.h"
#include "Source/Model.h"
#include "Source/BVH.h"
#include "Source/Camera.h"

int main(void)
{
	//spdlog::set_pattern(LOGGER_FORMAT);
	spdlog::set_level(spdlog::level::level_enum(SPDLOG_ACTIVE_LEVEL));
	
	using namespace Pooraytracer;

	LOGI("Hello Pooraytracer!");

	const std::string fileName = "veach-mis";
	LOGI("{}", fileName);
	std::shared_ptr<Model> model = 
		std::make_shared<Pooraytracer::Model>(RESOURCES_DIR+fileName, fileName);

	LOGI("Building BVH...");
	HittableList world;
	for (auto& mesh : model->meshes) {
		world.Add(make_shared<BVHNode>(mesh));
	}
	world = HittableList(make_shared<BVHNode>(world));
	LOGI("Building BVH End...");

	Camera camera;

	// cornell-box
	//camera.eye = glm::vec3(278.0f, 273.0f, -800.0f);
	//camera.lookAt = glm::vec3(278.0f, 273.0f, -799.0f);
	//camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	//camera.fovy = 39.3077f;
	//camera.imageWidth = 100;
	//camera.imageHeight = 100;
	//camera.samplesPerPixel = 10;
	
	// veach-mis
	camera.eye = glm::vec3(28.2792f, 5.2f, 1.23612e-06f);
	camera.lookAt = glm::vec3(0.0f, 2.8f, 0.0f);
	camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	camera.fovy = 20.1143f;
	camera.background = color(0.0, 0.0, 0.0);
	camera.imageWidth = 1280;
	camera.imageHeight = 720;
	camera.samplesPerPixel = 50;

	// veach-mis for test
	//camera.eye = glm::vec3(28.2792f, 5.2f, 1.23612e-06f);
	//camera.lookAt = glm::vec3(0.0f, 2.8f, 0.0f);
	//camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	//camera.fovy = 20.1143f;
	//camera.background = color(0.0, 0.0, 0.0);
	//camera.imageWidth = 320;
	//camera.imageHeight = 180;
	//camera.samplesPerPixel = 1;

	camera.Render(world);

	return 0;
}