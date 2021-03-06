#version 450
#extension GL_ARB_separate_shader_objects : enable //<-- needs to be there for Vulkan to work

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 modelPos;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler2D texSampler;

void main() {
	outColor = texture(texSampler, fragTexCoord);
}
