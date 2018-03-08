#pragma once
#include <vector>
#include <fstream>
#include "../scene-window-system/RenderObject.h"

struct PipelineStatisticsResult
{
	uint64_t inputAssemblyVertices,
		inputAssemblyPrimitives,
		vertexShaderInvocations,
		geometryShaderInvocations,
		geometryShaderPrimitives,
		clippingInvocations,
		clippingPrimitives,
		fragmentShaderInvocations,
		tesselationControlShaderPatches,
		tesselationEvaluationShaderInvocations,
		computeShaderInvocations;
};

struct DrawRenderObjectsInfo
{
	const RenderObject* roArr = nullptr;
	size_t roArrCount = 0;
	size_t roArrStride = 0;	//<-- the last thread can have a difference between count and stride
	uint32_t dynamicAllignment;
	vk::CommandBuffer* commandBuffer;
	uint32_t numOfIndices = 0;
	vk::PipelineLayout* pipelineLayout;
	vk::DescriptorSet* descriptorSet;
	vk::QueryPool* queryPool;
	uint32_t frameIndex;
	uint32_t threadId;
	vk::RenderPass* renderPass;
	vk::RenderPassBeginInfo* renderPassBeginInfo;
	vk::Pipeline* pipeline;
	vk::Buffer* vertexBuffer;
	vk::Buffer* indexBuffer;
	vk::Framebuffer* framebuffer;
	uint64_t dynamicUniformBufferStride;
};

static void SaveToFile(const std::string& file, const std::string& data)
{
	std::ofstream fs;
	fs.open(file, std::ofstream::app);
	fs << data;
	fs.close();
}

static std::vector<char> readFile(const std::string& filename) {

	//ate: read from the end of file
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file " + filename);
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	return buffer;
}

static uint32_t findMemoryType(vk::PhysicalDevice device, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
	auto memProperties = device.getMemoryProperties();

	for (auto i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && memProperties.memoryTypes[i].propertyFlags & properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

#ifdef _DEBUG
#define GET_INSTANCE_PROCEDURE(name) (PFN_##name)vkGetInstanceProcAddr(instance, #name);

static VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	static auto func = GET_INSTANCE_PROCEDURE(vkCreateDebugReportCallbackEXT);

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator = nullptr) {
	static auto func = GET_INSTANCE_PROCEDURE(vkDestroyDebugReportCallbackEXT);
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

#undef GET_INSTANCE_PROCEDURE
#endif