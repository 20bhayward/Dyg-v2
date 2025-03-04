#include "VulkanRenderer.h"
#include "../Procedural/World.h"
#include "../Simulation/Material.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <set>
#include <chrono>

namespace Engine {

// Helper function to check Vulkan availability
bool VulkanRenderer::isVulkanAvailable() {
    SDL_Window* testWindow = SDL_CreateWindow(
        "Vulkan Test", 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED,
        1, 1, 
        SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN
    );
    
    if (!testWindow) {
        return false;
    }
    
    // Check if Vulkan is available (via SDL_Vulkan_LoadLibrary)
    unsigned int extensionCount = 0;
    bool vulkanSupported = SDL_Vulkan_GetInstanceExtensions(testWindow, &extensionCount, nullptr);
    
    SDL_DestroyWindow(testWindow);
    return vulkanSupported;
}

// Constructor
VulkanRenderer::VulkanRenderer(int screenWidth, int screenHeight, const std::string& appName)
    : m_screenWidth(screenWidth)
    , m_screenHeight(screenHeight)
    , m_appName(appName)
    , m_clearColor{0.1f, 0.2f, 0.4f, 1.0f} // Purple background for visibility
    , m_currentFrame(0)
    , m_currentImageIndex(0)
    , m_framebufferResized(false) {
    
    // Initialize Vulkan members
    m_instance = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    m_surface = VK_NULL_HANDLE;
    m_swapchain = VK_NULL_HANDLE;
    m_renderPass = VK_NULL_HANDLE;
    m_pipelineLayout = VK_NULL_HANDLE;
    m_graphicsPipeline = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_debugMessenger = VK_NULL_HANDLE;
    
    // Initialize viewport and scissor
    m_viewport = {
        0.0f, 0.0f,
        static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight),
        0.0f, 1.0f
    };
    
    m_scissor = {
        {0, 0},
        {static_cast<uint32_t>(m_screenWidth), static_cast<uint32_t>(m_screenHeight)}
    };
}

// Destructor
VulkanRenderer::~VulkanRenderer() {
    cleanup();
}

bool VulkanRenderer::initialize(SDL_Window* window) {
    std::cout << "Initializing Vulkan renderer..." << std::endl;
    
    // Initialize Vulkan instance
    if (!createInstance()) {
        std::cerr << "Failed to create Vulkan instance" << std::endl;
        return false;
    }
    
    // Setup debug messenger if validation layers are enabled
    if (!setupDebugMessenger()) {
        std::cerr << "Failed to setup debug messenger" << std::endl;
        return false;
    }
    
    // Create window surface
    if (!createSurface(window)) {
        std::cerr << "Failed to create window surface" << std::endl;
        return false;
    }
    
    // Pick physical device (GPU)
    if (!pickPhysicalDevice()) {
        std::cerr << "Failed to find a suitable GPU" << std::endl;
        return false;
    }
    
    // Create logical device
    if (!createLogicalDevice()) {
        std::cerr << "Failed to create logical device" << std::endl;
        return false;
    }
    
    // Create swap chain
    if (!createSwapChain()) {
        std::cerr << "Failed to create swap chain" << std::endl;
        return false;
    }
    
    // Create image views
    if (!createImageViews()) {
        std::cerr << "Failed to create image views" << std::endl;
        return false;
    }
    
    // Create render pass
    if (!createRenderPass()) {
        std::cerr << "Failed to create render pass" << std::endl;
        return false;
    }
    
    // Create descriptor set layout
    if (!createDescriptorSetLayout()) {
        std::cerr << "Failed to create descriptor set layout" << std::endl;
        return false;
    }
    
    // Create graphics pipeline
    if (!createGraphicsPipeline()) {
        std::cerr << "Failed to create graphics pipeline" << std::endl;
        return false;
    }
    
    // Create framebuffers
    if (!createFramebuffers()) {
        std::cerr << "Failed to create framebuffers" << std::endl;
        return false;
    }
    
    // Create command pool
    if (!createCommandPool()) {
        std::cerr << "Failed to create command pool" << std::endl;
        return false;
    }
    
    // Create world texture
    if (!createWorldTexture(m_screenWidth, m_screenHeight)) {
        std::cerr << "Failed to create world texture" << std::endl;
        return false;
    }
    
    // Create vertex buffer
    if (!createVertexBuffer()) {
        std::cerr << "Failed to create vertex buffer" << std::endl;
        return false;
    }
    
    // Create index buffer
    if (!createIndexBuffer()) {
        std::cerr << "Failed to create index buffer" << std::endl;
        return false;
    }
    
    // Create uniform buffer
    if (!createUniformBuffer()) {
        std::cerr << "Failed to create uniform buffer" << std::endl;
        return false;
    }
    
    // Create descriptor pool
    if (!createDescriptorPool()) {
        std::cerr << "Failed to create descriptor pool" << std::endl;
        return false;
    }
    
    // Create descriptor sets
    if (!createDescriptorSets()) {
        std::cerr << "Failed to create descriptor sets" << std::endl;
        return false;
    }
    
    // Create command buffers
    if (!createCommandBuffers()) {
        std::cerr << "Failed to create command buffers" << std::endl;
        return false;
    }
    
    // Create synchronization objects
    if (!createSyncObjects()) {
        std::cerr << "Failed to create synchronization objects" << std::endl;
        return false;
    }
    
    std::cout << "Vulkan renderer initialized successfully" << std::endl;
    return true;
}

