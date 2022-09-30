#pragma once
#include "VE_Device.h"
#include "VE_GameObject.h"
#include "VE_Window.h"
#include "VE_Renderer.h"

#include <memory>
#include <vector>

const uint32_t WINDOW_WIDTH = 1280;
const uint32_t WINDOW_HEIGHT = 720;
const std::string WINDOW_TITLE = "Vulkan Application";

namespace VulkanEngine {

	class Application
	{
	public:
		Application();
		~Application();

		// Delete the copy constructor and copy operator
		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		void Run();

	private:
		void LoadGameObjects();

		// Recursive triangle effect
		void Sierpinski(std::vector<VEModel::Vertex>& vertices,
			uint32_t depth,
			glm::vec2 top,
			glm::vec2 right,
			glm::vec2 left);

	private:
		VEWindow window{ WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE };
		VEDevice device{ window };
		VERenderer renderer{ window, device };
		std::vector<VEGameObject> gameObjects;;
	};
}