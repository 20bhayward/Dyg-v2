#version 450

// Input from vertex shader
layout(location = 0) in vec2 fragTexCoord;

// Output color
layout(location = 0) out vec4 outColor;

// Uniform values
layout(binding = 0) uniform UniformBufferObject {
    float time;       // Total elapsed time
    float deltaTime;  // Time since last frame
    vec2 resolution;  // Screen resolution
    vec2 worldOffset; // World camera position (x, y)
    float zoomLevel;  // Camera zoom level
} ubo;

// Sampler for the world texture
layout(binding = 1) uniform sampler2D worldTexture;

// Material info - used for custom rendering effects per material
layout(push_constant) uniform PushConstants {
    uint materialType; // Material type being rendered
} material;

// Function to add subtle noise to make materials look more natural
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(443.897, 441.423, 437.195));
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

// Noise function for material texture
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f*f*(3.0-2.0*f); // Smooth interpolation
    
    float n = mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
    
    return n;
}

// Function to add a glow effect for emissive materials
vec3 addGlow(vec3 color, float strength) {
    // Pulsating glow effect based on time
    float glowIntensity = 0.5 + 0.5 * sin(ubo.time * 2.0);
    return color * (1.0 + strength * glowIntensity);
}

// Function to add liquid shimmer effect
vec3 addLiquidEffect(vec3 color, vec2 coord) {
    // Wavy pattern for liquids
    float wave = sin(coord.x * 20.0 + ubo.time * 2.0) * 0.03 + 
                sin(coord.y * 15.0 + ubo.time * 1.5) * 0.02;
    
    // Add subtle blue-white highlights
    vec3 highlight = vec3(0.7, 0.8, 1.0);
    return mix(color, highlight, wave * 0.3);
}

void main() {
    // Sample the texture
    vec4 texColor = texture(worldTexture, fragTexCoord);
    
    // Base case - just use the texture color
    vec3 finalColor = texColor.rgb;
    float alpha = texColor.a;
    
    // Skip processing for empty/air cells
    if (alpha < 0.01)
        discard;
    
    // Add noise-based detail to give texture to materials
    vec2 noiseCoord = fragTexCoord * vec2(100.0, 100.0) + ubo.worldOffset;
    float noiseValue = noise(noiseCoord);
    
    // Apply material-specific effects
    uint materialID = material.materialType;
    
    if (materialID == 1) { // Sand
        // Add grainy texture to sand
        finalColor *= 0.9 + 0.2 * noiseValue;
    }
    else if (materialID == 2) { // Water
        // Add flowing/shimmering effect to water
        finalColor = addLiquidEffect(finalColor, fragTexCoord + vec2(0.0, ubo.time * 0.05));
        // Make water slightly transparent
        alpha = 0.8;
    }
    else if (materialID == 4) { // Fire
        // Add glowing/pulsating effect to fire
        finalColor = addGlow(finalColor, 0.7);
        
        // Add flickering effect
        float flicker = noise(vec2(ubo.time * 6.0, fragTexCoord.y * 10.0));
        finalColor *= 0.8 + 0.4 * flicker;
    }
    else if (materialID == 10) { // Lava
        // Add glowing effect to lava
        finalColor = addGlow(finalColor, 0.4);
        
        // Add slow pulsing
        float pulse = 0.8 + 0.2 * sin(ubo.time * 0.5 + fragTexCoord.x * 5.0 + fragTexCoord.y * 3.0);
        finalColor *= pulse;
    }
    
    // Add subtle vignette effect for better appearance
    vec2 center = vec2(0.5, 0.5);
    float vignette = 1.0 - 0.3 * length(fragTexCoord - center);
    finalColor *= vignette;
    
    // Output final color with alpha
    outColor = vec4(finalColor, alpha);
}