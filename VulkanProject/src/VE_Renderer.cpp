#include "VE_Renderer.h"

#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace VulkanEngine {

	VERenderer::VERenderer(VEWindow& window, VEDevice& device)
		: m_Window{window}, m_Device{device}
	{
		RecreateSwapChain();
		CreateCommandBuffers();
	}

	VERenderer::~VERenderer()
	{
		FreeCommandBuffers();
	}

	void VERenderer::RecreateSwapChain()
	{
		auto extent = m_Window.GetExtent();

		while (extent.width == 0 || extent.height == 0)
		{
			extent = m_Window.GetExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Device.Device());

		if (m_SwapChain == nullptr)
		{
			m_SwapChain = std::make_unique<VESwapChain>(m_Device, extent);
		}
		else
		{
			std::shared_ptr<VESwapChain> oldSwapChain = std::move(m_SwapChain);

			m_SwapChain = std::make_unique<VESwapChain>(m_Device, extent, oldSwapChain);

			if (!oldSwapChain->CompareSwapFormats(*m_SwapChain.get()))
			{
				throw std::runtime_error("Swap chain image (or depth) format has changed.");
			}
		}
	}

	void VERenderer::CreateCommandBuffers()
	{
		m_CommandBuffers.resize(VESwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo = {};

		allocInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level					= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool			= m_Device.GetCommandPool();
		allocInfo.commandBufferCount	= static_cast<uint32_t>(m_CommandBuffers.size());

		if (vkAllocateCommandBuffers(m_Device.Device(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate command buffers.");
		}
	}

	void VERenderer::FreeCommandBuffers() 
	{
		vkFreeCommandBuffers(m_Device.Device(),
			m_Device.GetCommandPool(),
			static_cast<uint32_t>(m_CommandBuffers.size()),
			m_CommandBuffers.data());

		m_CommandBuffers.clear();
	}

	VkCommandBuffer VERenderer::BeginFrame()
	{
		assert(!m_IsFrameStarted && "Can't call BeginFrame while it's already in progress.");

		auto result = m_SwapChain->AcquireNextImage(&m_CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to aquire the next swap chain image");
		}

		m_IsFrameStarted = true;
		
		auto commandBuffer = GetCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo = {};

		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to begin recording command buffer.");
		}

		return commandBuffer;
	}

	void VERenderer::EndFrame()
	{
		assert(m_IsFrameStarted && "Can't call EndFrame while a frame is not in progress.");

		auto commandBuffer = GetCurrentCommandBuffer();

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer.");
		}

		auto result = m_SwapChain->SubmitCommandBuffers(&commandBuffer, &m_CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window.WasWindowResized())
		{
			m_Window.ResetWindowResizeFlag();
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to aquire the next swap chain image");
		}

		m_IsFrameStarted	= false;
		m_CurrentFrameIndex	= (m_CurrentFrameIndex + 1) % VESwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void VERenderer::BeginSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(m_IsFrameStarted && "Can't call BeginSwapChainRenderPass while a frame is not in progress.");
		assert(commandBuffer == GetCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

		VkRenderPassBeginInfo renderPassInfo = {};

		renderPassInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass			= m_SwapChain->GetRenderPass();
		renderPassInfo.framebuffer			= m_SwapChain->GetFrameBuffer(m_CurrentImageIndex);

		renderPassInfo.renderArea.offset	= { 0, 0 };
		renderPassInfo.renderArea.extent	= m_SwapChain->GetSwapChainExtent();

		std::array<VkClearValue, 2> clearValues = {};

		clearValues[0].color				= { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil			= { 1.0f, 0 };

		renderPassInfo.clearValueCount		= static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues			= clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {};

		viewport.x							= 0.0f;
		viewport.y							= 0.0f;
		viewport.width						= static_cast<float>(m_SwapChain->GetSwapChainExtent().width);
		viewport.height						= static_cast<float>(m_SwapChain->GetSwapChainExtent().height);
		viewport.minDepth					= 0.0f;
		viewport.maxDepth					= 1.0f;

		VkRect2D scissor{ {0, 0}, m_SwapChain->GetSwapChainExtent() };

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	}

	void VERenderer::EndSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(m_IsFrameStarted && "Can't call EndSwapChainRenderPass while a frame is not in progress.");
		assert(commandBuffer == GetCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

		vkCmdEndRenderPass(commandBuffer);

	}

}