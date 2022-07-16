#include "Application/Application.hpp"
#include "FileSystem/FileReader.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

using namespace LearnVulkan;

#ifdef DEBUG
const std::vector<const char*> Application::VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
#endif

const std::vector<const char*> Application::PHYSICAL_DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef OS_MACOS
    "VK_KHR_portability_subset"
#endif
};

const int Application::MAX_FRAMES_IN_FLIGHT = 2;

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
    clearSwapchain();
    vkDestroyDescriptorSetLayout(mLogicalDevice, mDescriptorSetLayout, nullptr);
    vkDestroyBuffer(mLogicalDevice, mIndexBuffer, nullptr);
    vkFreeMemory(mLogicalDevice, mIndexBufferMemory, nullptr);
    vkDestroyBuffer(mLogicalDevice, mVertexBuffer, nullptr);
    vkFreeMemory(mLogicalDevice, mVertexBufferMemory, nullptr);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyFence(mLogicalDevice, mInFlightFences[i], nullptr);
        vkDestroySemaphore(mLogicalDevice, mRenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(mLogicalDevice, mImageAvailableSemaphores[i], nullptr);
    }
    vkDestroyCommandPool(mLogicalDevice, mCommandPool, nullptr);
    vkDestroyDevice(mLogicalDevice, nullptr);
#ifdef DEBUG
    destoryDebugUtilsMessengerEXT(mVulkanInstance, mDebugMessenger, nullptr);
#endif
    vkDestroySurfaceKHR(mVulkanInstance, mWindowSurface, nullptr);
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
        vkDeviceWaitIdle(mLogicalDevice);
        mbQuit = true;
        return;
    }
    glfwPollEvents();
    drawFrame();
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

    // create window
    mWindow = glfwCreateWindow(mConfig.windowWidth, mConfig.windowHeight, mConfig.windowTitle, nullptr, nullptr);
    glfwSetWindowUserPointer(mWindow, this);
    glfwSetFramebufferSizeCallback(mWindow, frameBufferResizeCallback);
}

void Application::initVulkan()
{
    if (!checkExtensionSupport())
    {
        mbQuit = true;
        return;
    }
    createVulkanInstance();
#ifdef DEBUG
    setupDebugMessenger();
#endif
    createWindowSurface();
    pickPysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncronizationObjects();
}

