#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "../../scene-window-system/TestConfiguration.h"
#include "../scene-window-system/Scene.h"
#include "VulkanApplication.h"

const int WIDTH = 800;
const int HEIGHT = 600;

// Called from main(). Runs this example.
int runVulkanTest(const TestConfiguration& configuration) 
{
	auto cubeCountPerDim = configuration.cubeDimension;
	auto paddingFactor = configuration.cubePadding;

	Window win = Window(GetModuleHandle(nullptr), "VulkanTest", "Vulkan Test", WIDTH, HEIGHT);

	Camera camera = Camera::Default();
	auto heightFOV = camera.FieldOfView() / win.aspectRatio();
	auto base = (cubeCountPerDim + (cubeCountPerDim - 1) * paddingFactor) / 2.0f;
	auto camDistance = base / std::tan(heightFOV / 2);
	float z = camDistance + base + camera.Near();

	camera.SetPosition({ 0.0f, 0.0f, z, 1.0f });
	auto magicFactor = 2;
	camera.SetFar(magicFactor * (z + base + camera.Near()));
	auto scene = Scene(camera, cubeCountPerDim, paddingFactor);

	VulkanApplication app(scene, win);

	try {
		app.run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char *argv[]) {

	std::stringstream arg;

	for (auto i = 0; i < argc; ++i) {
		arg << argv[i] << " "; 
	}

	TestConfiguration::SetTestConfiguration(arg.str().c_str());
	auto& conf = TestConfiguration::GetInstance();
	runVulkanTest(conf);
}