#pragma once

#include "vulkan/vulkan.h"
#include <vector>

namespace LearnVulkan
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
}  // namespace LearnVulkan