void VulkanRenderer::cleanup() {
    std::cout << "Starting VulkanRenderer cleanup..." << std::endl;
    
    // Wait for device to finish operations
    if (m_device != VK_NULL_HANDLE) {
        try {
            std::cout << "Waiting for device idle..." << std::endl;
            vkDeviceWaitIdle(m_device);
        } catch (const std::exception& e) {
            std::cerr << "Error waiting for device: " << e.what() << std::endl;
        }
    }
    
    // Clean up sync objects
    std::cout << "Cleaning up sync objects..." << std::endl;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (m_device != VK_NULL_HANDLE) {
            if (i < m_renderFinishedSemaphores.size() && m_renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
                m_renderFinishedSemaphores[i] = VK_NULL_HANDLE;
            }
            if (i < m_imageAvailableSemaphores.size() && m_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
                m_imageAvailableSemaphores[i] = VK_NULL_HANDLE;
            }
            if (i < m_inFlightFences.size() && m_inFlightFences[i] != VK_NULL_HANDLE) {
                vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
                m_inFlightFences[i] = VK_NULL_HANDLE;
            }
        }
    }
    
    cleanupSwapChain();
    
    // Clean up uniform buffer
    if (m_device != VK_NULL_HANDLE) {
        if (m_uniformBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_uniformBuffer, nullptr);
        }
        if (m_uniformBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_uniformBufferMemory, nullptr);
        }
    }
    
    // Clean up descriptor pool
    if (m_device != VK_NULL_HANDLE && m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    }
    
    // Clean up descriptor set layout
    if (m_device != VK_NULL_HANDLE && m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
    }
    
    // Clean up vertex buffer
    if (m_device != VK_NULL_HANDLE) {
        if (m_vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        }
        if (m_vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
        }
    }
    
    // Clean up index buffer
    if (m_device != VK_NULL_HANDLE) {
        if (m_indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        }
        if (m_indexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
        }
    }
    
    // Clean up world texture
    if (m_device != VK_NULL_HANDLE) {
        if (m_worldTexture.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_device, m_worldTexture.sampler, nullptr);
        }
        if (m_worldTexture.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, m_worldTexture.imageView, nullptr);
        }
        if (m_worldTexture.image != VK_NULL_HANDLE) {
            vkDestroyImage(m_device, m_worldTexture.image, nullptr);
        }
        if (m_worldTexture.memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_worldTexture.memory, nullptr);
        }
    }
    
    // Clean up command pool
    if (m_device != VK_NULL_HANDLE && m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    }
    
    // Clean up device
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
    }
    
    // Clean up debug messenger
    if (m_enableValidationLayers && m_instance != VK_NULL_HANDLE && m_debugMessenger != VK_NULL_HANDLE) {
        destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    }
    
    // Clean up surface
    if (m_instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }
    
    // Clean up instance
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
    }
}

