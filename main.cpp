#include "Engine/Core/Application.h"
#include "Engine/Simulation/Material.h"
#include "Engine/Procedural/World.h"
#include "Engine/Procedural/ProceduralGenerator.h"
#include "Engine/Rendering/Renderer.h"
#include <iostream>
#include <string>
#include <SDL2/SDL.h>
#include <memory>
#include <chrono>
#include <thread>
#include <csignal>

// Global quit flag
volatile sig_atomic_t g_quit = 0;

// Signal handler for Ctrl+C
void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    g_quit = 1;
    
    // Ensure SDL gets a chance to process the quit event
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* WINDOW_TITLE = "Dyg-Endless Sand Simulation";

const int TARGET_FPS = 60;
const double FRAME_TIME = 1000.0 / TARGET_FPS; // ms per frame

// Camera parameters
int cameraX = 640; // Center of screen
int cameraY = 360; // Center of screen
float zoomLevel = 0.5f; // Zoomed out to see more
const float ZOOM_STEP = 0.1f;
const float MIN_ZOOM = 0.5f;
const float MAX_ZOOM = 4.0f;

// Mouse parameters
bool leftMousePressed = false;
bool rightMousePressed = false;
bool middleMousePressed = false;
int mouseX = 0, mouseY = 0;
int prevMouseX = 0, prevMouseY = 0;

// Selected material for placing
uint8_t selectedMaterial = 1; // Sand by default

// Brush settings
int brushSize = 3; // Diameter of brush in particles (3x3)
const int MIN_BRUSH_SIZE = 1;
const int MAX_BRUSH_SIZE = 20;

