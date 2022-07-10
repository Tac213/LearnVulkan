#pragma once

#include "vulkan/vulkan.h"
#include <vector>

namespace LearnVulkan
{
    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
}  // namespace LearnVulkan
