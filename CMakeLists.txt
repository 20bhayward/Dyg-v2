cmake_minimum_required(VERSION 3.12)
project(DygEndless VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(USE_VULKAN "Use Vulkan rendering backend" ON)

# Find packages
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)


# Find SDL2
find_package(SDL2 REQUIRED)

# Find Vulkan if enabled
if(USE_VULKAN)
    find_package(Vulkan REQUIRED)
    add_definitions(-DUSE_VULKAN)
endif()

# Source files
file(GLOB_RECURSE SOURCES 
    "Engine/Core/*.cpp"
    "Engine/Simulation/*.cpp"
    "Engine/Procedural/*.cpp"
    "Engine/Rendering/*.cpp"
    "Engine/Assets/*.cpp"
    "main.cpp"
)

# Header files
file(GLOB_RECURSE HEADERS 
    "Engine/Core/*.h"
    "Engine/Simulation/*.h"
    "Engine/Procedural/*.h"
    "Engine/Rendering/*.h"
    "Engine/Assets/*.h"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_ENABLE_EXPERIMENTAL)
# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

# Dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
    glm::glm
    nlohmann_json::nlohmann_json
    ${SDL2_LIBRARIES}
)

# Add Vulkan if enabled
if(USE_VULKAN)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})
endif()

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Engine/Assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/Engine/Assets
)

# Copy SDL2 DLLs on Windows platforms
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL2::SDL2>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# If shader directory exists, copy shaders
if(EXISTS "${CMAKE_SOURCE_DIR}/Engine/Assets/Shaders/spirv")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Engine/Assets/Shaders/spirv
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/Engine/Assets/Shaders/spirv
    )
endif()

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Additional compilation flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()