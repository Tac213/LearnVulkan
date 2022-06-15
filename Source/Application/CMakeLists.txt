set(TARGET_NAME LearnVulkanApplication)

add_executable(${TARGET_NAME} LearnVulkan.cpp)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 20 OUTPUT_NAME "LearnVulkan")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_link_libraries(${TARGET_NAME} PUBLIC glm)
target_link_libraries(${TARGET_NAME} PUBLIC glfw)
target_link_libraries(${TARGET_NAME} PUBLIC ${Vulkan_LIBRARY})

target_include_directories(${TARGET_NAME} PUBLIC ${Vulkan_INCLUDE_DIR})
target_include_directories(${TARGET_NAME} PUBLIC ${THIRD_PARTY_DIR}/glm)
