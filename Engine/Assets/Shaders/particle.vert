#version 450

// Input vertex data
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

// Output to fragment shader
layout(location = 0) out vec2 fragTexCoord;

// Uniform values that change each frame
layout(binding = 0) uniform UniformBufferObject {
    float time;       // Total elapsed time
    float deltaTime;  // Time since last frame
    vec2 resolution;  // Screen resolution
    vec2 worldOffset; // World camera position (x, y)
    float zoomLevel;  // Camera zoom level
} ubo;

void main() {
    // Pass texture coordinates to fragment shader
    fragTexCoord = inTexCoord;
    
    // Final vertex position
    gl_Position = vec4(inPosition, 0.0, 1.0);
}