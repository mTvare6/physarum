#version 460 core
layout (local_size_x = 256) in;

struct agent {
    vec2 position;
    float angle;
};

layout(std430, binding = 0) buffer AgentBuffer {
    agent agents[];
};

layout(rgba8, binding = 1) uniform image2D outputImage;

uniform int num_agents;

uint hash(uint state) {
    state ^= uint(2747636419u);
    state *= uint(2654435769u);
    state ^= state >> 16;
    state *= uint(2654435769u);
    state ^= state >> 16;
    state *= uint(2654435769u);
    return state;
}
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

float scale01(uint state) {
    return float(state) / float(0xffffffff);
}

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= num_agents) return;

    agent ag = agents[id];
    vec2 size = imageSize(outputImage);
    uint random = hash(uint(ag.position.x + ag.position.y * size.x) + hash(id));
    float randFactor = floatConstruct(random);
    vec2 dir = vec2(cos(ag.angle), sin(ag.angle));

    vec2 newpos = ag.position + dir * 3.0;

    // Check if the new position is outside the image bounds and wrap it
    if (newpos.x < 5.0 || (newpos.x+5.0) >= size.x || newpos.y < 5.0 || (newpos.y+5.0) >= size.y) {
        newpos.x = clamp(newpos.x, 5.0, size.x - 5.0);
        newpos.y = clamp(newpos.y, 5.0, size.y - 5.0);
        agents[id].angle = randFactor * 6.28;
    }

    // Store the new position in the image (at the new position)
    imageStore(outputImage, ivec2(newpos), vec4(1.0, 1.0, 1.0, 1.0));
    agents[id].position = newpos;
}
