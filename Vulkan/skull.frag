#version 450
#extension GL_ARB_separate_shader_objects : enable //<-- needs to be there for Vulkan to work

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 modelPos;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler2D texSampler;

vec4 black = vec4(0.0,0.0,0.0,1.0);
vec4 white = vec4(1.0,1.0,1.0,1.0);

void main() {
		vec3 pos =  modelPos * 1.0;
		float total = floor(pos.x) + floor(pos.y) + floor(pos.z);
		bool isEven = mod(total, 2.0) == 0.0;
		outColor = isEven ? black : white;
	/*{
		float c = 5.0f;
		outColor = vec4((modelPos.x + 4) * 0.2, (modelPos.y + 5) * 0.1, (modelPos.z + 2) * 0.2, 1.0) * max(0.1, sin(modelPos.x * c) * sin(modelPos.y * c) * sin(modelPos.z * c));
	}*/
}
