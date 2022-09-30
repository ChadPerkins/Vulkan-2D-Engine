#pragma once
#include "VE_Device.h"
#include "VE_SwapChain.h"
#include "VE_Window.h"

#include <cassert>
#include <memory>
#include <vector>

namespace VulkanEngine {

	class VERenderer
	{
	public:
		VERenderer(VEWindow& window, VEDevice& device);
		~VERenderer();

		// Delete the copy constructor and copy operator
		VERenderer(const VERenderer&) = delete;
		VERenderer& operator=(const VERenderer&) = delete;

		VkRenderPass GetSwapChainRenderPass() const { return m_SwapChain->GetRenderPass(); }

		bool IsFrameInProgress() const { return m_IsFrameStarted; }

		VkCommandBuffer GetCurrentCommandBuffer() const 
		{
			assert(m_IsFrameStarted && "Cannot get command buffer when the frame is not in progress.");
			return m_CommandBuffers[m_CurrentFrameIndex];
		}

		uint32_t GetFrameIndex() const
		{
			assert(m_IsFrameStarted && "Cannot get frame index when the frame is not in progress.");
			return m_CurrentFrameIndex;
		}

		VkCommandBuffer BeginFrame();
		void EndFrame();
		void BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void EndSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void CreateCommandBuffers();
		void FreeCommandBuffers();
		void RecreateSwapChain();

	private:
		VEWindow& m_Window;
		VEDevice& m_Device;
		std::unique_ptr<VESwapChain> m_SwapChain;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		uint32_t m_CurrentImageIndex;
		uint32_t m_CurrentFrameIndex = 0;
		bool m_IsFrameStarted = false;
	};
}