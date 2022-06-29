#pragma once

#include "Configuration.hpp"
#include "Interface/IApplication.hpp"
#include "Interface/Interface.hpp"
#include <vector>

namespace LearnVulkan
{
    class Application : _implements_ IApplication
    {
    public:
        explicit Application(const ApplicationConfiguration&);
        Application() = delete;
        virtual int initialize() override;
        virtual void finalize() override;
        virtual void tick() override;

        bool isQuit() override;

    protected:
        bool mbQuit;
        const ApplicationConfiguration& mConfig;
        VkInstance mVulkanInstance;

        virtual void initWindow() override;
        virtual void initVulkan() override;

    private:
        void createVulkanInstance();
        static bool checkExtensionSupport();
        static std::vector<const char*> getRequiredExtensions();
#ifdef DEBUG
        static bool checkValidationLayerSupport();
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                            void* pUserData);
        void setupDebugMessenger();
        static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
        static void destoryDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
        VkDebugUtilsMessengerEXT mDebugMessenger;
        static const std::vector<const char*> VALIDATION_LAYERS;
#endif
    };
}  // namespace LearnVulkan