void Application::drawFrame()
{
    // Wait until the previous frame has finished
    vkWaitForFences(mLogicalDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

    // acquiring an image from the swap chain
    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(mLogicalDevice, mSwapchain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }
    else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        std::cerr << "Failed to acquired swapchain image!" << std::endl;
        return;
    }

    // reset the fence to the unsignaled state
    // only reset the fence if we are submitting work
    vkResetFences(mLogicalDevice, 1, &mInFlightFences[mCurrentFrame]);

    // record the command buffer
    vkResetCommandBuffer(mCommandBuffers[mCurrentFrame], 0);
    recordCommandBuffer(mCommandBuffers[mCurrentFrame], imageIndex);

    updateUniformBuffer(mCurrentFrame);

    // submit the command buffer
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {mImageAvailableSemaphores[mCurrentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffers[mCurrentFrame];
    VkSemaphore signalSemaphores[] = {mRenderFinishedSemaphores[mCurrentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // submit the command buffer to the graphics queue
    if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrame]) != VK_SUCCESS)
    {
        std::cerr << "Failed to submit draw command buffer!" << std::endl;
        mbQuit = true;
        return;
    }

    VkPresentInfoKHR presentInfo {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {mSwapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkResult presentResult = vkQueuePresentKHR(mPresentQueue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || mbFramebufferResized)
    {
        mbFramebufferResized = false;
        recreateSwapchain();
    }
    else if (presentResult != VK_SUCCESS)
    {
        std::cerr << "Failed to present swapchain images!" << std::endl;
        mbQuit = true;
        return;
    }

    mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::frameBufferResizeCallback(GLFWwindow* window, int width, int height)
{
    Application* application = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    application->mbFramebufferResized = true;
}

bool Application::checkExtensionSupport()
{
    uint32_t vulkanExtensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(vulkanExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionCount, extensions.data());

    std::vector<const char*> requiredExtensions = getRequiredExtensions();
    bool bAllSupported = true;
    const char* unsupportedExtensionName = nullptr;

    for (const char* requiredExtension : requiredExtensions)
    {
        bool bSupported = false;
        for (const VkExtensionProperties& extension : extensions)
        {
            if (strcmp(requiredExtension, extension.extensionName) == 0)
            {
                bSupported = true;
                break;
            }
        }
        if (!bSupported)
        {
            bAllSupported = false;
            unsupportedExtensionName = requiredExtension;
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
#ifdef DEBUG
    if (!checkValidationLayerSupport())
    {
        mbQuit = true;
        std::cerr << "Validation layers requested, but not available!" << std::endl;
        return;
    }
#endif
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
    std::vector<const char*> extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
#ifdef DEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = &debugCreateInfo;
#else
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
#endif

#ifdef OS_MACOS
    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    if (vkCreateInstance(&createInfo, nullptr, &mVulkanInstance) != VK_SUCCESS)
    {
        mbQuit = true;
        std::cerr << "Failed to create Vulkan instance!" << std::endl;
    }
}

std::vector<const char*> Application::getRequiredExtensions()
{
    uint32_t glfwRequiredExtensionCount = 0;
    const char** glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
    std::vector<const char*> extensions(glfwRequiredExtensions, glfwRequiredExtensions + glfwRequiredExtensionCount);
#ifdef DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
#ifdef OS_MACOS
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
    return extensions;
}

#ifdef DEBUG
bool Application::checkValidationLayerSupport()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : VALIDATION_LAYERS)
    {
        bool bLayerFound = false;
        for (const VkLayerProperties& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                bLayerFound = true;
                break;
            }
        }
        if (!bLayerFound)
        {
            return false;
        }
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                          VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                          const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                          void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void Application::setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo {};
    populateDebugMessengerCreateInfo(createInfo);

    if (createDebugUtilsMessengerEXT(mVulkanInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS)
    {
        std::cerr << "Failed to set up debug messenger!" << std::endl;
        mbQuit = true;
    }
}

void Application::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    // createInfo.pUserData = nullptr;
}

VkResult Application::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func == nullptr)
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

void Application::destoryDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func == nullptr)
    {
        return;
    }
    func(instance, debugMessenger, pAllocator);
}
#endif  // ifdef DEBUG

void Application::pickPysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mVulkanInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        std::cerr << "Failed to find GPUs with Vulkan support!" << std::endl;
        return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mVulkanInstance, &deviceCount, devices.data());

    int highestScore = 0;
    for (const VkPhysicalDevice& device : devices)
    {
        int deviceScore = rateDeviceSuitability(device);
        if (deviceScore > highestScore)
        {
            mPhysicalDevice = device;
            highestScore = deviceScore;
        }
    }

    if (mPhysicalDevice == VK_NULL_HANDLE)
    {
        std::cerr << "Failed to find a suitable GPU!" << std::endl;
        mbQuit = true;
    }
}

int Application::rateDeviceSuitability(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilyIndices(device);
    bool bExtensionsSupported = checkPhysicalDeviceSupport(device);
    bool bSwapchainAdequate = false;
    if (bExtensionsSupported)
    {
        SwapchainSupportDetails swapchainSupportDetails = querySwapchainSupport(device);
        bSwapchainAdequate = !swapchainSupportDetails.formats.empty() && !swapchainSupportDetails.presentModes.empty();
    }
    if (!indices.isComplete() || !bExtensionsSupported || !bSwapchainAdequate)
    {
        return 0;
    }

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

#ifndef OS_MACOS
    // Application cannot function without geometry shaders
    if (!deviceFeatures.geometryShader)
    {
        return 0;
    }
#endif

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }
    else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 100;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

QueueFamilyIndices Application::findQueueFamilyIndices(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 bPresentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mWindowSurface, &bPresentSupport);
        if (bPresentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

bool Application::checkPhysicalDeviceSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(PHYSICAL_DEVICE_EXTENSIONS.begin(), PHYSICAL_DEVICE_EXTENSIONS.end());
    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void Application::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilyIndices(mPhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    const float QUEUE_PRIORITY = 1.0f;
    for (uint32_t queueFamilyIndex : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &QUEUE_PRIORITY;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures {};

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
#ifdef DEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    createInfo.enabledExtensionCount = static_cast<uint32_t>(PHYSICAL_DEVICE_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = PHYSICAL_DEVICE_EXTENSIONS.data();

    if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mLogicalDevice) != VK_SUCCESS)
    {
        std::cerr << "Failed to create logical device!" << std::endl;
        mbQuit = true;
        return;
    }
    vkGetDeviceQueue(mLogicalDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue);
    vkGetDeviceQueue(mLogicalDevice, indices.presentFamily.value(), 0, &mPresentQueue);
}

