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
uniform int senseRadius;
uniform float senseOffset;
uniform float senseAngle;
uniform float agentSpeed;
uniform float agentRotateFactor;
uniform float agentDepositAmount;

float sense(agent ag, float senseOffsetAngle){
  ivec2 textureSize = imageSize(outputImage);
  float senseDirection = senseOffsetAngle + ag.angle;
  vec2 dir = vec2(cos(senseDirection), sin(senseDirection));
  ivec2 senseCenter = ivec2(ag.position+dir*senseOffset);
  vec4 sum = vec4(0.0);
  for (int x = -senseRadius; x <= senseRadius; ++x) {
    for (int y = -senseRadius; y <= senseRadius; ++y) {
      ivec2 pos = senseCenter + ivec2(x, y);
      if (pos.x >= 0 && pos.x < textureSize.x && pos.y >= 0 && pos.y < textureSize.y) {
        sum += imageLoad(outputImage, pos);
      }
    }
  }
  return sum.r;
}

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
  const uint ieeeMantissa = 0x007FFFFFu; 
  const uint ieeeOne      = 0x3F800000u; 

  m &= ieeeMantissa;                     
  m |= ieeeOne;                          

  float  f = uintBitsToFloat( m );       
  return f - 1.0;                        
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


  float wForward = sense(ag, 0);
  float wLeft = sense(ag, senseAngle);
  float wRight = sense(ag, -senseAngle);

  if (wForward > wLeft && wForward > wRight){
    agents[id].angle += 0;
  }
  else if (wForward < wLeft && wForward < wRight){
    agents[id].angle += (2.0*randFactor-1)*agentRotateFactor;
  }
  else if (wRight > wLeft){
    agents[id].angle -= randFactor * agentRotateFactor;
  }
  else if (wLeft > wRight){
    agents[id].angle += randFactor * agentRotateFactor;
  }



  vec2 dir = vec2(cos(ag.angle), sin(ag.angle));
  vec2 newpos = ag.position + dir * agentSpeed;

  
  if (newpos.x < 5.0 || (newpos.x+5.0) >= size.x || newpos.y < 5.0 || (newpos.y+5.0) >= size.y) {
    newpos.x = clamp(newpos.x, 5.0, size.x - 5.0);
    newpos.y = clamp(newpos.y, 5.0, size.y - 5.0);
    agents[id].angle = randFactor * 6.28;
  }

  
  ivec2 pos = ivec2(newpos);
  imageStore(outputImage, pos, imageLoad(outputImage, pos)+agentDepositAmount);
  agents[id].position = newpos;
}
