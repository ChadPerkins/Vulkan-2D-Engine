#include "VE_SwapChain.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace VulkanEngine {

    VESwapChain::VESwapChain(VEDevice& deviceRef, VkExtent2D extent)
        : m_Device{ deviceRef }, m_WindowExtent{ extent }
    {
        Init();
    }

    VESwapChain::VESwapChain(VEDevice& deviceRef, VkExtent2D extent, std::shared_ptr<VESwapChain> previous)
        : m_Device{ deviceRef }, m_WindowExtent{ extent }, m_OldSwapChain{ previous }
    {
        Init();

        // Clean up old SwapChain since it's no longer needed
        m_OldSwapChain = nullptr;
    }

    void VESwapChain::Init()
    {
        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateDepthResources();
        CreateFramebuffers();
        CreateSyncObjects();
    }

    VESwapChain::~VESwapChain()
    {
        for (auto imageView : m_SwapChainImageViews)
        {
            vkDestroyImageView(m_Device.Device(), imageView, nullptr);
        }
        m_SwapChainImageViews.clear();

        if (m_SwapChain != nullptr)
        {
            vkDestroySwapchainKHR(m_Device.Device(), m_SwapChain, nullptr);
            m_SwapChain = nullptr;
        }

        for (int i = 0; i < m_DepthImages.size(); i++)
        {
            vkDestroyImageView(m_Device.Device(), m_DepthImageViews[i], nullptr);
            vkDestroyImage(m_Device.Device(), m_DepthImages[i], nullptr);
            vkFreeMemory(m_Device.Device(), m_DepthImageMemorys[i], nullptr);
        }

        for (auto framebuffer : m_SwapChainFramebuffers)
        {
            vkDestroyFramebuffer(m_Device.Device(), framebuffer, nullptr);
        }

        vkDestroyRenderPass(m_Device.Device(), m_RenderPass, nullptr);

        // cleanup synchronization objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(m_Device.Device(), m_RenderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_Device.Device(), m_ImageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_Device.Device(), m_InFlightFences[i], nullptr);
        }
    }

    VkResult VESwapChain::AcquireNextImage(uint32_t* imageIndex)
    {
        vkWaitForFences(m_Device.Device(),
            1,
            &m_InFlightFences[m_CurrentFrame],
            VK_TRUE,
            std::numeric_limits<uint64_t>::max());

        VkResult result = vkAcquireNextImageKHR(m_Device.Device(),
            m_SwapChain,
            std::numeric_limits<uint64_t>::max(),
            m_ImageAvailableSemaphores[m_CurrentFrame],  // must be a not signaled semaphore
            VK_NULL_HANDLE,
            imageIndex);

        return result;
    }

    VkResult VESwapChain::SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex)
    {
        if (m_ImagesInFlight[*imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_Device.Device(), 1, &m_ImagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
        }

        m_ImagesInFlight[*imageIndex]                   = m_InFlightFences[m_CurrentFrame];

        VkSubmitInfo submitInfo = {};

        submitInfo.sType                                = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]                    = { m_ImageAvailableSemaphores[m_CurrentFrame] };
        VkPipelineStageFlags waitStages[]               = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount                   = 1;
        submitInfo.pWaitSemaphores                      = waitSemaphores;
        submitInfo.pWaitDstStageMask                    = waitStages;

        submitInfo.commandBufferCount                   = 1;
        submitInfo.pCommandBuffers                      = buffers;

        VkSemaphore signalSemaphores[]                  = { m_RenderFinishedSemaphores[m_CurrentFrame] };
        submitInfo.signalSemaphoreCount                 = 1;
        submitInfo.pSignalSemaphores                    = signalSemaphores;

        vkResetFences(m_Device.Device(), 1, &m_InFlightFences[m_CurrentFrame]);

        if (vkQueueSubmit(m_Device.GraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo = {};

        presentInfo.sType                               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount                  = 1;
        presentInfo.pWaitSemaphores                     = signalSemaphores;

        VkSwapchainKHR swapChains[]                     = { m_SwapChain };
        presentInfo.swapchainCount                      = 1;
        presentInfo.pSwapchains                         = swapChains;

        presentInfo.pImageIndices                       = imageIndex;

        auto result = vkQueuePresentKHR(m_Device.PresentQueue(), &presentInfo);

        m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return result;
    }

    void VESwapChain::CreateSwapChain()
    {
        SwapChainSupportDetails SwapChainSupport        = m_Device.GetSwapChainSupport();

        VkSurfaceFormatKHR surfaceFormat                = ChooseSwapSurfaceFormat(SwapChainSupport.Formats);
        VkPresentModeKHR presentMode                    = ChooseSwapPresentMode(SwapChainSupport.PresentModes);
        VkExtent2D extent                               = ChooseSwapExtent(SwapChainSupport.Capabilities);

        uint32_t imageCount                             = SwapChainSupport.Capabilities.minImageCount + 1;

        if (SwapChainSupport.Capabilities.maxImageCount > 0 && imageCount > SwapChainSupport.Capabilities.maxImageCount)
        {
            imageCount = SwapChainSupport.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};

        createInfo.sType                                = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface                              = m_Device.Surface();

        createInfo.minImageCount                        = imageCount;
        createInfo.imageFormat                          = surfaceFormat.format;
        createInfo.imageColorSpace                      = surfaceFormat.colorSpace;
        createInfo.imageExtent                          = extent;
        createInfo.imageArrayLayers                     = 1;
        createInfo.imageUsage                           = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices                      = m_Device.FindPhysicalQueueFamilies();
        uint32_t queueFamilyIndices[]                   = { indices.GraphicsFamily, indices.PresentFamily };

        if (indices.GraphicsFamily != indices.PresentFamily)
        {
            createInfo.imageSharingMode                 = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount            = 2;
            createInfo.pQueueFamilyIndices              = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode                 = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount            = 0;        // Optional
            createInfo.pQueueFamilyIndices              = nullptr;  // Optional
        }

        createInfo.preTransform                         = SwapChainSupport.Capabilities.currentTransform;
        createInfo.compositeAlpha                       = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode                          = presentMode;
        createInfo.clipped                              = VK_TRUE;

        createInfo.oldSwapchain                         = m_OldSwapChain == nullptr ? VK_NULL_HANDLE : m_OldSwapChain->m_SwapChain;

        if (vkCreateSwapchainKHR(m_Device.Device(), &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }

        // we only specified a minimum number of images in the swap chain, so the implementation is
        // allowed to create a swap chain with more. That's why we'll first query the final number of
        // images with vkGetm_SwapChainImagesKHR, then resize the container and finally call it again to
        // retrieve the handles.
        vkGetSwapchainImagesKHR(m_Device.Device(), m_SwapChain, &imageCount, nullptr);
        m_SwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_Device.Device(), m_SwapChain, &imageCount, m_SwapChainImages.data());

        m_SwapChainImageFormat                          = surfaceFormat.format;
        m_SwapChainExtent                               = extent;
    }

    void VESwapChain::CreateImageViews()
    {
        m_SwapChainImageViews.resize(m_SwapChainImages.size());

        for (size_t i = 0; i < m_SwapChainImages.size(); i++)
        {
            VkImageViewCreateInfo viewInfo = {};

            viewInfo.sType                              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image                              = m_SwapChainImages[i];
            viewInfo.viewType                           = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format                             = m_SwapChainImageFormat;
            viewInfo.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel      = 0;
            viewInfo.subresourceRange.levelCount        = 1;
            viewInfo.subresourceRange.baseArrayLayer    = 0;
            viewInfo.subresourceRange.layerCount        = 1;

            if (vkCreateImageView(m_Device.Device(), &viewInfo, nullptr, &m_SwapChainImageViews[i]) !=
                VK_SUCCESS)
            {
                throw std::runtime_error("failed to create texture image view!");
            }
        }
    }

    void VESwapChain::CreateRenderPass()
    {
        VkAttachmentDescription depthAttachment = {};

        depthAttachment.format                          = FindDepthFormat();
        depthAttachment.samples                         = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp                          = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp                         = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp                   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp                  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout                     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef = {};

        depthAttachmentRef.attachment                   = 1;
        depthAttachmentRef.layout                       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription colorAttachment = {};

        colorAttachment.format                          = GetSwapChainImageFormat();
        colorAttachment.samples                         = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp                          = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp                         = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilStoreOp                  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp                   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.initialLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout                     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};

        colorAttachmentRef.attachment                   = 0;
        colorAttachmentRef.layout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};

        subpass.pipelineBindPoint                       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount                    = 1;
        subpass.pColorAttachments                       = &colorAttachmentRef;
        subpass.pDepthStencilAttachment                 = &depthAttachmentRef;

        VkSubpassDependency dependency = {};

        dependency.dstSubpass                           = 0;
        dependency.dstAccessMask                        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                                          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask                         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcSubpass                           = VK_SUBPASS_EXTERNAL;
        dependency.srcAccessMask                        = 0;
        dependency.srcStageMask                         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                                                          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

        VkRenderPassCreateInfo renderPassInfo = {};

        renderPassInfo.sType                            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount                  = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments                     = attachments.data();
        renderPassInfo.subpassCount                     = 1;
        renderPassInfo.pSubpasses                       = &subpass;
        renderPassInfo.dependencyCount                  = 1;
        renderPassInfo.pDependencies                    = &dependency;

        if (vkCreateRenderPass(m_Device.Device(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void VESwapChain::CreateFramebuffers()
    {
        m_SwapChainFramebuffers.resize(ImageCount());

        for (size_t i = 0; i < ImageCount(); i++)
        {
            std::array<VkImageView, 2> attachments      = { m_SwapChainImageViews[i], m_DepthImageViews[i] };

            VkExtent2D SwapChainExtent                  = GetSwapChainExtent();
            VkFramebufferCreateInfo framebufferInfo = {};

            framebufferInfo.sType                       = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass                  = m_RenderPass;
            framebufferInfo.attachmentCount             = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments                = attachments.data();
            framebufferInfo.width                       = SwapChainExtent.width;
            framebufferInfo.height                      = SwapChainExtent.height;
            framebufferInfo.layers                      = 1;

            if (vkCreateFramebuffer(
                m_Device.Device(),
                &framebufferInfo,
                nullptr,
                &m_SwapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void VESwapChain::CreateDepthResources()
    {
        VkFormat depthFormat                            = FindDepthFormat();
        m_SwapChainDepthFormat                          = depthFormat;
        VkExtent2D SwapChainExtent                      = GetSwapChainExtent();

        m_DepthImages.resize(ImageCount());
        m_DepthImageMemorys.resize(ImageCount());
        m_DepthImageViews.resize(ImageCount());

        for (int i = 0; i < m_DepthImages.size(); i++)
        {
            VkImageCreateInfo imageInfo = {};

            imageInfo.sType                             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType                         = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width                      = SwapChainExtent.width;
            imageInfo.extent.height                     = SwapChainExtent.height;
            imageInfo.extent.depth                      = 1;
            imageInfo.mipLevels                         = 1;
            imageInfo.arrayLayers                       = 1;
            imageInfo.format                            = depthFormat;
            imageInfo.tiling                            = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout                     = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage                             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageInfo.samples                           = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode                       = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.flags = 0;

            m_Device.CreateImageWithInfo(
                imageInfo,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_DepthImages[i],
                m_DepthImageMemorys[i]);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType                              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image                              = m_DepthImages[i];
            viewInfo.viewType                           = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format                             = depthFormat;
            viewInfo.subresourceRange.aspectMask        = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel      = 0;
            viewInfo.subresourceRange.levelCount        = 1;
            viewInfo.subresourceRange.baseArrayLayer    = 0;
            viewInfo.subresourceRange.layerCount        = 1;

            if (vkCreateImageView(m_Device.Device(), &viewInfo, nullptr, &m_DepthImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create texture image view!");
            }
        }
    }

    void VESwapChain::CreateSyncObjects()
    {
        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_ImagesInFlight.resize(ImageCount(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo = {};

        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};

        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(m_Device.Device(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device.Device(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_Device.Device(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    VkSurfaceFormatKHR VESwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VESwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                std::cout << "Present mode: Mailbox" << std::endl;
                return availablePresentMode;
            }
        }

        // for (const auto &availablePresentMode : availablePresentModes) {
        //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        //     std::cout << "Present mode: Immediate" << std::endl;
        //     return availablePresentMode;
        //   }
        // }

        std::cout << "Present mode: V-Sync" << std::endl;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VESwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            VkExtent2D actualExtent = m_WindowExtent;

            actualExtent.width = std::max(capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width,  actualExtent.width));

            actualExtent.height = std::max(capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    VkFormat VESwapChain::FindDepthFormat()
    {
        return m_Device.FindSupportedFormat({ VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

}