int main(int argc, char** argv) {
    std::cout << "Starting Dyg-Endless Sand Simulation Engine" << std::endl;
    
    // Register signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create SDL window
    SDL_Window* window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Initialize the material database
    Engine::MaterialDatabase::Initialize();
    
    // Try to load materials from config (fallback to defaults if it fails)
    try {
        Engine::MaterialDatabase::LoadMaterials("Engine/Assets/Configs/materials.json");
    } catch (const std::exception& e) {
        std::cerr << "Error loading materials: " << e.what() << std::endl;
        std::cerr << "Falling back to default materials." << std::endl;
    }
    
    // Create a world
    Engine::World world;
    
    // Create a procedural generator
    Engine::ProceduralGenerator generator;
    
    // Create some initial chunks around the origin
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            Engine::Chunk* chunk = world.CreateChunk(glm::ivec2(x, y));
            generator.GenerateChunk(chunk);
        }
    }
    
    // Create the renderer
    auto renderer = std::make_unique<Engine::Renderer>(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!renderer->initialize(window)) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Main loop
    bool quit = false;
    SDL_Event e;
    
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    // Set up a proper handler for window close button
    SDL_SetHint(SDL_HINT_VIDEO_X11_XRANDR, "1");
    SDL_SetHint(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "1");
    
    while (!quit && !g_quit) {
        // Calculate frame time and delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto frameTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastFrameTime
        ).count();
        
        // Debug message every 5 seconds
        static auto lastDebugTime = std::chrono::high_resolution_clock::now();
        auto debugElapsed = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - lastDebugTime
        ).count();
        if (debugElapsed >= 5) {
            std::cout << "Running... Press ESC to exit" << std::endl;
            std::cout << "Controls: Left-click to place, Right-click to erase" << std::endl;
            std::cout << "         Shift+Plus/Minus to adjust brush size (current: " << brushSize << ")" << std::endl;
            std::cout << "         Keys 1-0 to select materials (current: " 
                      << Engine::MaterialDatabase::Get().GetMaterial(selectedMaterial).name << ")" << std::endl;
            lastDebugTime = currentTime;
        }
        
        // Process events (check before AND after rendering)
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                std::cout << "Received SDL_QUIT event. Exiting..." << std::endl;
                quit = true;
                break;
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    renderer->handleResize(e.window.data1, e.window.data2);
                } else if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    std::cout << "Window close event received. Exiting..." << std::endl;
                    quit = true;
                    break;
                }
            } else if (e.type == SDL_KEYDOWN) {
                // Handle key presses
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        std::cout << "ESC key pressed. Exiting..." << std::endl;
                        quit = true;
                        break;
                    
                    // Camera controls
                    case SDLK_w:
                    case SDLK_UP:
                        cameraY -= 10;
                        break;
                    case SDLK_s:
                    case SDLK_DOWN:
                        cameraY += 10;
                        break;
                    case SDLK_a:
                    case SDLK_LEFT:
                        cameraX -= 10;
                        break;
                    case SDLK_d:
                    case SDLK_RIGHT:
                        cameraX += 10;
                        break;
                        
                    // Zoom controls
                    case SDLK_EQUALS:
                    case SDLK_PLUS:
                        if (SDL_GetModState() & KMOD_SHIFT) {
                            // Increase brush size with Shift+Plus
                            brushSize += 1;
                            if (brushSize > MAX_BRUSH_SIZE) brushSize = MAX_BRUSH_SIZE;
                            std::cout << "Brush size: " << brushSize << std::endl;
                        } else {
                            // Zoom in with Plus
                            zoomLevel += ZOOM_STEP;
                            if (zoomLevel > MAX_ZOOM) zoomLevel = MAX_ZOOM;
                        }
                        break;
                    case SDLK_MINUS:
                        if (SDL_GetModState() & KMOD_SHIFT) {
                            // Decrease brush size with Shift+Minus
                            brushSize -= 1;
                            if (brushSize < MIN_BRUSH_SIZE) brushSize = MIN_BRUSH_SIZE;
                            std::cout << "Brush size: " << brushSize << std::endl;
                        } else {
                            // Zoom out with Minus
                            zoomLevel -= ZOOM_STEP;
                            if (zoomLevel < MIN_ZOOM) zoomLevel = MIN_ZOOM;
                        }
                        break;
                        
                    // Material selection
                    case SDLK_1:
                        selectedMaterial = 1; // Sand
                        std::cout << "Selected material: Sand" << std::endl;
                        break;
                    case SDLK_2:
                        selectedMaterial = 2; // Water
                        std::cout << "Selected material: Water" << std::endl;
                        break;
                    case SDLK_3:
                        selectedMaterial = 3; // Stone
                        std::cout << "Selected material: Stone" << std::endl;
                        break;
                    case SDLK_4:
                        selectedMaterial = 4; // Fire
                        std::cout << "Selected material: Fire" << std::endl;
                        break;
                    case SDLK_5:
                        selectedMaterial = 5; // Wood
                        std::cout << "Selected material: Wood" << std::endl;
                        break;
                    case SDLK_6:
                        selectedMaterial = 6; // Gunpowder
                        std::cout << "Selected material: Gunpowder" << std::endl;
                        break;
                    case SDLK_7:
                        selectedMaterial = 7; // Acid
                        std::cout << "Selected material: Acid" << std::endl;
                        break;
                    case SDLK_8:
                        selectedMaterial = 8; // Oil
                        std::cout << "Selected material: Oil" << std::endl;
                        break;
                    case SDLK_9:
                        selectedMaterial = 9; // Smoke
                        std::cout << "Selected material: Smoke" << std::endl;
                        break;
                    case SDLK_0:
                        selectedMaterial = 10; // Salt
                        std::cout << "Selected material: Salt" << std::endl;
                        break;
                    default:
                        break;
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    leftMousePressed = true;
                } else if (e.button.button == SDL_BUTTON_RIGHT) {
                    rightMousePressed = true;
                } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                    middleMousePressed = true;
                    prevMouseX = e.button.x;
                    prevMouseY = e.button.y;
                }
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    leftMousePressed = false;
                } else if (e.button.button == SDL_BUTTON_RIGHT) {
                    rightMousePressed = false;
                } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                    middleMousePressed = false;
                }
            } else if (e.type == SDL_MOUSEMOTION) {
                mouseX = e.motion.x;
                mouseY = e.motion.y;
                
                // Handle camera panning with middle mouse button
                if (middleMousePressed) {
                    int deltaX = mouseX - prevMouseX;
                    int deltaY = mouseY - prevMouseY;
                    
                    // Invert and scale movement (faster when zoomed out)
                    cameraX -= static_cast<int>(deltaX / zoomLevel);
                    cameraY -= static_cast<int>(deltaY / zoomLevel);
                    
                    prevMouseX = mouseX;
                    prevMouseY = mouseY;
                }
            } else if (e.type == SDL_MOUSEWHEEL) {
                // Mouse wheel for zooming
                if (e.wheel.y > 0) { // Scroll up
                    zoomLevel += ZOOM_STEP;
                    if (zoomLevel > MAX_ZOOM) zoomLevel = MAX_ZOOM;
                } else if (e.wheel.y < 0) { // Scroll down
                    zoomLevel -= ZOOM_STEP;
                    if (zoomLevel < MIN_ZOOM) zoomLevel = MIN_ZOOM;
                }
            }
        }
        
        // Handle particle placement and deletion
        if (leftMousePressed || rightMousePressed) {
            // Print debug info - this will help confirm if the mouse input is being registered
            std::cout << "Mouse action: left=" << (leftMousePressed ? "true" : "false") 
                      << ", right=" << (rightMousePressed ? "true" : "false") 
                      << ", pos=(" << mouseX << "," << mouseY << ")" << std::endl;
            
            // Convert screen coordinates to world coordinates
            // The camera coordinates should be at the center of the screen
            int screenCenterX = WINDOW_WIDTH / 2;
            int screenCenterY = WINDOW_HEIGHT / 2;
            int offsetX = mouseX - screenCenterX;
            int offsetY = mouseY - screenCenterY;
            
            // Calculate world coordinates based on camera position and zoom
            int worldX = cameraX + static_cast<int>(offsetX / zoomLevel);
            int worldY = cameraY + static_cast<int>(offsetY / zoomLevel);
            
            std::cout << "World coords: (" << worldX << "," << worldY << ")" << std::endl;
            
            // Always place exactly one particle at cursor for debugging
            if (leftMousePressed) {
                // Directly place a single particle regardless of what's there
                std::cout << "Placing particle with material ID " << (int)selectedMaterial << std::endl;
                world.SetParticle(worldX, worldY, Engine::Particle(selectedMaterial));
                
                // Check if it was placed successfully
                const Engine::Particle& placed = world.GetParticle(worldX, worldY);
                std::cout << "After placement: materialID=" << (int)placed.materialID << std::endl;
            }
            
            // Old brush code with radius
            if (false) { // Disabled temporarily for debugging
                // Determine brush radius (half of diameter)
                int radius = brushSize / 2;
                
                // Place or erase material based on which button is pressed
                for (int dy = -radius; dy <= radius; dy++) {
                    for (int dx = -radius; dx <= radius; dx++) {
                        // Calculate distance from center of brush
                        float distance = sqrt(dx*dx + dy*dy);
                        
                        // Only affect pixels within the brush radius
                        if (distance <= radius) {
                            int targetX = worldX + dx;
                            int targetY = worldY + dy;
                            
                            if (leftMousePressed) {
                                // Place material only if the target position is empty
                                const Engine::Particle& existingParticle = world.GetParticle(targetX, targetY);
                                if (existingParticle.IsEmpty()) {
                                    world.SetParticle(targetX, targetY, Engine::Particle(selectedMaterial));
                                }
                            } else if (rightMousePressed) {
                                // Erase material (set to empty/air, material ID 0)
                                world.SetParticle(targetX, targetY, Engine::Particle(0));
                            }
                        }
                    }
                }
            }
        }
        
        // Check if we should quit before expensive operations
        if (quit || g_quit) {
            break;
        }
        
        // Update simulation (based on delta time)
        float dt = frameTimeMs / 1000.0f; // Convert to seconds
        world.Update(dt);
        
        // Render
        try {
            renderer->beginFrame();
            renderer->renderWorld(world, cameraX, cameraY, zoomLevel);
            renderer->endFrame();
        } catch (const std::exception& e) {
            std::cerr << "Rendering error: " << e.what() << std::endl;
            // Don't quit on rendering errors, just skip this frame
        }
        
        // Process events again after rendering
        SDL_PumpEvents(); // Update the event queue
        if (SDL_HasEvent(SDL_QUIT)) {
            std::cout << "Quit event detected after rendering" << std::endl;
            quit = true;
        }
        
        // Cap frame rate if frame time less than target
        if (frameTimeMs < FRAME_TIME) {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(FRAME_TIME - frameTimeMs)
            ));
        }
        
        lastFrameTime = currentTime;
    }
    
    // Save the world before exiting
    world.Save("worlddata");
    
    // Cleanup
    renderer->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "Shutting down Sand Simulation Engine" << std::endl;
    return 0;
}