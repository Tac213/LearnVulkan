set(TARGET_NAME LearnVulkanRuntime)

# Define include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Public)

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.hpp)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 20 OUTPUT_NAME "Runtime")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_compile_definitions(${TARGET_NAME} 
    PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

target_link_libraries(${TARGET_NAME} PUBLIC glm)
target_link_libraries(${TARGET_NAME} PUBLIC glfw)
target_link_libraries(${TARGET_NAME} PUBLIC ${Vulkan_LIBRARY})

target_include_directories(${TARGET_NAME} PUBLIC ${Vulkan_INCLUDE_DIR})
target_include_directories(${TARGET_NAME} PUBLIC ${THIRD_PARTY_DIR}/glm)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Public)