void VulkanRenderer::beginFrame() {
    // Wait for previous frame to complete
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    
    // Acquire the next image from the swap chain
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, 
                                          m_imageAvailableSemaphores[m_currentFrame], 
                                          VK_NULL_HANDLE, &m_currentImageIndex);
    
    // If swapchain is out of date, recreate it
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }
    
    // Reset fence for current frame
    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
    
    // Reset command buffer
    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
    
    // Begin command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;
    
    if (vkBeginCommandBuffer(m_commandBuffers[m_currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }
    
    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapchainFramebuffers[m_currentImageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchainExtent;
    
    // Set clear color
    VkClearValue clearColor = {{{m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(m_commandBuffers[m_currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Bind the graphics pipeline
    vkCmdBindPipeline(m_commandBuffers[m_currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    
    // Set viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchainExtent.width);
    viewport.height = static_cast<float>(m_swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_commandBuffers[m_currentFrame], 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchainExtent;
    vkCmdSetScissor(m_commandBuffers[m_currentFrame], 0, 1, &scissor);
}

void VulkanRenderer::endFrame() {
    // End render pass
    vkCmdEndRenderPass(m_commandBuffers[m_currentFrame]);
    
    // End command buffer recording
    if (vkEndCommandBuffer(m_commandBuffers[m_currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
    
    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
    
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
    
    // Present the frame
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {m_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_currentImageIndex;
    presentInfo.pResults = nullptr;
    
    VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }
    
    // Advance to next frame
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::renderWorld(const World& world, int cameraX, int cameraY, float zoomLevel) {
    // First, update the world texture with camera information
    updateWorldTexture(world, cameraX, cameraY, zoomLevel);
    
    // Then, update uniform buffer with camera information
    updateUniformBuffer(m_currentFrame, cameraX, cameraY, zoomLevel);
    
    // Bind descriptor sets for the world texture and uniform buffer
    vkCmdBindDescriptorSets(
        m_commandBuffers[m_currentFrame],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        0, 1,
        &m_descriptorSet,
        0, nullptr
    );
    
    // Bind the vertex buffer
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_commandBuffers[m_currentFrame], 0, 1, vertexBuffers, offsets);
    
    // Bind the index buffer
    vkCmdBindIndexBuffer(m_commandBuffers[m_currentFrame], m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    
    // Push constant for material type (will be used for each different material)
    struct PushConstants {
        uint32_t materialType;
    } constants;
    
    // Render each type of material - iterate through all materials in the database
    for (uint32_t materialId = 1; materialId <= 10; materialId++) {
        // Update push constants
        constants.materialType = materialId;
        vkCmdPushConstants(
            m_commandBuffers[m_currentFrame],
            m_pipelineLayout,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushConstants),
            &constants
        );
        
        // Draw the quad
        vkCmdDrawIndexed(m_commandBuffers[m_currentFrame], 6, 1, 0, 0, 0);
    }
    
    // Debug log periodically to show rendering is working
    static int frameCount = 0;
    if (frameCount++ % 60 == 0) {  // Log every 60 frames
        std::cout << "Rendering world with materials (frame " << frameCount << ")" << std::endl;
    }
}

void VulkanRenderer::setClearColor(float r, float g, float b, float a) {
    m_clearColor = {r, g, b, a};
    std::cout << "Setting clear color to: " << r << ", " << g << ", " << b << ", " << a << std::endl;
}

void VulkanRenderer::setViewport(int x, int y, int width, int height) {
    m_viewport = {
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(width), static_cast<float>(height),
        0.0f, 1.0f
    };
    
    m_scissor = {
        {static_cast<int32_t>(x), static_cast<int32_t>(y)},
        {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}
    };
}

void VulkanRenderer::handleResize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
    m_framebufferResized = true;
}

// Implementation stub for the first part of the VulkanRenderer class
bool VulkanRenderer::createInstance() {
    std::cout << "Creating Vulkan instance..." << std::endl;
    
    // Check validation layer support if enabled
    if (m_enableValidationLayers && !checkValidationLayerSupport()) {
        std::cerr << "Validation layers requested, but not available!" << std::endl;
        return false;
    }
    
    // Application info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = m_appName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Dyg-Endless Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    // Create info for the instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    // Get required extensions (including those needed by SDL)
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    // Setup validation layers if enabled
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
        
        // Setup debug messenger creation info for creating the instance
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        debugCreateInfo.pUserData = nullptr;
        
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }
    
    // Create the instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance! Error code: " << result << std::endl;
        return false;
    }
    
    return true;
}

// Helper methods for instance creation
bool VulkanRenderer::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    // Check if all requested layers are available
    for (const char* layerName : m_validationLayers) {
        bool layerFound = false;
        
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        
        if (!layerFound) {
            return false;
        }
    }
    
    return true;
}

std::vector<const char*> VulkanRenderer::getRequiredExtensions() {
    // Get SDL required extensions
    unsigned int sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(nullptr, &sdlExtensionCount, nullptr);
    std::vector<const char*> extensions(sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(nullptr, &sdlExtensionCount, extensions.data());
    
    // Add debug messenger extension if validation layers are enabled
    if (m_enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    return extensions;
}

// Debug callback implementation
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    
    return VK_FALSE; // Always return VK_FALSE
}

bool VulkanRenderer::setupDebugMessenger() {
    if (!m_enableValidationLayers) {
        return true;
    }
    
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
    
    if (createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
        std::cerr << "Failed to set up debug messenger!" << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createSurface(SDL_Window* window) {
    if (SDL_Vulkan_CreateSurface(window, m_instance, &m_surface) != SDL_TRUE) {
        std::cerr << "Failed to create window surface! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

bool VulkanRenderer::pickPhysicalDevice() {
    // Enumerate physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        std::cerr << "Failed to find GPUs with Vulkan support!" << std::endl;
        return false;
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    
    // Find a suitable device
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            m_physicalDevice = device;
            break;
        }
    }
    
    if (m_physicalDevice == VK_NULL_HANDLE) {
        std::cerr << "Failed to find a suitable GPU!" << std::endl;
        return false;
    }
    
    // Print selected device info
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
    std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
    
    return true;
}

bool VulkanRenderer::isDeviceSuitable(VkPhysicalDevice device) {
    // Get device properties and features
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    
    // Check if the device is a discrete GPU and supports geometry shaders
    bool isDiscrete = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    bool hasGeometryShader = deviceFeatures.geometryShader;
    
    // Check for queue families
    QueueFamilyIndices indices = findQueueFamilies(device);
    
    // Check for required extensions
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    
    // Check for swap chain support
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    
    // We prefer discrete GPUs, but will accept integrated ones if that's all we have
    // The device must have a graphics and presentation queue, and support our required extensions
    return indices.isComplete() && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());
    
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

VulkanRenderer::QueueFamilyIndices VulkanRenderer::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    
    // Get queue family properties
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    // Find queue families that support graphics and presentation
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // Check for graphics support
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            indices.graphicsFamilyHasValue = true;
        }
        
        // Check for presentation support
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
            indices.presentFamilyHasValue = true;
        }
        
        // If we found both, break early
        if (indices.isComplete()) {
            break;
        }
        
        i++;
    }
    
    return indices;
}

VulkanRenderer::SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;
    
    // Get surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
    
    // Get surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }
    
    // Get presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }
    
    return details;
}

bool VulkanRenderer::createLogicalDevice() {
    // Get queue family indices
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    
    // Create a set of unique queue families we need
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily,
        indices.presentFamily
    };
    
    // Configure queue priorities (even if we only use one queue)
    float queuePriority = 1.0f;
    
    // Create queue create info for each unique family
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    // Specify the device features we need
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE; // Enable anisotropic filtering
    
    // Create the logical device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    
    // Enable device extensions
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
    
    // For compatibility with older Vulkan implementations, we also set validation layers on the device
    if (m_enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
    // Create the logical device
    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        std::cerr << "Failed to create logical device!" << std::endl;
        return false;
    }
    
    // Get the queue handles
    vkGetDeviceQueue(m_device, indices.graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily, 0, &m_presentQueue);
    
    return true;
}

VkSurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // Prefer SRGB color space if available
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    
    // If preferred format not available, just take the first one
    return availableFormats[0];
}

VkPresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Prefer mailbox (triple buffering) if available
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    
    // FIFO (V-sync) is guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        // If the surface has a specific size, use it
        return capabilities.currentExtent;
    } else {
        // Use the current window size, but clamp to min/max supported
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(m_screenWidth),
            static_cast<uint32_t>(m_screenHeight)
        };
        
        actualExtent.width = std::max(capabilities.minImageExtent.width,
                             std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                              std::min(capabilities.maxImageExtent.height, actualExtent.height));
        
        return actualExtent;
    }
}

bool VulkanRenderer::createSwapChain() {
    // Query swap chain support
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);
    
    // Choose swap chain format, present mode and extent
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    
    // Determine how many images to use in the swap chain (one more than minimum for better performance)
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    
    // Make sure we don't exceed maximum (0 means no maximum)
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    // Create swap chain info
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // Always 1 unless making stereoscopic 3D app
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    // Handle queue families (if graphics and present are different)
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
    
    if (indices.graphicsFamily != indices.presentFamily) {
        // Images can be used across multiple queue families - requires ownership transfers
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        // Best performance - image owned by one queue family at a time
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    
    // No pre-transformation
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    
    // Specify if alpha channel should be used for blending with other windows
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    
    // Use the present mode we selected
    createInfo.presentMode = presentMode;
    
    // Enable clipping (don't render pixels not visible)
    createInfo.clipped = VK_TRUE;
    
    // For when we need to recreate the swap chain (window resize, etc)
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    // Create the swap chain
    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        std::cerr << "Failed to create swap chain!" << std::endl;
        return false;
    }
    
    // Get the swap chain images
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());
    
    // Store the format and extent for later use
    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;
    
    return true;
}

