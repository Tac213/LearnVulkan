#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

namespace LearnVulkan
{
    struct UniformBufferObject
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;
    };
}  // namespace LearnVulkan
