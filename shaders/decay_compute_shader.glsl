#version 460 core

layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba8, binding = 1) uniform image2D outputImage;

uniform float decayAmount;
uniform float blurAmount;

void main() {
  ivec2 textureSize = imageSize(outputImage);
  ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
  if (texCoord.x >= textureSize.x || texCoord.y >= textureSize.y) return;

  vec4 blurredColor = vec4(0.0);
  vec4 currentColor = imageLoad(outputImage, texCoord);
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      ivec2 neighborCoord = texCoord + ivec2(x, y);
      if (neighborCoord.x >= 0 && neighborCoord.x < textureSize.x &&
        neighborCoord.y >= 0 && neighborCoord.y < textureSize.y) {
        blurredColor += imageLoad(outputImage, neighborCoord);  
      }
    }
  }

  blurredColor /= 9.0;
  vec4 lerpColor = mix(currentColor, blurredColor, blurAmount);
  lerpColor.rgb -= decayAmount;
  lerpColor.rgb = clamp(lerpColor.rgb, vec3(0.0), vec3(1.0));

  imageStore(outputImage, texCoord, lerpColor);
}