bool VulkanRenderer::createImageViews() {
    // Resize the image views vector to match the number of swap chain images
    m_swapchainImageViews.resize(m_swapchainImages.size());
    
    // Create an image view for each swap chain image
    for (size_t i = 0; i < m_swapchainImages.size(); i++) {
        m_swapchainImageViews[i] = createImageView(
            m_swapchainImages[i],
            m_swapchainImageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
        
        if (m_swapchainImageViews[i] == VK_NULL_HANDLE) {
            std::cerr << "Failed to create image view for swap chain image " << i << std::endl;
            return false;
        }
    }
    
    return true;
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    
    // Default color mapping
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    
    // Subresource range describes what the image's purpose is
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    VkImageView imageView;
    if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    
    return imageView;
}

bool VulkanRenderer::createRenderPass() {
    // Define attachment description
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear values at start
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store for presentation
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Not using stencil
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Not using stencil
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Don't care about previous layout
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout for presentation
    
    // Define attachment reference
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // Index into the attachment array
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout during the subpass
    
    // Define subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Graphics subpass (not compute)
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    // Define subpass dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Implicitly before or after this render pass
    dependency.dstSubpass = 0; // Our subpass index
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Wait for this stage
    dependency.srcAccessMask = 0; // No access before transition
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Operations to wait for
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Write to color attachment
    
    // Create the render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        std::cerr << "Failed to create render pass!" << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createDescriptorSetLayout() {
    // We need two bindings:
    // 1. Uniform buffer for camera and other parameters
    // 2. Combined image sampler for the world texture
    
    // Uniform buffer binding
    VkDescriptorSetLayoutBinding uniformBinding{};
    uniformBinding.binding = 0;
    uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBinding.descriptorCount = 1;
    uniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Access from both shaders
    uniformBinding.pImmutableSamplers = nullptr;
    
    // Sampler binding for the world texture
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Only access from fragment shader
    samplerBinding.pImmutableSamplers = nullptr;
    
    // Combine both bindings
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uniformBinding, samplerBinding};
    
    // Create the descriptor set layout with both bindings
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor set layout!" << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createGraphicsPipeline() {
    try {
        // Load shader code from SPIR-V files
        auto vertShaderCode = readFile("Engine/Assets/Shaders/spirv/particle.vert.spv");
        auto fragShaderCode = readFile("Engine/Assets/Shaders/spirv/particle.frag.spv");
        
        // Create shader modules
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
        
        // Setup vertex shader stage
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main"; // Entry point
        
        // Setup fragment shader stage
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main"; // Entry point
        
        // Combine shader stages
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
        
        // Vertex input state - describes the format of the vertex data
        // For now, we'll use a simple quad with 2D position and texture coordinates
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        
        // Define vertex binding (how vertices are arranged in memory)
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0; // Binding index
        bindingDescription.stride = 4 * sizeof(float); // pos.x, pos.y, texcoord.u, texcoord.v
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to next data entry after each vertex
        
        // Define vertex attributes (what each piece of vertex data represents)
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        
        // Position attribute
        attributeDescriptions[0].binding = 0; // Which binding the data comes from
        attributeDescriptions[0].location = 0; // Location in the shader
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // vec2 (x, y)
        attributeDescriptions[0].offset = 0; // Offset within the vertex data
        
        // Texture coordinate attribute
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT; // vec2 (u, v)
        attributeDescriptions[1].offset = 2 * sizeof(float); // Offset after position
        
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        
        // Input assembly state - describes what kind of geometry will be drawn
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Basic triangles
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        
        // Viewport state - describes the viewport and scissor rectangle
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        // We'll set these dynamically later
        
        // Rasterization state - converts geometry into fragments
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; // Don't clamp fragments to near/far planes
        rasterizer.rasterizerDiscardEnable = VK_FALSE; // Don't discard geometry
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Fill triangles
        rasterizer.lineWidth = 1.0f; // Line width when drawing lines
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Cull back faces
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Vertex order for front faces
        rasterizer.depthBiasEnable = VK_FALSE; // No depth bias
        
        // Multisampling state - for anti-aliasing
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE; // Disable multisampling for now
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        // Color blending state - how to blend colors with what's already in the framebuffer
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE; // Enable blending for particles
        // Standard alpha blending
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE; // Disable logical operations
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        
        // Dynamic state - aspects of the pipeline that can be changed without recreating the pipeline
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        
        // Setup push constants for passing material type to the shader
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Only used in fragment shader
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(uint32_t); // One uint for material type
        
        // Create the pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1; 
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        
        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            std::cerr << "Failed to create pipeline layout!" << std::endl;
            vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
            vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
            return false;
        }
        
        // Create the graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // No depth testing for 2D
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;
        
        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
            std::cerr << "Failed to create graphics pipeline!" << std::endl;
            vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
            vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
            return false;
        }
        
        // Clean up shader modules, they're no longer needed after pipeline creation
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating graphics pipeline: " << e.what() << std::endl;
        return false;
    }
}

