#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace VulkanEngine {

	class VEWindow {
	public:
		VEWindow(int width, int height, std::string title);
		~VEWindow();

		// Delete the copy constructor and copy operator
		VEWindow(const VEWindow&) = delete;
		VEWindow& operator=(const VEWindow&) = delete;

		bool Close() { return glfwWindowShouldClose(m_Window); }
		VkExtent2D GetExtent() { return { m_Width, m_Height }; }
		bool WasWindowResized() { return m_FramebufferResized; }
		void ResetWindowResizeFlag() { m_FramebufferResized = false; }

		void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	private:
		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void InitWindow();

	private:
		uint32_t m_Width;
		uint32_t m_Height;
		bool m_FramebufferResized = false;
		GLFWwindow* m_Window;
		std::string m_Title;

	};
}