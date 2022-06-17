#pragma once

#include "Interface/IModule.hpp"
#include "Interface/Interface.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace LearnVulkan
{
    _Interface_ IApplication : _inherits_ IModule
    {
    public:
        virtual bool isQuit() = 0;

    protected:
        GLFWwindow* mWindow = nullptr;

        virtual void initWindow() = 0;
        virtual void initVulkan() = 0;
    };
}  // namespace LearnVulkan
