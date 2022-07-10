#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>


namespace LearnVulkan
{
    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;
        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };
}  // namespace LearnVulkan
