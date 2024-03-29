#include <array>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>

namespace LearnVulkan
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        bool operator==(const Vertex& another) const;
        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
    };
}  // namespace LearnVulkan

namespace std
{
    template<>
    struct hash<LearnVulkan::Vertex>
    {
        size_t operator()(LearnVulkan::Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}  // namespace std
