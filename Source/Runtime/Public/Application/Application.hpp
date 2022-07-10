#pragma once

#include "Configuration.hpp"
#include "Interface/IApplication.hpp"
#include "Interface/Interface.hpp"
#include "Vertex.hpp"
#include "VulkanUtility/QueueFamilyIndices.hpp"
#include "VulkanUtility/SwapchainSupportDetails.hpp"
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
        VkSurfaceKHR mWindowSurface;
        VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
        VkDevice mLogicalDevice;
        VkQueue mGraphicsQueue;
        VkQueue mPresentQueue;
        VkSwapchainKHR mSwapchain;
        std::vector<VkImage> mSwapchainImages;
        VkFormat mSwapchainImageFormat;
        VkExtent2D mSwapchainExtent;
        std::vector<VkImageView> mSwapchainImageViews;
        VkRenderPass mRenderPass;
        VkPipelineLayout mPipelineLayout;
        VkPipeline mGraphicsPipeline;
        std::vector<VkFramebuffer> mSwapchainFramebuffers;
        VkCommandPool mCommandPool;
        VkBuffer mVertexBuffer;
        VkDeviceMemory mVertexBufferMemory;
        std::vector<VkCommandBuffer> mCommandBuffers;
        std::vector<VkSemaphore> mImageAvailableSemaphores;
        std::vector<VkSemaphore> mRenderFinishedSemaphores;
        std::vector<VkFence> mInFlightFences;
        bool mbFramebufferResized = false;
        uint32_t mCurrentFrame = 0;

        virtual void initWindow() override;
        virtual void initVulkan() override;
        void drawFrame();

    private:
        const std::vector<Vertex> vertices = {
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
        static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
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

        void pickPysicalDevice();
        int rateDeviceSuitability(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device);
        static bool checkPhysicalDeviceSupport(VkPhysicalDevice device);
        static const std::vector<const char*> PHYSICAL_DEVICE_EXTENSIONS;

        void createLogicalDevice();
        void createWindowSurface();

        SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
        static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        void createSwapchain();
        void recreateSwapchain();
        void clearSwapchain();
        void createImageViews();
        void createRenderPass();
        void createGraphicsPipeline();

        VkShaderModule createShaderModule(std::vector<char> shaderCode);

        static const int MAX_FRAMES_IN_FLIGHT;
        void createFramebuffers();
        void createCommandPool();
        void createVertexBuffer();
        void createCommandBuffers();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void createSyncronizationObjects();

        uint32_t findPhysicalDeviceMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    };
}  // namespace LearnVulkan
