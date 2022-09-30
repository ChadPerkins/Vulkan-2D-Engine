#pragma once
#include "VE_Device.h"

#include <string>
#include <vector>

namespace VulkanEngine {

	struct PipelineConfigInfo
	{
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		VkPipelineViewportStateCreateInfo ViewportInfo;
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo RasterizationInfo;
		VkPipelineMultisampleStateCreateInfo MultisampleInfo;
		VkPipelineColorBlendAttachmentState ColorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo ColorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo DepthStencilInfo;
		std::vector<VkDynamicState> DynamicStateEnables;
		VkPipelineDynamicStateCreateInfo DynamicStateInfo;
		VkPipelineLayout PipelineLayout		= nullptr;
		VkRenderPass RenderPass				= nullptr;
		uint32_t Subpass					= 0;
	};

	class VEPipeline
	{
	public:
		VEPipeline(VEDevice& device,
			const std::string& vertShaderPath,
			const std::string& fragShaderPath,
			const PipelineConfigInfo& configInfo);
		~VEPipeline();

		VEPipeline(const VEPipeline&) = delete;
		VEPipeline& operator=(const VEPipeline&) = delete;

		void Bind(VkCommandBuffer commandBuffer);

		static void DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

	private:
		static std::vector<char> ReadFile(const std::string& filepath);

		void CreateGraphicsPipeline(const std::string& vertShaderPath,
			const std::string& fragShaderPath,
			const PipelineConfigInfo& configInfo);

		void CreateShaderModule(const std::vector<char>& shader, VkShaderModule* shaderModule);

	private:
		VEDevice& m_Device;
		VkPipeline m_GraphicsPipeline;
		VkShaderModule m_VertShaderModule;
		VkShaderModule m_FragShaderModule;
	};
}


