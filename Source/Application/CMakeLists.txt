set(TARGET_NAME LearnVulkanApplication)

add_executable(${TARGET_NAME} LearnVulkan.cpp)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 20 OUTPUT_NAME "LearnVulkan")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_link_libraries(${TARGET_NAME} PUBLIC LearnVulkanRuntime)
