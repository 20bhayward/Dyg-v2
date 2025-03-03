#pragma once

#include <memory>
#include <string>
#include <vector>
#include <array>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Engine {

// Forward declarations
class World;
class Chunk;

struct VulkanTexture {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
    VkSampler sampler;
    uint32_t width;
    uint32_t height;
};

class VulkanRenderer {
public:
    VulkanRenderer(int screenWidth, int screenHeight, const std::string& appName = "Dyg Particle Simulation");
    ~VulkanRenderer();

    bool initialize(SDL_Window* window);
    void cleanup();
    
    void beginFrame();
    void endFrame();
    
    void renderWorld(const World& world, int cameraX = 0, int cameraY = 0, float zoomLevel = 1.0f);
    
    // Setters for rendering properties
    void setClearColor(float r, float g, float b, float a = 1.0f);
    void setViewport(int x, int y, int width, int height);
    
    // Helper function to check if Vulkan is available
    static bool isVulkanAvailable();
    
    // Window resize handling
    void handleResize(int width, int height);

private:
    // Basic Vulkan objects
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkSurfaceKHR m_surface;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    
    // Swapchain
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    
    // Command processing
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    
    // Sync objects
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_imagesInFlight;
    size_t m_currentFrame;
    
    // Render pass
    VkRenderPass m_renderPass;
    
    // Pipeline
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    
    // Descriptor sets
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;
    
    // Vertex/index buffers for quad rendering
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
    
    // Uniform buffer for shader parameters
    VkBuffer m_uniformBuffer;
    VkDeviceMemory m_uniformBufferMemory;
    
    // World texture for rendering particles
    VulkanTexture m_worldTexture;
    
    // Screen dimensions
    int m_screenWidth;
    int m_screenHeight;
    
    // Application name
    std::string m_appName;
    
    // Clear color for background
    std::array<float, 4> m_clearColor;
    
    // Viewport dimensions
    VkViewport m_viewport;
    VkRect2D m_scissor;
    
    // Current frame state
    uint32_t m_currentImageIndex;
    bool m_framebufferResized;
    
    // Max frames in flight
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    
    // Validation layers to enable for debugging
    std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    // Required device extensions
    std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    // Enable validation layers in debug builds
#ifdef NDEBUG
    const bool m_enableValidationLayers = false;
#else
    // Set to false by default as validation layers might not be available
    const bool m_enableValidationLayers = false;
#endif
    
    // Initialization methods
    bool createInstance();
    bool setupDebugMessenger();
    bool createSurface(SDL_Window* window);
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapChain();
    bool createImageViews();
    bool createRenderPass();
    bool createDescriptorSetLayout();
    bool createGraphicsPipeline();
    bool createFramebuffers();
    bool createCommandPool();
    bool createWorldTexture(uint32_t width, uint32_t height);
    bool createVertexBuffer();
    bool createIndexBuffer();
    bool createUniformBuffer();
    bool createDescriptorPool();
    bool createDescriptorSets();
    bool createCommandBuffers();
    bool createSyncObjects();
    
    // Helper methods for init/shutdown
    void cleanupSwapChain();
    void recreateSwapChain();
    
    // Vulkan utility methods
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    
    // Device selection and capability query
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    
    // Swap chain creation helpers
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    // Queue family helper
    struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
    };
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    
    // Shader loading
    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    
    // Memory management helpers
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    
    // Command buffer helpers
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    
    // Update uniform buffer with current frame info
    void updateUniformBuffer(uint32_t currentImage, int cameraX, int cameraY, float zoomLevel);
    
    // Update world texture from simulation data
    void updateWorldTexture(const World& world, int cameraX = 0, int cameraY = 0, float zoomLevel = 1.0f);
    
    // Debug callback function for validation layers
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

} // namespace Engine