bool VulkanRenderer::createFramebuffers() {
    // Resize framebuffers array to match the number of swap chain image views
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());
    
    // Create a framebuffer for each swap chain image view
    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        // For now, we only have a single attachment - the color attachment from the image view
        VkImageView attachments[] = {
            m_swapchainImageViews[i]
        };
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass; // The render pass this framebuffer is compatible with
        framebufferInfo.attachmentCount = 1; // Number of attachments
        framebufferInfo.pAttachments = attachments; // The attachments
        framebufferInfo.width = m_swapchainExtent.width; // Framebuffer width
        framebufferInfo.height = m_swapchainExtent.height; // Framebuffer height
        framebufferInfo.layers = 1; // Number of layers in image arrays
        
        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create framebuffer " << i << std::endl;
            return false;
        }
    }
    
    return true;
}

bool VulkanRenderer::createCommandPool() {
    // Command pools manage the memory used to store command buffers
    // Command buffers submitted to the graphics queue should come from a pool created with the graphics queue family
    
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allow command buffers to be reset individually
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily; // Use graphics queue family
    
    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        std::cerr << "Failed to create command pool!" << std::endl;
        return false;
    }
    
    return true;
}

// Command buffer helper methods
VkCommandBuffer VulkanRenderer::beginSingleTimeCommands() {
    // Create a temporary command buffer for one-time use operations like image layout transitions
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    // End and submit the temporary command buffer, then wait for it to finish
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);
    
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

