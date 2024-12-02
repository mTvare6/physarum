
#version 460 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba8, binding = 1) uniform image2D outputImage;
uniform float decayAmount;
void main() {
  ivec2 textureSize = imageSize(outputImage);
  ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
    
  if (texCoord.x >= textureSize.x || texCoord.y >= textureSize.y) return;
  vec4 color = imageLoad(outputImage, texCoord);
  color.rgb -= decayAmount;
  imageStore(outputImage, texCoord, color);
}
