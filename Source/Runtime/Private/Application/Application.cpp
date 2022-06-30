#include "Application/Application.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

using namespace LearnVulkan;

#ifdef DEBUG
const std::vector<const char*> Application::VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
#endif

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
    vkDestroyDevice(mLogicalDevice, nullptr);
#ifdef DEBUG
    destoryDebugUtilsMessengerEXT(mVulkanInstance, mDebugMessenger, nullptr);
#endif
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
#ifdef DEBUG
    setupDebugMessenger();
#endif
    pickPysicalDevice();
    createLogicalDevice();
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
    if (!indices.isComplete())
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
        i++;
    }

    return indices;
}

void Application::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilyIndices(mPhysicalDevice);

    VkDeviceQueueCreateInfo queueCreateInfo {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;
    const float QUEUE_PRIORITY = 1.0f;
    queueCreateInfo.pQueuePriorities = &QUEUE_PRIORITY;

    VkPhysicalDeviceFeatures deviceFeatures {};

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
#ifdef DEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

#ifdef OS_MACOS
    const char* deviceExtension = "VK_KHR_portability_subset";
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = &deviceExtension;
#endif

    if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mLogicalDevice) != VK_SUCCESS)
    {
        std::cerr << "Failed to create logical device!" << std::endl;
        mbQuit = true;
        return;
    }
    vkGetDeviceQueue(mLogicalDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue);
}
