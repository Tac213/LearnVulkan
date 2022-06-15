cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(LearnVulkan VERSION 0.1.0)

IF(${UNIX})
    IF(${APPLE})
        set(LEARN_VULKAN_TARGET_PLATFORM "MacOS")
        set(OS_MACOS 1)
    ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES Android)
        set(LEARN_VULKAN_TARGET_PLATFORM "Android")
        set(OS_ANDROID 1)
    ELSE(${APPLE})
        set(LEARN_VULKAN_TARGET_PLATFORM "Linux")
        set(OS_LINUX 1)
    ENDIF(${APPLE})
ELSEIF(${WIN32})
    set(LEARN_VULKAN_TARGET_PLATFORM "Windows")
    set(OS_WINDOWS 1)
ENDIF(${UNIX})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# set(CMAKE_C_COMPILER "clang")
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS "-Wall -std=gnu++20")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

MESSAGE( STATUS "LEARN_VULKAN_TARGET_PLATFORM: " ${LEARN_VULKAN_TARGET_PLATFORM} )

MESSAGE(STATUS "Attempting auto locate Vulkan using CMake......")

# Find Vulkan Path using CMake's Vulkan Module
# This will return Boolean 'Vulkan_FOUND' indicating the status of find as success(ON) or fail(OFF).
# Include directory path - 'Vulkan_INCLUDE_DIRS' and 'Vulkan_LIBRARY' with required libraries.
find_package(Vulkan)

# Try extracting VulkanSDK path from ${Vulkan_INCLUDE_DIRS}
if (NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "")
    set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
    STRING(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
endif()

if(NOT Vulkan_FOUND)
    # CMake may fail to locate the libraries but could be able to 
    # provide some path in Vulkan SDK include directory variable
    # 'Vulkan_INCLUDE_DIRS', try to extract path from this.
    MESSAGE(STATUS "Failed to locate Vulkan SDK, retrying again...")
    if(EXISTS "${VULKAN_PATH}")
        MESSAGE(STATUS "Successfully located the Vulkan SDK: ${VULKAN_PATH}")
    else()
        MESSAGE("Error: Unable to locate Vulkan SDK. Please turn ensure 'VULKAN_SDK' has been set as Environment Variable.")
        return()
    endif()
endif()

set(LEARN_VULKAN_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(LEARN_VULKAN_BINARY_ROOT_DIR "${CMAKE_BINARY_DIR}")
set(THIRD_PARTY_DIR "${LEARN_VULKAN_ROOT_DIR}/ThirdParty")

if(${OS_WINDOWS})
    set(glslangValidator_executable ${VULKAN_PATH}/Bin/glslangValidator.exe)
    add_compile_definitions("LEARN_VULKAN_VK_LAYER_PATH=${VULKAN_PATH}/Bin")
endif()

if(${OS_MACOS})
    set(glslangValidator_executable ${VULKAN_PATH}/Bin/glslangValidator)
    add_compile_definitions("LEARN_VULKAN_VK_LAYER_PATH=${VULKAN_PATH}/Bin")
    add_compile_definitions("LEARN_VULKAN_VK_ICD_FILENAMES=${VULKAN_PATH}/Bin/MoltenKV_icd.json")
endif()

if(${OS_LINUX})
    set(glslangValidator_executable ${VULKAN_PATH}/Bin/glslangValidator)
    add_compile_definitions("LEARN_VULKAN_VK_LAYER_PATH=${VULKAN_PATH}/Bin")
endif()

add_subdirectory(ThirdParty)

add_subdirectory(Source/Runtime)
add_subdirectory(Source/Application)
