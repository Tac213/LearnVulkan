#include "Application/Application.hpp"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <vector>

using namespace LearnVulkan;

Application::Application(const ApplicationConfiguration& configuration)
    : mConfig(configuration)
{}

int Application::initialize()
{
    mbQuit = false;
    initWindow();
    if (!mWindow)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }
    initVulkan();
    return EXIT_SUCCESS;
}

void Application::finalize()
{
    vkDestroyInstance(mVulkanInstance, nullptr);
    if (mWindow)
    {
        glfwDestroyWindow(mWindow);
    }
    glfwTerminate();
}

void Application::tick()
{
    if (glfwWindowShouldClose(mWindow))
    {
        mbQuit = true;
        return;
    }
    glfwPollEvents();
}

bool Application::isQuit()
{
    return mbQuit;
}

void Application::initWindow()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    // Because GLFW was originally designed to create an OpenGL context
    // we need to tell it to not create an OpenGL context with a subsequent call
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // disable window resize
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // create window
    mWindow = glfwCreateWindow(mConfig.windowWidth, mConfig.windowHeight, mConfig.windowTitle, nullptr, nullptr);
}

void Application::initVulkan()
{
    if (!checkExtensionSupport())
    {
        mbQuit = true;
        return;
    }
    createVulkanInstance();
}

bool Application::checkExtensionSupport()
{
    uint32_t vulkanExtensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(vulkanExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionCount, extensions.data());
    uint32_t glfwRequiredExtensionCount = 0;
    const char** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
    bool bAllSupported = true;
    const char* unsupportedExtensionName = nullptr;

    for (int i = 0; i < glfwRequiredExtensionCount; i++)
    {
        bool bSupported = false;
        for (const VkExtensionProperties& extension : extensions)
        {
            if (strcmp(glfwRequiredExtensions[i], extension.extensionName) == 0)
            {
                bSupported = true;
                break;
            }
        }
        if (!bSupported)
        {
            bAllSupported = false;
            unsupportedExtensionName = glfwRequiredExtensions[i];
            break;
        }
    }
    if (!bAllSupported)
    {
        std::cerr << "Unsupported extension: " << unsupportedExtensionName << std::endl;
        return false;
    }
    return true;
}

void Application::createVulkanInstance()
{
    VkApplicationInfo appInfo {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = mConfig.windowTitle;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Tamashii";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    uint32_t glfwRequiredExtensionCount = 0;
    const char** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
    createInfo.enabledExtensionCount = glfwRequiredExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwRequiredExtensions;
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &mVulkanInstance) != VK_SUCCESS)
    {
        mbQuit = true;
        std::cerr << "Failed to create Vulkan instance!" << std::endl;
    }
}
