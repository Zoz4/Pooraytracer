#include "Source/Logger.h"
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
	//camera.background = color(0.0, 0.0, 0.0);
	//camera.imageWidth =  1024;
	//camera.imageHeight = 1024 ;
	//camera.samplesPerPixel = 500;
	//camera.maxDepth = 50;
	//camera.threadNums = 32;

	// cornell-box for debug
	//camera.eye = glm::vec3(278.0f, 273.0f, -800.0f);
	//camera.lookAt = glm::vec3(278.0f, 273.0f, -799.0f);
	//camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	//camera.fovy = 39.3077f;
	//camera.background = color(0.0, 0.0, 0.0);
	//camera.imageWidth = 256;
	//camera.imageHeight = 256;
	//camera.samplesPerPixel = 10;
	//camera.maxDepth = 5;
	//camera.threadNums = 16;
	
	// veach-mis
	camera.eye = glm::vec3(28.2792f, 5.2f, 1.23612e-06f);
	camera.lookAt = glm::vec3(0.0f, 2.8f, 0.0f);
	camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	camera.fovy = 20.1143f;
	camera.background = color(0.0, 0.0, 0.0);
	camera.imageWidth = 1280;
	camera.imageHeight = 720;
	camera.samplesPerPixel = 500;
	camera.maxDepth = 50;
	camera.threadNums = 16;

	// veach-mis for debug
	//camera.eye = glm::vec3(28.2792f, 5.2f, 1.23612e-06f);
	//camera.lookAt = glm::vec3(0.0f, 2.8f, 0.0f);
	//camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	//camera.fovy = 20.1143f;
	//camera.background = color(0.0, 0.0, 0.0);
	//camera.imageWidth = 128;
	//camera.imageHeight = 72;
	//camera.samplesPerPixel = 10;
	//camera.threadNums = 16;


	camera.Render(world);
	camera.WriteColorAttachment(PROJECT_ROOT"Results/" + fileName + "_" + GetTimestamp() + ".png");

	return 0;
}