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
	// cornell-box
	// veach-mis
	// bathroom2
	const std::string fileName = "bathroom2";
	const std::string filePath = RESOURCES_DIR + fileName;
	// .obj file path filePath/fileName.obj
	// .xml file path filePath/fileName.xml
	LOGI("{}", fileName);
	LOGI("{}", filePath);

	Camera camera;
	camera.bSampleLights = true;
	camera.russianRoulette = 0.8;
	camera.samplesPerPixel = 100;
	camera.maxDepth = 100;
	camera.threadNums = 16;
	camera.background = color(0.0, 0.0, 0.0);
	camera.SetViewParametersByXmlFile(filePath + "/" + fileName + ".xml");

	std::shared_ptr<Model> model = std::make_shared<Pooraytracer::Model>(filePath, fileName);

	LOGI("Building BVH...");
	HittableList world;
	HittableList lights;
	for (auto& mesh : model->meshes) {
		world.Add(make_shared<BVHNode>(mesh));
		if (mesh->material->HasEmission()) {
			lights.Add(make_shared<BVHNode>(mesh));
		}
	}
	world = HittableList(make_shared<BVHNode>(world));
	lights = HittableList(make_shared<BVHNode>(lights));
	LOGI("Building BVH End...");

	auto startTime = std::chrono::steady_clock::now();
	camera.Render(world, lights);
	std::string executionTime = GetExecutionTimeInMinutes(startTime);

	camera.WriteColorAttachment(PROJECT_ROOT"Results/" + fileName + "_" + GetTimestamp() + "_" + camera.GetParametersStr() + "_" + executionTime + ".png");

	return 0;
}