#pragma once

#include <cstdint>
#include <optional>

namespace LearnVulkan
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };
}  // namespace LearnVulkan