void Application::createWindowSurface()
{
    if (glfwCreateWindowSurface(mVulkanInstance, mWindow, nullptr, &mWindowSurface) != VK_SUCCESS)
    {
        std::cerr << "Failed to create window surface!" << std::endl;
        mbQuit = true;
    }
}

SwapchainSupportDetails Application::querySwapchainSupport(VkPhysicalDevice device)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mWindowSurface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, mWindowSurface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, mWindowSurface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, mWindowSurface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, mWindowSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR Application::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(mWindow, &width, &height);
    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return actualExtent;
}

void Application::createSwapchain()
{
    SwapchainSupportDetails swapChainSupportDetails = querySwapchainSupport(mPhysicalDevice);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupportDetails.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupportDetails.capabilities);

    uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
    if (swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilities.maxImageCount)
    {
        imageCount = swapChainSupportDetails.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = mWindowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilyIndices(mPhysicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
    }

    createInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(mLogicalDevice, &createInfo, nullptr, &mSwapchain) != VK_SUCCESS)
    {
        std::cerr << "Failed to create swap chain" << std::endl;
        mbQuit = true;
        return;
    }

    vkGetSwapchainImagesKHR(mLogicalDevice, mSwapchain, &imageCount, nullptr);
    mSwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(mLogicalDevice, mSwapchain, &imageCount, mSwapchainImages.data());

    mSwapchainImageFormat = surfaceFormat.format;
    mSwapchainExtent = extent;
}

