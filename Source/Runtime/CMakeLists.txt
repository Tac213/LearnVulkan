set(TARGET_NAME LearnVulkanRuntime)

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE HEADER_FILES "*.hpp")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

# add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

# set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 20)
# set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")
