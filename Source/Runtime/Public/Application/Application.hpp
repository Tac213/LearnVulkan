#pragma once

#include "Configuration.hpp"
#include "Interface/IApplication.hpp"
#include "Interface/Interface.hpp"
#include "Vertex.hpp"
#include "VulkanUtility/QueueFamilyIndices.hpp"
#include "VulkanUtility/SwapchainSupportDetails.hpp"
#include "VulkanUtility/UniformBufferObject.hpp"
#include <string>
#include <unordered_map>
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
        VkPhysicalDeviceProperties mPhysicalDeviceProperties;
        VkDevice mLogicalDevice;
        VkQueue mGraphicsQueue;
        VkQueue mPresentQueue;
        VkSwapchainKHR mSwapchain;
        std::vector<VkImage> mSwapchainImages;
        VkFormat mSwapchainImageFormat;
        VkExtent2D mSwapchainExtent;
        std::vector<VkImageView> mSwapchainImageViews;
        VkRenderPass mRenderPass;
        VkDescriptorSetLayout mDescriptorSetLayout;
        VkPipelineLayout mPipelineLayout;
        VkPipeline mGraphicsPipeline;
        std::vector<VkFramebuffer> mSwapchainFramebuffers;
        VkCommandPool mCommandPool;
        VkImage mDepthImage;
        VkDeviceMemory mDepthImageMemory;
        VkImageView mDepthImageView;
        uint32_t mMipLevels;
        VkImage mTextureImage;
        VkDeviceMemory mTextureImageMemory;
        VkImageView mTextureImageView;
        VkSampler mTextureSampler;
        VkSampleCountFlagBits mMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
        VkImage mColorImage;
        VkDeviceMemory mColorImageMemory;
        VkImageView mColorImageView;
        VkBuffer mVertexBuffer;
        VkDeviceMemory mVertexBufferMemory;
        VkBuffer mIndexBuffer;
        VkDeviceMemory mIndexBufferMemory;
        std::vector<VkBuffer> mUniformBuffers;
        std::vector<VkDeviceMemory> mUniformBuffersMemory;
        VkDescriptorPool mDescriptorPool;
        std::vector<VkDescriptorSet> mDescriptorSets;
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
        const std::string modelPath = "Model/viking_room.obj";
        const std::string texturePath = "Texture/viking_room.png";
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
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

        void pickPhysicalDevice();
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
        void createDescriptorSetLayout();
        void createGraphicsPipeline();

        VkShaderModule createShaderModule(std::vector<char> shaderCode);

        static const int MAX_FRAMES_IN_FLIGHT;
        void createFramebuffers();
        void createCommandPool();
        void createColorResources();
        void createDepthResources();
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
        void createCommandBuffers();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void createSyncronizationObjects();

        uint32_t findPhysicalDeviceMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipLevels);

        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);

        void updateUniformBuffer(uint32_t currentImageIndex);

        void createTextureImage();
        void createTextureImageView();
        void createTextureSampler();
        VkSampleCountFlagBits getMaxUsableSampleCount() const;

        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat findDepthFormat();
        static bool hasStencilComponent(VkFormat format);

        void loadModel();
    };
}  // namespace LearnVulkan