bool VulkanRenderer::createWorldTexture(uint32_t width, uint32_t height) {
    // Store texture dimensions
    m_worldTexture.width = width;
    m_worldTexture.height = height;
    
    // Create the texture image
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; // RGBA format for pixel data
    
    // Create image
    createImage(
        width, height,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_worldTexture.image,
        m_worldTexture.memory
    );
    
    // Transition image layout to shader optimal
    transitionImageLayout(
        m_worldTexture.image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    
    // Create image view
    m_worldTexture.imageView = createImageView(
        m_worldTexture.image,
        format,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    
    if (m_worldTexture.imageView == VK_NULL_HANDLE) {
        std::cerr << "Failed to create texture image view!" << std::endl;
        return false;
    }
    
    // Create texture sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST; // Pixelated look for particle simulation
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_worldTexture.sampler) != VK_SUCCESS) {
        std::cerr << "Failed to create texture sampler!" << std::endl;
        return false;
    }
    
    return true;
}

void VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
                              VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
                              VkImage& image, VkDeviceMemory& imageMemory) {
    // Create the image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
    }
    
    // Allocate memory for the image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory!");
    }
    
    // Bind memory to the image
    vkBindImageMemory(m_device, image, imageMemory, 0);
}

void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    // Image layout transitions are performed using image memory barriers
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    // Determine source and destination stage masks based on layouts
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // Direct transition from undefined to shader read
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("Unsupported layout transition!");
    }
    
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
    
    endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    
    endSingleTimeCommands(commandBuffer);
}

bool VulkanRenderer::createVertexBuffer() {
    // Define vertex data for a single fullscreen quad
    // Each vertex has position (x, y) and texture coordinates (u, v)
    const std::vector<float> vertices = {
        // Position      // Texcoord
        -1.0f, -1.0f,    0.0f, 1.0f, // Bottom left
         1.0f, -1.0f,    1.0f, 1.0f, // Bottom right
         1.0f,  1.0f,    1.0f, 0.0f, // Top right
        -1.0f,  1.0f,    0.0f, 0.0f  // Top left
    };
    
    VkDeviceSize bufferSize = sizeof(float) * vertices.size();
    
    // Create a staging buffer in host visible memory
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    try {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );
        
        // Map memory and copy vertex data to the staging buffer
        void* data;
        vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(m_device, stagingBufferMemory);
        
        // Create the actual vertex buffer in device local memory (faster access by the GPU)
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_vertexBuffer,
            m_vertexBufferMemory
        );
        
        // Copy data from staging buffer to the vertex buffer
        copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
        
        // Clean up the staging buffer
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating vertex buffer: " << e.what() << std::endl;
        return false;
    }
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                               VkMemoryPropertyFlags properties, VkBuffer& buffer, 
                               VkDeviceMemory& bufferMemory) {
    // Create the buffer object
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }
    
    // Get memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);
    
    // Allocate memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }
    
    // Bind memory to the buffer
    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    // Start a one-time command buffer
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    // Record the copy command
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    // End and submit the command buffer
    endSingleTimeCommands(commandBuffer);
}

