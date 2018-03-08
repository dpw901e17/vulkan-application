#version 450
#extension GL_ARB_separate_shader_objects : enable //<-- needs to be there for Vulkan to work

layout(binding = 0) uniform UniformBufferObjectView {
  mat4 projection;
  mat4 view;
} uboView;

layout(binding = 1) uniform UniformBufferObjectInstance {
  mat4 model;
} uboInstance;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 modelPos;

//output to be sent through the entire rest of pipeline.
out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = uboView.projection * uboView.view * uboInstance.model * vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
	modelPos = inPosition;
}