void Application::recreateSwapchain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(mWindow, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(mWindow, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(mLogicalDevice);

    clearSwapchain();

    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
}

void Application::clearSwapchain()
{
    for (VkFramebuffer framebuffer : mSwapchainFramebuffers)
    {
        vkDestroyFramebuffer(mLogicalDevice, framebuffer, nullptr);
    }
    vkDestroyPipeline(mLogicalDevice, mGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(mLogicalDevice, mPipelineLayout, nullptr);
    vkDestroyRenderPass(mLogicalDevice, mRenderPass, nullptr);
    for (auto& imageView : mSwapchainImageViews)
    {
        vkDestroyImageView(mLogicalDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(mLogicalDevice, mSwapchain, nullptr);

    vkDestroyDescriptorPool(mLogicalDevice, mDescriptorPool, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(mLogicalDevice, mUniformBuffers[i], nullptr);
        vkFreeMemory(mLogicalDevice, mUniformBuffersMemory[i], nullptr);
    }
}

void Application::createImageViews()
{
    mSwapchainImageViews.resize(mSwapchainImages.size());
    for (size_t i = 0; i < mSwapchainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = mSwapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = mSwapchainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(mLogicalDevice, &createInfo, nullptr, &mSwapchainImageViews[i]) != VK_SUCCESS)
        {
            std::cerr << "Failed to create image views!" << std::endl;
            mbQuit = true;
            return;
        }
    }
}

void Application::createRenderPass()
{
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = mSwapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(mLogicalDevice, &createInfo, nullptr, &mRenderPass) != VK_SUCCESS)
    {
        std::cerr << "Failed to create render pass" << std::endl;
        mbQuit = true;
    }
}

void Application::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("Shader/Vert.spv");
    auto fragShaderCode = readFile("Shader/Frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStageInfos[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(mSwapchainExtent.width);
    viewport.height = static_cast<float>(mSwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor {};
    scissor.offset = {0, 0};
    scissor.extent = mSwapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationState {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.lineWidth = 1.0f;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState {};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.minSampleShading = 1.0f;
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {};
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendState {};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachmentState;
    colorBlendState.blendConstants[0] = 0.0f;
    colorBlendState.blendConstants[1] = 0.0f;
    colorBlendState.blendConstants[2] = 0.0f;
    colorBlendState.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(mLogicalDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
    {
        std::cerr << "Failed to create pipeline layout" << std::endl;
        mbQuit = true;
        return;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStageInfos;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationState;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = nullptr;

    pipelineInfo.layout = mPipelineLayout;

    pipelineInfo.renderPass = mRenderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(mLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS)
    {
        std::cerr << "Failed to create graphics pipeline" << std::endl;
        mbQuit = true;
        return;
    }

    vkDestroyShaderModule(mLogicalDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(mLogicalDevice, fragShaderModule, nullptr);
}

void Application::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo uboLayoutInfo {};
    uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uboLayoutInfo.bindingCount = 1;
    uboLayoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(mLogicalDevice, &uboLayoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS)
    {
        std::cerr << "Failed to create descriptor set layout!" << std::endl;
        mbQuit = true;
    }
}

VkShaderModule Application::createShaderModule(std::vector<char> shaderCode)
{
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule shaderModule;

    if (vkCreateShaderModule(mLogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

void Application::createFramebuffers()
{
    mSwapchainFramebuffers.resize(mSwapchainImageViews.size());

    for (size_t i = 0; i < mSwapchainImageViews.size(); i++)
    {
        VkImageView attachments[] = {mSwapchainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = mRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = mSwapchainExtent.width;
        framebufferInfo.height = mSwapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(mLogicalDevice, &framebufferInfo, nullptr, &mSwapchainFramebuffers[i]) != VK_SUCCESS)
        {
            std::cerr << "Failed to create framebuffer!" << std::endl;
            mbQuit = true;
            return;
        }
    }
}

void Application::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilyIndices(mPhysicalDevice);
    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    if (vkCreateCommandPool(mLogicalDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
    {
        std::cerr << "Failed to create command pool!" << std::endl;
        mbQuit = true;
    }
}

void Application::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(mLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(mLogicalDevice, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mVertexBuffer, mVertexBufferMemory);
    copyBuffer(stagingBuffer, mVertexBuffer, bufferSize);

    vkDestroyBuffer(mLogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(mLogicalDevice, stagingBufferMemory, nullptr);
}

void Application::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(mLogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(mLogicalDevice, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mIndexBuffer, mIndexBufferMemory);
    copyBuffer(stagingBuffer, mIndexBuffer, bufferSize);

    vkDestroyBuffer(mLogicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(mLogicalDevice, stagingBufferMemory, nullptr);
}

void Application::createUniformBuffers()
{
    mUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    mUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mUniformBuffers[i], mUniformBuffersMemory[i]);
    }
}

void Application::createDescriptorPool()
{
    VkDescriptorPoolSize poolSize {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(mLogicalDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
    {
        std::cerr << "Failed to create Descriptor Pool!" << std::endl;
        mbQuit = true;
    }
}

void Application::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = mDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    mDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(mLogicalDevice, &allocInfo, mDescriptorSets.data()) != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate Descriptor Sets!" << std::endl;
        mbQuit = true;
        return;
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = mUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet writeDescriptorSet {};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = mDescriptorSets[i];
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.pBufferInfo = &bufferInfo;
        writeDescriptorSet.pImageInfo = nullptr;
        writeDescriptorSet.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(mLogicalDevice, 1, &writeDescriptorSet, 0, nullptr);
    }
}

void Application::createCommandBuffers()
{
    mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());

    if (vkAllocateCommandBuffers(mLogicalDevice, &allocInfo, mCommandBuffers.data()) != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate command buffers!" << std::endl;
        mbQuit = true;
    }
}

void Application::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = mRenderPass;
    renderPassInfo.framebuffer = mSwapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = mSwapchainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

    VkBuffer vertexBuffers[] = {mVertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSets[mCurrentFrame], 0, nullptr);
    // vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void Application::createSyncronizationObjects()
{
    mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(mLogicalDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS)
        {
            std::cerr << "Failed to create Image Available Semaphore!" << std::endl;
            mbQuit = true;
            return;
        }

        if (vkCreateSemaphore(mLogicalDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            std::cerr << "Failed to create Render Finished Semaphore!" << std::endl;
            mbQuit = true;
            return;
        }

        if (vkCreateFence(mLogicalDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS)
        {
            std::cerr << "Failed to create In Flight Fence!" << std::endl;
            mbQuit = true;
            return;
        }
    }
}

uint32_t Application::findPhysicalDeviceMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable physical device memory type!");
}

void Application::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(mLogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(mLogicalDevice, buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findPhysicalDeviceMemoryType(memoryRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(mLogicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(mLogicalDevice, buffer, bufferMemory, 0);
}

void Application::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = mCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(mLogicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mGraphicsQueue);

    vkFreeCommandBuffers(mLogicalDevice, mCommandPool, 1, &commandBuffer);
}

void Application::updateUniformBuffer(uint32_t currentImageIndex)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.projection = glm::perspective(glm::radians(45.0f), mSwapchainExtent.width / static_cast<float>(mSwapchainExtent.height), 0.1f, 10.0f);
    ubo.projection[1][1] = -1;

    void* data;
    vkMapMemory(mLogicalDevice, mUniformBuffersMemory[currentImageIndex], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(mLogicalDevice, mUniformBuffersMemory[currentImageIndex]);
}