bool VulkanRenderer::createIndexBuffer() {
    // Define indices for drawing two triangles as a quad
    const std::vector<uint16_t> indices = {
        0, 1, 2, // First triangle
        2, 3, 0  // Second triangle
    };
    
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    
    // Create a staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    try {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );
        
        // Copy the index data to the staging buffer
        void* data;
        vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(m_device, stagingBufferMemory);
        
        // Create the actual index buffer
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_indexBuffer,
            m_indexBufferMemory
        );
        
        // Copy from staging buffer to the index buffer
        copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
        
        // Clean up staging buffer
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating index buffer: " << e.what() << std::endl;
        return false;
    }
}

bool VulkanRenderer::createUniformBuffer() {
    // Define a uniform buffer for passing information to the shaders
    // This will hold camera position, zoom level, and other global parameters
    
    // Align to the size of vec4 (16 bytes) for better performance
    // The buffer will contain: cameraX, cameraY, zoomLevel, time + any other parameters we need
    VkDeviceSize bufferSize = sizeof(float) * 16; // Reserve space for 4 vec4s
    
    try {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBuffer,
            m_uniformBufferMemory
        );
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating uniform buffer: " << e.what() << std::endl;
        return false;
    }
}

bool VulkanRenderer::createDescriptorPool() {
    // A descriptor pool allocates descriptor sets
    // We need to specify how many descriptor sets and of what types we'll allocate
    
    // Define the descriptor types we need and how many of each
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    
    // Uniform buffer descriptor
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    
    // Combined image sampler descriptor (for the world texture)
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;
    
    // Create the descriptor pool
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1; // We only need one descriptor set for now
    
    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor pool!" << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createDescriptorSets() {
    // A descriptor set is a collection of resources bound to the shader
    // We need to allocate a descriptor set from the pool, and then fill it with our resources
    
    // Allocate a descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;
    
    if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
        std::cerr << "Failed to allocate descriptor set!" << std::endl;
        return false;
    }
    
    // Update the descriptor set with our actual resources
    
    // First descriptor is the uniform buffer
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE; // The entire buffer
    
    // Second descriptor is the world texture
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_worldTexture.imageView;
    imageInfo.sampler = m_worldTexture.sampler;
    
    // Descriptor write operations
    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    
    // Uniform buffer descriptor
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_descriptorSet;
    descriptorWrites[0].dstBinding = 0; // Binding point in the shader
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    
    // Image sampler descriptor
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_descriptorSet;
    descriptorWrites[1].dstBinding = 1; // Binding point in the shader
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;
    
    // Update the descriptor set
    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    
    return true;
}

bool VulkanRenderer::createCommandBuffers() {
    // Command buffers record commands for execution on the GPU
    // We create one command buffer per frame in flight
    
    // Resize the command buffers vector
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    
    // Allocate the command buffers
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Can be submitted directly to a queue
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
    
    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        std::cerr << "Failed to allocate command buffers!" << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanRenderer::createSyncObjects() {
    // Synchronization objects ensure proper ordering of operations on the GPU
    
    // Resize the synchronization object vectors
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight.resize(m_swapchainImages.size(), VK_NULL_HANDLE);
    
    // Create the semaphores and fences
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so we don't wait forever on first frame
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            
            std::cerr << "Failed to create synchronization objects for frame " << i << std::endl;
            return false;
        }
    }
    
    return true;
}

void VulkanRenderer::cleanupSwapChain() {
    // Destroy framebuffers
    for (auto framebuffer : m_swapchainFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    
    // Destroy image views
    for (auto imageView : m_swapchainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }
    
    // Destroy swap chain
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void VulkanRenderer::recreateSwapChain() {
    // Handle minimization (wait until window is visible again)
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        // Get current window size
        SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &width, &height);
        SDL_PumpEvents(); // Process events to get minimization state
    }
    
    // Wait for device to finish operations
    vkDeviceWaitIdle(m_device);
    
    // Clean up old swap chain resources
    cleanupSwapChain();
    
    // Create new swap chain
    createSwapChain();
    createImageViews();
    createFramebuffers();
    
    // Update screen dimensions
    m_screenWidth = width;
    m_screenHeight = height;
}

