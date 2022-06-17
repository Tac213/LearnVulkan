#pragma once

#include <cstdint>

namespace LearnVulkan
{
    struct ApplicationConfiguration
    {
        ApplicationConfiguration(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle)
            : windowWidth(windowWidth)
            , windowHeight(windowHeight)
            , windowTitle(windowTitle) {}

        uint32_t    windowWidth;
        uint32_t    windowHeight;
        const char* windowTitle;
    };
}  // namespace LearnVulkan
