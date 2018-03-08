#pragma once
#include <glm/detail/type_vec2.hpp>
#include <glm/detail/type_vec3.hpp>
#include <vulkan/vulkan.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		vk::VertexInputBindingDescription binding_description;
		binding_description.binding = 0;
		binding_description.stride = sizeof(Vertex);
		binding_description.inputRate = vk::VertexInputRate::eVertex; // Replace with VK_VERTEX_INPUT_RATE_INSTANCE for instancing

		return binding_description;
	}

	static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
};