void VulkanRenderer::updateWorldTexture(const World& world, int cameraX, int cameraY, float zoomLevel) {
    // Create a temporary buffer with world texture data
    const uint32_t width = m_worldTexture.width;
    const uint32_t height = m_worldTexture.height;
    const VkDeviceSize bufferSize = width * height * 4; // RGBA = 4 bytes per pixel
    
    // Create pixel buffer for the world texture
    std::vector<uint8_t> pixels(width * height * 4, 0);
    
    // Count the number of non-empty pixels for debugging
    int nonEmptyPixels = 0;
    
    // Fill the pixel buffer with actual world data
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t pixelIndex = (y * width + x) * 4;
            
            // Convert screen coordinates to world coordinates
            // The camera coordinates should be at the center of the screen
            // Adjust for zoom level and offset from center
            int screenCenterX = width / 2;
            int screenCenterY = height / 2;
            int offsetX = x - screenCenterX;
            int offsetY = y - screenCenterY;
            
            // Calculate world coordinates based on camera position and zoom
            int worldX = cameraX + static_cast<int>(offsetX / zoomLevel);
            int worldY = cameraY + static_cast<int>(offsetY / zoomLevel);
            
            // Get the particle at this position
            auto particle = world.GetParticle(worldX, worldY);
            
            if (!particle.IsEmpty()) {
                nonEmptyPixels++;
                
                // Get material info
                const auto& material = MaterialDatabase::Get().GetMaterial(particle.materialID);
                
                // Use very visible colors - bright red for all particles for now
                pixels[pixelIndex + 0] = 255; // R - Bright red regardless of material
                pixels[pixelIndex + 1] = 0;   // G
                pixels[pixelIndex + 2] = 0;   // B
                pixels[pixelIndex + 3] = 255; // A - fully opaque
                
                // Print the first few particles for debug
                static int debugCount = 0;
                if (debugCount < 5) {
                    std::cout << "DEBUG: Set pixel at (" << x << "," << y << ") to material " 
                              << (int)particle.materialID << " (R=" << (int)pixels[pixelIndex+0]
                              << ", G=" << (int)pixels[pixelIndex+1]
                              << ", B=" << (int)pixels[pixelIndex+2]
                              << ", A=" << (int)pixels[pixelIndex+3] << ")" << std::endl;
                    debugCount++;
                }
            } else {
                // Empty space - use a faint grid for visibility
                if ((x % 64 == 0) || (y % 64 == 0)) {
                    // Draw faint grid lines
                    pixels[pixelIndex + 0] = 50;  // R
                    pixels[pixelIndex + 1] = 50;  // G
                    pixels[pixelIndex + 2] = 50;  // B
                    pixels[pixelIndex + 3] = 50;  // A - semi-transparent
                } else {
                    // Empty space - transparent black
                    pixels[pixelIndex + 0] = 0;    // R
                    pixels[pixelIndex + 1] = 0;    // G
                    pixels[pixelIndex + 2] = 0;    // B
                    pixels[pixelIndex + 3] = 0;    // A (transparent)
                }
            }
        }
    }
    
    // Debug - log if we found any particles
    static int frameCount = 0;
    if (frameCount++ % 60 == 0 || nonEmptyPixels > 0) {
        std::cout << "World texture update: found " << nonEmptyPixels << " non-empty pixels" << std::endl;
    }
    
    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                stagingBuffer, stagingBufferMemory);
    
    // Copy pixel data to staging buffer
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, pixels.data(), bufferSize);
    vkUnmapMemory(m_device, stagingBufferMemory);
    
    // Prepare texture for transfer
    transitionImageLayout(m_worldTexture.image, VK_FORMAT_R8G8B8A8_UNORM, 
                          VK_IMAGE_LAYOUT_UNDEFINED, 
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    // Copy from buffer to image
    copyBufferToImage(stagingBuffer, m_worldTexture.image, width, height);
    
    // Transition to shader readable format
    transitionImageLayout(m_worldTexture.image, VK_FORMAT_R8G8B8A8_UNORM, 
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // Cleanup staging buffer
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage, int cameraX, int cameraY, float zoomLevel) {
    // Define uniform buffer data
    struct UniformBufferObject {
        float cameraX;
        float cameraY;
        float zoomLevel;
        float time;  // For animation effects
        
        // Padding to maintain 16-byte alignment
        float padding[12];
    };
    
    // Update the UBO with current frame information
    UniformBufferObject ubo{};
    
    // Convert camera position to normalized device coordinates
    ubo.cameraX = static_cast<float>(cameraX) / m_screenWidth;
    ubo.cameraY = static_cast<float>(cameraY) / m_screenHeight;
    ubo.zoomLevel = zoomLevel;
    
    // Update time for animation effects
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    ubo.time = time;
    
    // Map memory and copy data to the uniform buffer
    void* data;
    vkMapMemory(m_device, m_uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
    memcpy(data, &ubo, sizeof(UniformBufferObject));
    vkUnmapMemory(m_device, m_uniformBufferMemory);
}

// Implementation of memoryType helper
uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

// Shader loading utilities
std::vector<char> VulkanRenderer::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    return buffer;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
    
    return shaderModule;
}

// Implementation of debug messenger extension functions
VkResult VulkanRenderer::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanRenderer::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

} // namespace Engine
