#include "VulkanApplication.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION

#include <windows.h>	// For Beeps included early for not redifining max

#include <algorithm>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <set>
#include <stb/stb_image.h>
#include <thread>		// For non blocking beeps
#include <vulkan/vulkan.h>

#include "SwapChainSupportDetails.h"
#include "QueueFamilyIndices.h"
#include "Vertex.h"
#include "Shader.h"
#include "Image.h"

const std::vector<const char*> VulkanApplication::s_DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#define TEST_USE_CUBE
#include "VertexCube.h"
#include "IndexCube.h"
#include "VertexSkull.h"
#include "IndexSkull.h"

VulkanApplication::VulkanApplication(Scene scene, Window& win)
	: m_Window(win), 
	  m_Scene(scene),
	  m_Instance()
{
}

void VulkanApplication::run()
{
	auto threadCount = TestConfiguration::GetInstance().drawThreadCount;
	m_ThreadPool = new ThreadPool(threadCount);
	m_QueryResults.resize(threadCount);

	initVulkan();
#ifdef _DEBUG
	updateUniformBuffer();
	updateDynamicUniformBuffer(0);
	updateDynamicUniformBuffer(1);
	updateDynamicUniformBuffer(2);

	glm::mat4 model_view = m_UniformBufferObject.view * m_InstanceUniformBufferObject.model[0];
	glm::vec3 model_space = {0.0f, 0.0f, 0.0f};
	glm::vec3 world_space = model_view * glm::vec4(model_space, 1.0f);
	glm::vec3 camera_space = m_UniformBufferObject.projection * model_view * glm::vec4(model_space, 1.0f);

	std::cout << "Model space:  " << model_space << std::endl;
	std::cout << "World space:  " << world_space << std::endl;
	std::cout << "Camera space: " << camera_space << std::endl;
#endif
	mainLoop();
	cleanup();
}

void VulkanApplication::createVertexBuffer()
{
	auto buffer_size = sizeof(Vertex)*s_Vertices.size();
	vk::BufferCreateInfo buffer_create_info = {};
	buffer_create_info.size = buffer_size;
	buffer_create_info.usage = vk::BufferUsageFlagBits::eTransferSrc;

	auto buffer = Buffer(m_PhysicalDevice, m_LogicalDevice, buffer_create_info, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	
	memcpy(buffer.map(), s_Vertices.data(), buffer_size);
	buffer.unmap();

	buffer_create_info.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;

	m_VertexBuffer = std::make_unique<Buffer>(m_PhysicalDevice, m_LogicalDevice, buffer_create_info, vk::MemoryPropertyFlagBits::eDeviceLocal);

	copyBuffer(buffer.m_Buffer, m_VertexBuffer->m_Buffer, buffer_size);
}

void VulkanApplication::createIndexBuffer()
{
	auto buffer_size = sizeof s_Indices[0]*s_Indices.size();
	vk::BufferCreateInfo buffer_create_info = {};
	buffer_create_info.size = buffer_size;
	buffer_create_info.usage = vk::BufferUsageFlagBits::eTransferSrc;

	auto buffer = Buffer(m_PhysicalDevice, m_LogicalDevice, buffer_create_info, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	memcpy(buffer.map(), s_Indices.data(), buffer_size);
	buffer.unmap();

	buffer_create_info.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;

	m_IndexBuffer =  std::make_unique<Buffer>(m_PhysicalDevice, m_LogicalDevice, buffer_create_info, vk::MemoryPropertyFlagBits::eDeviceLocal);

	copyBuffer(buffer.m_Buffer, m_IndexBuffer->m_Buffer, buffer_size);
}

void VulkanApplication::createDescriptorSetLayout()
{
	/*
	 * Layout bindings
	 * 0 : uniform buffer object layout
	 * 1 : dynamic uniform buffer object layout
	 * 2 : sampler layout
	 */
	std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {};

	bindings[0].binding = 0;
	bindings[0].descriptorType = vk::DescriptorType::eUniformBuffer;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = vk::ShaderStageFlagBits::eVertex;

	bindings[1].binding = 1;
	bindings[1].descriptorType = vk::DescriptorType::eUniformBufferDynamic;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = vk::ShaderStageFlagBits::eVertex;

	bindings[2].binding = 2;
	bindings[2].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();

	m_DescriptorSetLayout = m_LogicalDevice.createDescriptorSetLayout(layoutInfo);
}

void VulkanApplication::createUniformBuffer()
{
	vk::PhysicalDeviceProperties properties = m_PhysicalDevice.getProperties();

	auto buffer_size = sizeof(m_UniformBufferObject);
	vk::BufferCreateInfo buffer_create_info;
	buffer_create_info.size = buffer_size;
	buffer_create_info.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	m_UniformBuffer = std::make_unique<Buffer>(m_PhysicalDevice, m_LogicalDevice, buffer_create_info, vk::MemoryPropertyFlagBits::eHostVisible|vk::MemoryPropertyFlagBits::eHostCoherent);

	auto allignment = properties.limits.minUniformBufferOffsetAlignment;
	m_DynamicAllignment = sizeof(*m_InstanceUniformBufferObject.model);

	if(allignment > 0)
	{
		m_DynamicAllignment = (m_DynamicAllignment + allignment - 1) & ~(allignment - 1);
	}

	buffer_size = m_Scene.renderObjects().size() * m_DynamicAllignment;
	m_InstanceUniformBufferObject.model = static_cast<glm::mat4 *>(_aligned_malloc(buffer_size, m_DynamicAllignment));
	buffer_create_info.size = buffer_size;
	// Because no HOST_COHERENT flag we must flush the buffer when writing to it

	for (auto i = 0; i < m_SwapChainImages.size(); ++i) {
		m_DynamicUniformBuffer.push_back(std::make_unique<Buffer>(m_PhysicalDevice, m_LogicalDevice, buffer_create_info, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
	}
}

void VulkanApplication::createDescriptorPool()
{
	std::array<vk::DescriptorPoolSize, 3> pool_sizes;
	pool_sizes[0].type = vk::DescriptorType::eUniformBuffer;
	pool_sizes[0].descriptorCount = 1;
	pool_sizes[1].type = vk::DescriptorType::eUniformBufferDynamic;
	pool_sizes[1].descriptorCount = 1;
	pool_sizes[2].type = vk::DescriptorType::eCombinedImageSampler;
	pool_sizes[2].descriptorCount = 1;

	vk::DescriptorPoolCreateInfo pool_info = {};
	pool_info.poolSizeCount = pool_sizes.size();
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = 1; 

	m_DescriptorPool = m_LogicalDevice.createDescriptorPool(pool_info);
}

void VulkanApplication::createDescriptorSet()
{
	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_DescriptorSetLayout;

	m_DescriptorSet = m_LogicalDevice.allocateDescriptorSets(allocInfo)[0];

	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = m_UniformBuffer->m_Buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = m_UniformBuffer->size();

	vk::DescriptorBufferInfo dynamicBufferInfo;
	dynamicBufferInfo.buffer = m_DynamicUniformBuffer[0]->m_Buffer;
	dynamicBufferInfo.offset = 0;
	dynamicBufferInfo.range = m_DynamicAllignment;

	vk::DescriptorImageInfo image_info;
	image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	image_info.imageView = m_TextureImage->m_ImageView;
	image_info.sampler = m_TextureSampler;

	std::array<vk::WriteDescriptorSet, 3> descriptorWrites = {
		vk::WriteDescriptorSet(m_DescriptorSet, 0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setPBufferInfo(&bufferInfo),
		vk::WriteDescriptorSet(m_DescriptorSet, 1)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
			.setPBufferInfo(&dynamicBufferInfo),
		vk::WriteDescriptorSet(m_DescriptorSet, 2)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setPImageInfo(&image_info)
	};

	m_LogicalDevice.updateDescriptorSets(descriptorWrites, {});
}

vk::ImageView VulkanApplication::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags) const
{
	vk::ImageViewCreateInfo view_info = {};
	view_info.image = image;
	view_info.viewType = vk::ImageViewType::e2D;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	vk::ImageView image_view = m_LogicalDevice.createImageView(view_info);

	return image_view;
}

void VulkanApplication::createTextureSampler()
{
	vk::SamplerCreateInfo sampler_create_info = {};
	sampler_create_info.magFilter = vk::Filter::eLinear;
	sampler_create_info.minFilter = vk::Filter::eLinear;
	sampler_create_info.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampler_create_info.addressModeV = vk::SamplerAddressMode::eRepeat;
	sampler_create_info.addressModeW = vk::SamplerAddressMode::eRepeat;
	sampler_create_info.anisotropyEnable = VK_TRUE;
	sampler_create_info.maxAnisotropy = 16;
	sampler_create_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
	sampler_create_info.unnormalizedCoordinates = VK_FALSE;
	sampler_create_info.compareEnable = VK_FALSE;
	sampler_create_info.compareOp = vk::CompareOp::eAlways;
	sampler_create_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
	sampler_create_info.mipLodBias = 0.0f;
	sampler_create_info.minLod = 0.0f;
	sampler_create_info.maxLod = 0.0f;

	m_TextureSampler = m_LogicalDevice.createSampler(sampler_create_info);
}

vk::Format VulkanApplication::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const
{
	for(auto& format : candidates)
	{
		auto properties = m_PhysicalDevice.getFormatProperties(format);

		if ((tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features)  == features) || 
			(tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features)) {
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

vk::Format VulkanApplication::findDepthFormat() const
{
	return findSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
}

inline bool hasStencilComponent(vk::Format format)
{
	return format == vk::Format::eD32SfloatS8Uint|| format == vk::Format::eD24UnormS8Uint;
}

void VulkanApplication::createDepthResources()
{
	auto depth_format = findDepthFormat();

	vk::ImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.imageType = vk::ImageType::e2D;
	imageCreateInfo.extent.width = m_SwapChainExtent.width;
	imageCreateInfo.extent.height = m_SwapChainExtent.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = depth_format;
	imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
	imageCreateInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	imageCreateInfo.samples = vk::SampleCountFlagBits::e1;

	m_DepthImage = std::make_unique<Image>(m_LogicalDevice, m_PhysicalDevice, imageCreateInfo, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth);

	transitionImageLayout(m_DepthImage->m_Image, m_DepthImage->m_Format, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
}

void VulkanApplication::createQueryPool()
{
	vk::QueryPoolCreateInfo query_pool_create_info;
	query_pool_create_info
		.setQueryType(vk::QueryType::ePipelineStatistics)
		.setQueryCount(TestConfiguration::GetInstance().drawThreadCount)
		.setPipelineStatistics(
			vk::QueryPipelineStatisticFlagBits::eClippingInvocations |
			vk::QueryPipelineStatisticFlagBits::eClippingPrimitives |
			vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations |
			vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations |
			vk::QueryPipelineStatisticFlagBits::eGeometryShaderInvocations |
			vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives |
			vk::QueryPipelineStatisticFlagBits::eTessellationControlShaderPatches |
			vk::QueryPipelineStatisticFlagBits::eGeometryShaderPrimitives |
			vk::QueryPipelineStatisticFlagBits::eInputAssemblyVertices |
			vk::QueryPipelineStatisticFlagBits::eTessellationEvaluationShaderInvocations |
			vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations);

	// Throws exception on fail
	m_QueryPool = m_LogicalDevice.createQueryPool(query_pool_create_info);
}

// Initializes Vulkan
void VulkanApplication::initVulkan() {
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createDepthResources();
	createFramebuffers();
	createTextureImage();
	createTextureSampler();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffer();
	createDescriptorPool();
	createDescriptorSet();
	createQueryPool();
	createCommandBuffer();
	createSemaphores();
}

void VulkanApplication::createSemaphores() {
	vk::SemaphoreCreateInfo semaphoreInfo = {};

	m_ImageAvaliableSemaphore = m_LogicalDevice.createSemaphore(semaphoreInfo);
	m_RenderFinishedSemaphore = m_LogicalDevice.createSemaphore(semaphoreInfo);
}

 void VulkanApplication::createCommandBuffer() {
	 auto threadCount = TestConfiguration::GetInstance().drawThreadCount;
	 auto frameBufferCount = m_SwapChainFramebuffers.size();
	 //one command buffer per frame buffer per thread:
	m_DrawCommandBuffers.resize(frameBufferCount * threadCount);
	m_StartCommandBuffers.resize(frameBufferCount);
	//allocate room for buffers in command pool:

	vk::CommandBufferAllocateInfo allocInfo = {};
	allocInfo.level = vk::CommandBufferLevel::eSecondary; //buffers can be primary (called to by user) or secondary (called to by primary buffer)
	allocInfo.commandBufferCount = 1;
	
	for(int i = 0; i < threadCount * frameBufferCount; ++i)
	{
		allocInfo.commandPool = m_CommandPool[i];

		auto cmdBufVec = m_LogicalDevice.allocateCommandBuffers(allocInfo);

		m_DrawCommandBuffers[i] = cmdBufVec[0];
	}

	vk::CommandBufferAllocateInfo startAllocInfo = {};
	startAllocInfo.commandPool = m_StartCommandPool;
	startAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	startAllocInfo.commandBufferCount = m_SwapChainFramebuffers.size();

	m_StartCommandBuffers = m_LogicalDevice.allocateCommandBuffers(startAllocInfo);

	for (auto i = 0; i < m_SwapChainFramebuffers.size(); ++i) {
		recordCommandBuffers(i);
	}
}

 void VulkanApplication::recordCommandBuffers(uint32_t frameIndex) {

	 //starting render pass:
	 vk::RenderPassBeginInfo startRenderPassInfo = {};
	 startRenderPassInfo.renderPass = m_RenderPass;
	 startRenderPassInfo.framebuffer = m_SwapChainFramebuffers[frameIndex];

	 startRenderPassInfo.renderArea.offset = vk::Offset2D{ 0,0 };
	 startRenderPassInfo.renderArea.extent = m_SwapChainExtent;

	 std::array<vk::ClearValue, 2> start_clear_values = {};
	 start_clear_values[0].color = vk::ClearColorValue(std::array<float, 4>{1.0f, 0.0f, 0.0f, 1.0f});
	 start_clear_values[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

	 startRenderPassInfo.clearValueCount = start_clear_values.size();
	 startRenderPassInfo.pClearValues = start_clear_values.data();	

	 vk::RenderPassBeginInfo drawRenderPassInfo = {};
	 drawRenderPassInfo.renderPass = m_RenderPass;
	 drawRenderPassInfo.framebuffer = m_SwapChainFramebuffers[frameIndex];
	 drawRenderPassInfo.renderArea.offset = vk::Offset2D{ 0,0 };
	 drawRenderPassInfo.renderArea.extent = m_SwapChainExtent;
	 
	 std::array<vk::ClearValue, 2> draw_clear_values = {};
	 draw_clear_values[0].color = vk::ClearColorValue(std::array<float, 4>{0.3f, 0.3f, 0.3f, 1.0f});
	 draw_clear_values[1].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

	 drawRenderPassInfo.clearValueCount = draw_clear_values.size();
	 drawRenderPassInfo.pClearValues = draw_clear_values.data();

	 vk::CommandBufferBeginInfo beginInfo = {};
	 beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

	 /***********************************************************************************/
	 //Thread recording of draw commands:
	 auto threadCount = TestConfiguration::GetInstance().drawThreadCount;
	 std::vector<std::future<void>> futures;
	 for (auto i = 0; i < threadCount; ++i) {
		 DrawRenderObjectsInfo drawROInfo = {};

		 auto& command_buffer = m_DrawCommandBuffers[frameIndex * threadCount + i];
		 auto roCount = m_Scene.renderObjects().size() / threadCount;
		 auto stride = roCount;
		 drawROInfo.roArrStride = stride;

		 //last thread handles the remainder after integer division
		 if (i == threadCount - 1) {
			 roCount += m_Scene.renderObjects().size() % threadCount;
		 }

		 drawROInfo.commandBuffer = &command_buffer;
		 drawROInfo.descriptorSet = &m_DescriptorSet;
		 drawROInfo.dynamicAllignment = m_DynamicAllignment;
		 drawROInfo.numOfIndices = s_Indices.size();
		 drawROInfo.pipelineLayout = &m_PipelineLayout;
		 drawROInfo.roArr = &m_Scene.renderObjects()[i * stride];
		 drawROInfo.roArrCount = roCount;
		 drawROInfo.frameIndex = frameIndex;
		 drawROInfo.queryPool = &m_QueryPool;
		 drawROInfo.threadId = i;
		 drawROInfo.renderPass = &m_RenderPass;
		 drawROInfo.renderPassBeginInfo = &drawRenderPassInfo;
		 drawROInfo.pipeline = &m_GraphicsPipeline;
		 drawROInfo.vertexBuffer = &m_VertexBuffer->m_Buffer;
		 drawROInfo.indexBuffer = &m_IndexBuffer->m_Buffer;
		 drawROInfo.framebuffer = &m_SwapChainFramebuffers[frameIndex];
		 drawROInfo.dynamicUniformBufferStride = m_DynamicAllignment * m_Scene.renderObjects().size();

		 futures.push_back(m_ThreadPool->enqueue(DrawRenderObjects, drawROInfo));
	 }
	 /***********************************************************************************/

	 //record setup:	 
	 auto& startCommandBuffer = m_StartCommandBuffers[frameIndex];
	 startCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources  );
	 startCommandBuffer.begin(beginInfo);
	 startCommandBuffer.beginRenderPass(startRenderPassInfo, vk::SubpassContents::eInline);
	 
	 startCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_GraphicsPipeline);
	 
	 startCommandBuffer.bindVertexBuffers(
		 0,								// index of first buffer
		 { m_VertexBuffer->m_Buffer },	// Array of buffers
		 { 0 });							// Array of offsets into the buffers
	 startCommandBuffer.bindIndexBuffer(m_IndexBuffer->m_Buffer, 0, vk::IndexType::eUint16);
	 
	 startCommandBuffer.endRenderPass();
	 startCommandBuffer.beginRenderPass(drawRenderPassInfo, vk::SubpassContents::eSecondaryCommandBuffers);
	 
	 //wait for all recordings to finish
	 for (auto& future : futures) {
		 future.wait();
	 }

	 startCommandBuffer.executeCommands(threadCount, &m_DrawCommandBuffers[frameIndex * threadCount]);

	 startCommandBuffer.endRenderPass();
	 startCommandBuffer.end();
 }

 void VulkanApplication::DrawRenderObjects(DrawRenderObjectsInfo& info)
 {
	 vk::CommandBufferInheritanceInfo inheritInfo = {};
	 inheritInfo.framebuffer = *info.framebuffer;
	 inheritInfo.occlusionQueryEnable = VK_FALSE;
	 inheritInfo.pipelineStatistics =
		 vk::QueryPipelineStatisticFlagBits::eClippingInvocations |
		 vk::QueryPipelineStatisticFlagBits::eClippingPrimitives |
		 vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations |
		 vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations |
		 vk::QueryPipelineStatisticFlagBits::eGeometryShaderInvocations |
		 vk::QueryPipelineStatisticFlagBits::eGeometryShaderPrimitives |
		 vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives |
		 vk::QueryPipelineStatisticFlagBits::eInputAssemblyVertices |
		 vk::QueryPipelineStatisticFlagBits::eTessellationControlShaderPatches |
		 vk::QueryPipelineStatisticFlagBits::eTessellationEvaluationShaderInvocations |
		 vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations;
	 inheritInfo.renderPass = *info.renderPass;

	 vk::CommandBufferBeginInfo beginInfo = {};
	 beginInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eSimultaneousUse;
	 beginInfo.pInheritanceInfo = &inheritInfo;

	 info.commandBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	 info.commandBuffer->begin(beginInfo);

	 info.commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *info.pipeline);
	 
	 info.commandBuffer->bindVertexBuffers(
		 0,								// index of first buffer
		 { *info.vertexBuffer },	// Array of buffers
		 { 0 });							// Array of offsets into the buffers
	 
	 info.commandBuffer->bindIndexBuffer(*info.indexBuffer, 0, vk::IndexType::eUint16);

	 if (TestConfiguration::GetInstance().pipelineStatistics) {

		info.commandBuffer->beginQuery(*info.queryPool, info.threadId, vk::QueryControlFlags());
	 }

	 for (int j = 0; j < info.roArrCount; ++j) {
		 uint32_t dynamic_offset = info.threadId * info.roArrStride * info.dynamicAllignment + j * info.dynamicAllignment;
		 info.commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *info.pipelineLayout, 0, { *info.descriptorSet }, { dynamic_offset });
		 info.commandBuffer->drawIndexed(info.numOfIndices, 1, 0, 0, 0);
	 }

	 if (TestConfiguration::GetInstance().pipelineStatistics) {
		 info.commandBuffer->endQuery(*info.queryPool, info.threadId);
	 }
	
	 info.commandBuffer->end();
 }

 void VulkanApplication::createCommandPool() {
	auto frameBufferCount = m_SwapChainImages.size();
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_PhysicalDevice);

	vk::CommandPoolCreateInfo poolInfo = {};
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

	if (TestConfiguration::GetInstance().reuseCommandBuffers) {
		poolInfo.flags = vk::CommandPoolCreateFlags(); //optional about rerecording strategy of cmd buffer(s)
	}
	else {
		poolInfo.flags =vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	}

	auto bufferCount = TestConfiguration::GetInstance().drawThreadCount * frameBufferCount;
	for (auto i = 0; i < bufferCount; ++i) {
		m_CommandPool.push_back(m_LogicalDevice.createCommandPool(poolInfo));
	}

	m_StartCommandPool = m_LogicalDevice.createCommandPool(poolInfo);

	poolInfo.flags = vk::CommandPoolCreateFlags();
	m_SingleTimeCommandPool = m_LogicalDevice.createCommandPool(poolInfo);
}

 void VulkanApplication::createFramebuffers() {
	m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
		std::array<vk::ImageView, 2> attachments = {
			m_SwapChainImageViews[i],
			m_DepthImage->m_ImageView
		};

		vk::FramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.renderPass = m_RenderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_SwapChainExtent.width;
		framebufferInfo.height = m_SwapChainExtent.height;
		framebufferInfo.layers = 1;

		m_SwapChainFramebuffers[i] = m_LogicalDevice.createFramebuffer(framebufferInfo);


	}
}

 void VulkanApplication::createRenderPass() {

	// Color buffer resides as swapchain image. 
	vk::AttachmentDescription colorAttatchment;
	colorAttatchment.setFormat(m_SwapChainImageFormat)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStencilLoadOp(vk::AttachmentLoadOp::eLoad)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	vk::AttachmentReference colorAttatchmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);


	// Depth buffer resides as swapchain image. 
	vk::AttachmentDescription depthAttatchment;
	depthAttatchment.setFormat(findDepthFormat())
		.setLoadOp(vk::AttachmentLoadOp::eClear) // Clear buffer data at load
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::AttachmentReference depthAttatchmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDescription subpass = {};
	subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&colorAttatchmentRef)
		.setPDepthStencilAttachment(&depthAttatchmentRef);

	// Handling subpass dependencies
	vk::SubpassDependency dependency(VK_SUBPASS_EXTERNAL);
	dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setSrcAccessMask(vk::AccessFlags())
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

	std::array<vk::AttachmentDescription, 2> attachments = { colorAttatchment, depthAttatchment };

	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.setAttachmentCount(attachments.size())
		.setPAttachments(attachments.data())
		.setSubpassCount(1)
		.setPSubpasses(&subpass)
		.setDependencyCount(1)
		.setPDependencies(&dependency);

	m_RenderPass = m_LogicalDevice.createRenderPass(renderPassInfo);
}

 void VulkanApplication::createGraphicsPipeline() {

	//get byte code of shaders
	auto vertShader = Shader(m_LogicalDevice, "./shaders/vert.spv", vk::ShaderStageFlagBits::eVertex);


#ifdef TEST_USE_CUBE
	auto fragShader = Shader(m_LogicalDevice, "./shaders/frag.spv", vk::ShaderStageFlagBits::eFragment);
#endif
#ifdef TEST_USE_SKULL
	auto fragShader = Shader(m_LogicalDevice, "./shaders/skull.spv", vk::ShaderStageFlagBits::eFragment);
#endif

	//for later reference:
	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShader.m_Info, fragShader.m_Info };

	auto bindingDescription = Vertex::getBindingDescription();
	auto attribute_descriptions = Vertex::getAttributeDescriptions();

	// Information on how to read from vertex buffer
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.setVertexBindingDescriptionCount(1)
		.setPVertexBindingDescriptions(&bindingDescription)
		.setVertexAttributeDescriptionCount(attribute_descriptions.size())
		.setPVertexAttributeDescriptions(attribute_descriptions.data());

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);

	// Creating a viewport to render to
	vk::Viewport viewport(0.0f, 0.0f, m_SwapChainExtent.width, m_SwapChainExtent.height, 0.0f, 1.0f);

	// Scissor rectangle. Defines image cropping of viewport.
	vk::Rect2D scissor({ 0, 0 }, m_SwapChainExtent);

	// Combine viewport and scissor into a viewport state
	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.setViewportCount(1)
		.setPViewports(&viewport)
		.setScissorCount(1)
		.setPScissors(&scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.setPolygonMode(vk::PolygonMode::eFill)
		.setLineWidth(1.0f)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eCounterClockwise);

	// Multisampling. Not in use but works to do anti aliasing
	vk::PipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setMinSampleShading(1.0f); //optional

	//Color blending
	vk::PipelineColorBlendAttachmentState colorBlendAttatchment;
	colorBlendAttatchment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		.setSrcColorBlendFactor(vk::BlendFactor::eOne)  // optional
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne); // optional

	//Global Color Blending
	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.setLogicOp(vk::LogicOp::eCopy)  // optional
		.setAttachmentCount(1)
		.setPAttachments(&colorBlendAttatchment);

	 // For uniforms
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setSetLayoutCount(1)
		.setPSetLayouts(&m_DescriptorSetLayout);

	m_PipelineLayout = m_LogicalDevice.createPipelineLayout(pipelineLayoutInfo);

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_info;
	depth_stencil_info.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setMaxDepthBounds(1.0f);

	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.setStageCount(2)
		.setPStages(shaderStages)
		.setPVertexInputState(&vertexInputInfo)
		.setPInputAssemblyState(&inputAssembly)
		.setPViewportState(&viewportState)
		.setPRasterizationState(&rasterizer)
		.setPMultisampleState(&multisampling)
		.setPDepthStencilState(&depth_stencil_info)
		.setPColorBlendState(&colorBlending)
		.setLayout(m_PipelineLayout)
		.setRenderPass(m_RenderPass);

	m_GraphicsPipeline = m_LogicalDevice.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);
}

 void VulkanApplication::createImageViews() {
	m_SwapChainImageViews.resize(m_SwapChainImages.size());

	for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
		m_SwapChainImageViews[i] = createImageView(m_SwapChainImages[i], m_SwapChainImageFormat);
	}
}

//  Creates and sets the swapchain + sets "swapChainImageFormat" and "swapChainExtent".
 void VulkanApplication::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_PhysicalDevice);

	vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentmodes);
	m_SwapChainExtent = chooseSwapExtend(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo = {};
	createInfo.surface = m_Surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = m_SwapChainExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);
	uint32_t queueFamilyIndices[] = {
		indices.graphicsFamily, indices.presentFamily
	};

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0; //optional
		createInfo.pQueueFamilyIndices = nullptr; //optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	m_SwapChain = m_LogicalDevice.createSwapchainKHR(createInfo);


	m_SwapChainImages = m_LogicalDevice.getSwapchainImagesKHR(m_SwapChain);

	
	m_SwapChainImageFormat = surfaceFormat.format;
}

 void VulkanApplication::createSurface() {
	 VkWin32SurfaceCreateInfoKHR surface_info = {};
	 surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	 surface_info.hwnd = m_Window.GetHandle();
	 surface_info.hinstance = GetModuleHandle(nullptr);

	 auto CreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(m_Instance->getProcAddr("vkCreateWin32SurfaceKHR"));

	 if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(static_cast<VkInstance>(*m_Instance), &surface_info, nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_Surface)) != VK_SUCCESS) {
		 throw std::runtime_error("failed to create window surface!");
	 }
}

 void VulkanApplication::createLogicalDevice() {
	auto indices = findQueueFamilies(m_PhysicalDevice);

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };
	float queuePriorty = 1.0f;

	// Runs over each family and makes a createinfo object for them. Only one
	// queue is created if the physical device dictates that the present and 
	// graphics queue is one and
	for (int queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriorty; // Priority required even with 1 queue
		queueCreateInfos.push_back(queueCreateInfo);
	}


	vk::PhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.pipelineStatisticsQuery = VK_TRUE;

												  // Creating VkDeviceCreateInfo object
	vk::DeviceCreateInfo createInfo = {};
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = s_DeviceExtensions.size();
	createInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();
	createInfo.enabledLayerCount = 0;

	m_LogicalDevice = m_PhysicalDevice.createDevice(createInfo);

	// Get handle to queue in the logicalDevice
	m_GraphicsQueue = m_LogicalDevice.getQueue(indices.graphicsFamily, 0);
	m_PresentQueue = m_LogicalDevice.getQueue(indices.presentFamily, 0);
}

 void VulkanApplication::pickPhysicalDevice() {	
	for (auto& device : m_Instance->enumeratePhysicalDevices()) {
		if(isDeviceSuitable(device)){
			m_PhysicalDevice = device;
			break;
		}
	}

	if (!m_PhysicalDevice) {
		throw std::runtime_error("failed to find suitable GPU!");
	}
}

 QueueFamilyIndices VulkanApplication::findQueueFamilies(const vk::PhysicalDevice& device) const
 {
	QueueFamilyIndices indices;


	std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
	
	for (int i = 0; i < queueFamilies.size(); i++) {
		auto queueFamily = queueFamilies.at(i);

		//check for graphics family
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		//check for present family
		if (queueFamily.queueCount > 0 && device.getSurfaceSupportKHR(i, m_Surface)) {
			indices.presentFamily = i;
		}


		if (indices.isComplete()) {
			break;
		}
	}

	return indices;
}

 bool VulkanApplication::isDeviceSuitable(const vk::PhysicalDevice& device) const
 {

	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentmodes.empty();
	}

	vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy && supportedFeatures.pipelineStatisticsQuery;
}

 bool VulkanApplication::checkDeviceExtensionSupport(const vk::PhysicalDevice& device)
 {

	std::vector<vk::ExtensionProperties> avaliableExtensions = device.enumerateDeviceExtensionProperties();
	

	std::set<std::string> requiredExtensions(s_DeviceExtensions.begin(), s_DeviceExtensions.end());

	for (const auto& extension : avaliableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

std::vector<const char*> getRequiredExtensions()
{
	std::vector<const char*> extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef _DEBUG
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
	};

	return extensions;
}

 SwapChainSupportDetails VulkanApplication::querySwapChainSupport(const vk::PhysicalDevice& device) const
 {
	SwapChainSupportDetails details;
	details.capabilities = device.getSurfaceCapabilitiesKHR(m_Surface);

	details.formats = device.getSurfaceFormatsKHR(m_Surface);
	
	details.presentmodes = device.getSurfacePresentModesKHR(m_Surface);

	return details;
}

 vk::SurfaceFormatKHR VulkanApplication::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {

	//In the case the surface has no preference
	if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
		return{ vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
	}

	// If we're not allowed to freely choose a format  
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
			availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

 vk::PresentModeKHR VulkanApplication::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
	vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
			return availablePresentMode;
		}
		if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

 vk::Extent2D VulkanApplication::chooseSwapExtend(const vk::SurfaceCapabilitiesKHR & capabilities) const
 {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	vk::Extent2D actualExtent = { m_Window.width(), m_Window.height() };
	
	actualExtent.width = std::max(
		 capabilities.minImageExtent.width,
		 std::min(capabilities.maxImageExtent.width, actualExtent.width)
	);

	actualExtent.height = std::max(
		capabilities.minImageExtent.height,
		std::min(capabilities.maxImageExtent.height, actualExtent.height)
	);

	 return actualExtent;
}

glm::vec3 convertToGLM(const Vec4f& vec)
{
	return { vec.x, vec.y, vec.z };
}

void VulkanApplication::updateUniformBuffer()
{
	m_UniformBufferObject.view = lookAt(
		convertToGLM(m_Scene.camera().Position()),
		convertToGLM(m_Scene.camera().Target()),
		convertToGLM(m_Scene.camera().Up()));
	m_UniformBufferObject.projection = glm::perspective(
		m_Scene.camera().FieldOfView(),
		m_Window.aspectRatio(),
		m_Scene.camera().Near(),
		m_Scene.camera().Far());
	m_UniformBufferObject.projection[1][1] *= -1; // flip up and down

	memcpy(m_UniformBuffer->map(), &m_UniformBufferObject, sizeof(m_UniformBufferObject));
	m_UniformBuffer->unmap();
}

void VulkanApplication::updateDynamicUniformBuffer(int frameIndex) const
{
	for (auto index = 0; index < m_Scene.renderObjects().size(); index++)
	{
		auto& render_object = m_Scene.renderObjects()[index];
		auto model = reinterpret_cast<glm::mat4*>(reinterpret_cast<uint64_t>(m_InstanceUniformBufferObject.model) + (index * m_DynamicAllignment));
		*model = translate(glm::mat4(), { render_object.x(), render_object.y(), render_object.z() });

		//hack around the const to update m_RotationAngle. //TODO: remove rotation feature or const from m_Scene.renderObjects()
		auto noconst =  const_cast<RenderObject*>(&render_object);
		noconst->m_RotationAngle = (render_object.m_RotationAngle + 1) % 360;

		if (TestConfiguration::GetInstance().rotateCubes) {
			auto rotateX = 0.0001f*(index + 1) * std::pow(-1, index);
			auto rotateY = 0.0002f*(index + 1) * std::pow(-1, index);
			auto rotateZ = 0.0003f*(index + 1) * std::pow(-1, index);
			*model = glm::rotate<float>(*model, render_object.m_RotationAngle * 3.14159268 / 180, glm::tvec3<float>{ rotateX, rotateY, rotateZ });
		}
	}

	memcpy(m_DynamicUniformBuffer[frameIndex]->map(), m_InstanceUniformBufferObject.model, m_DynamicAllignment * m_Scene.renderObjects().size());
	m_DynamicUniformBuffer[frameIndex]->unmap();

	vk::DescriptorBufferInfo dynamicBufferInfo;
	dynamicBufferInfo.buffer = m_DynamicUniformBuffer[frameIndex]->m_Buffer;
	dynamicBufferInfo.offset = 0;
	dynamicBufferInfo.range = m_DynamicAllignment;

	std::array<vk::WriteDescriptorSet, 1> writes = {
		vk::WriteDescriptorSet(m_DescriptorSet, 1)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic)
		.setPBufferInfo(&dynamicBufferInfo)
	};

	m_LogicalDevice.updateDescriptorSets(writes, {});
}

void VulkanApplication::mainLoop() {

	using Clock = std::chrono::high_resolution_clock;

	size_t nanoSec = 0;
	size_t probeCount = 0;
	auto lastUpdate = Clock::now();

	WMIAccessor wmiAccesor;
	_bstr_t probeProperties[] = { "Identifier", "Value", "SensorType" };

	auto& testConfig = TestConfiguration::GetInstance();

	if (testConfig.openHardwareMonitorData) {
		wmiAccesor.Connect("OpenHardwareMonitor");
	}

	std::stringstream fpsCsv;
	fpsCsv << "FPS\n";

	int fps = 0;	//<-- this one is incremented each frame (and reset once a second)
	int oldfps = 0;	//<-- this one is recorded by the probe (and set once per second)
	size_t secondTrackerInNanoSec = 0;

	auto frametimes = std::vector<size_t>(); //<-- holds frame times in ns
	std::stringstream frametimeCsv;
	frametimeCsv << "frametime(nanoseconds)\n";

	while ((nanoSec / 1000000000 < testConfig.seconds) || (testConfig.seconds == 0))
	{
		MSG message;
		if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
		{
			if (message.message == WM_QUIT) {
				break;
			}
			else if (message.message == WM_KEYDOWN) {
				//if key pressed is "r":
				if (message.wParam == 82) {
					auto& rc = TestConfiguration::GetInstance().rotateCubes;
					rc = !rc;
				}
			}

			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		else
		{
			//**************************************************
			auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - lastUpdate).count();
			nanoSec += delta;
			secondTrackerInNanoSec += delta;
			lastUpdate = Clock::now();

			//has a (multiplicia of) second(s) passed?
			if (secondTrackerInNanoSec > 1000000000) {
				auto title = "FPS: " + std::to_string(fps);
		
				m_Window.SetTitle(title.c_str());
				secondTrackerInNanoSec %= 1000000000;
				oldfps = fps;
				fps = 0;

				if (TestConfiguration::GetInstance().recordFrameTime) {
					frametimeCsv << delta << "\n";
				}

				if (TestConfiguration::GetInstance().recordFPS) {
					fpsCsv << oldfps << "\n";
				}
			}

			if (testConfig.openHardwareMonitorData &&
				nanoSec / 1000000 > probeCount * testConfig.probeInterval)
			{
				//QueryItem queries one item from WMI, which is separated into multiple items (put in a vector here)
				auto items = wmiAccesor.QueryItem("sensor", probeProperties, 3);

				//each item needs a timestamp and an id ( = probeCount) and the last completely measured FPS
				for (auto& item : items) {
					item.Timestamp = std::to_string(nanoSec);
					item.Id = std::to_string(probeCount);
					item.FPS = std::to_string(oldfps);
					wmiCollection.Add(item);
				}

				++probeCount;
			}

			//**************************************************

			updateUniformBuffer();

			drawFrame();
			++fps;
		}
	}

	m_LogicalDevice.waitIdle();

	if (testConfig.pipelineStatistics) {
		for (auto i = 0; i < testConfig.drawThreadCount; ++i) {
			auto& queryResult = m_QueryResults[i];
			PipelineStatisticsDataItem item;
			item.CInvocations = std::to_string(queryResult.clippingInvocations);
			item.CommandListId = std::to_string(i);
			item.CPrimitives = std::to_string(queryResult.clippingPrimitives);
			item.CSInvocations = std::to_string(queryResult.computeShaderInvocations);
			item.DSInvocations = "N/A";
			item.GSInvocations = std::to_string(queryResult.geometryShaderInvocations);
			item.GSPrimitives = std::to_string(queryResult.geometryShaderPrimitives);
			item.HSInvocations = "N/A";
			item.IAPrimitives = std::to_string(queryResult.inputAssemblyPrimitives);
			item.IAVertices = std::to_string(queryResult.inputAssemblyVertices);
			item.PSInvocations = std::to_string(queryResult.fragmentShaderInvocations);
			item.VSInvocations = std::to_string(queryResult.vertexShaderInvocations);

			pipelineStatisticsCollection.Add(item);
		}
	}

	//save files
	auto now = time(NULL);
	tm* localNow = new tm();
	localtime_s(localNow, &now);

	auto yearStr = std::to_string((1900 + localNow->tm_year));
	auto monthStr = localNow->tm_mon < 9 ? "0" + std::to_string(localNow->tm_mon + 1) : std::to_string(localNow->tm_mon + 1);
	auto dayStr = localNow->tm_mday < 10 ? "0" + std::to_string(localNow->tm_mday) : std::to_string(localNow->tm_mday);
	auto hourStr = localNow->tm_hour < 10 ? "0" + std::to_string(localNow->tm_hour) : std::to_string(localNow->tm_hour);
	auto minStr = localNow->tm_min < 10 ? "0" + std::to_string(localNow->tm_min) : std::to_string(localNow->tm_min);
	auto secStr = localNow->tm_sec < 10 ? "0" + std::to_string(localNow->tm_sec) : std::to_string(localNow->tm_sec);

	auto fname = yearStr + monthStr + dayStr + hourStr + minStr + secStr;

	if (testConfig.exportCsv && testConfig.openHardwareMonitorData) {

		auto csvStr = wmiCollection.MakeString(";");
		SaveToFile("data_" + fname + ".csv", csvStr);
	}

	if (testConfig.exportCsv && testConfig.pipelineStatistics) {
		auto csvStr = pipelineStatisticsCollection.MakeString(";");
		SaveToFile("stat_" + fname + ".csv", csvStr);
	}

	if (testConfig.exportCsv) {
		auto csvStr = testConfig.MakeString(";");
		SaveToFile("conf_" + fname + ".csv", csvStr);
	}

	if (testConfig.recordFPS) {
		SaveToFile("fps_" + fname + ".csv", fpsCsv.str());
	}

	if (testConfig.recordFrameTime) {
		SaveToFile("frameTime_" + fname + ".csv", frametimeCsv.str());
	}

	delete localNow;
}

void VulkanApplication::drawFrame() {

	// Aquire image
	auto imageResult = m_LogicalDevice.acquireNextImageKHR(m_SwapChain, std::numeric_limits<uint64_t>::max(), m_ImageAvaliableSemaphore, vk::Fence());
	updateDynamicUniformBuffer(imageResult.value);
	
	if(imageResult.result != vk::Result::eSuccess && imageResult.result != vk::Result::eSuboptimalKHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image!");
	}

	if (!TestConfiguration::GetInstance().reuseCommandBuffers) {
		recordCommandBuffers(imageResult.value);
	}

	//Submitting Command Buffer
	vk::SubmitInfo submitInfo = {};

	// Wait in this stage until semaphore is aquired
	vk::Semaphore  waitSemaphores[] = { m_ImageAvaliableSemaphore };
	vk::PipelineStageFlags waitStages[] = {
		vk::PipelineStageFlagBits::eColorAttachmentOutput
	};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	auto threadCount = TestConfiguration::GetInstance().drawThreadCount;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_StartCommandBuffers[imageResult.value];

	// Specify which sempahore to signal once command buffers have been executed.
	vk::Semaphore signalSemaphores[] = { m_RenderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	m_GraphicsQueue.submit({ submitInfo }, vk::Fence());

	// Present the image presented
	vk::PresentInfoKHR presentInfo = {};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	vk::SwapchainKHR swapChains[] = { m_SwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageResult.value;

	presentInfo.pResults = nullptr; // Would contain VK result for all images if more than 1

	m_PresentQueue.presentKHR(presentInfo);
	
	//TODO: move this up to the beginning?
	m_PresentQueue.waitIdle();
	
	if (TestConfiguration::GetInstance().pipelineStatistics) {
		if(m_LogicalDevice.getQueryPoolResults(
			m_QueryPool, 
			0, 
			threadCount,
			sizeof(PipelineStatisticsResult) * threadCount, 
			m_QueryResults.data(), 
			sizeof uint64_t, 
			vk::QueryResultFlagBits::eWait | vk::QueryResultFlagBits::e64) != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed get the query results from the logical device");
		}
	}
}

void VulkanApplication::cleanup() {
	delete m_ThreadPool;
	cleanupSwapChain();

	_aligned_free(m_InstanceUniformBufferObject.model);

	m_LogicalDevice.destroySampler(m_TextureSampler);
	m_LogicalDevice.destroyDescriptorPool(m_DescriptorPool);
	m_LogicalDevice.destroyDescriptorSetLayout(m_DescriptorSetLayout);

	m_LogicalDevice.destroySemaphore(m_RenderFinishedSemaphore);
	m_LogicalDevice.destroySemaphore(m_ImageAvaliableSemaphore);

	
	for (auto& pool : m_CommandPool) {
		m_LogicalDevice.destroyCommandPool(pool);
	}

	m_LogicalDevice.destroyCommandPool(m_StartCommandPool);
	m_LogicalDevice.destroyCommandPool(m_SingleTimeCommandPool);

	m_LogicalDevice.destroyQueryPool(m_QueryPool);
	m_VertexBuffer = nullptr;
	m_IndexBuffer = nullptr;
	m_UniformBuffer = nullptr;
	m_DynamicUniformBuffer;
	for (auto& dynBuf : m_DynamicUniformBuffer) {
		dynBuf = nullptr;
	}
	m_TextureImage = nullptr;
	m_DepthImage = nullptr;
	m_LogicalDevice.destroy();
	m_Instance->destroySurfaceKHR(m_Surface);
}

void VulkanApplication::recreateSwapChain()
{
	m_LogicalDevice.waitIdle();

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFramebuffers();
	createCommandBuffer();
}

void VulkanApplication::cleanupSwapChain()
{
	m_LogicalDevice.waitIdle();
	auto framebufferCount = m_SwapChainFramebuffers.size();
	for (auto frameBuffer  : m_SwapChainFramebuffers) {
		m_LogicalDevice.destroyFramebuffer(frameBuffer);
	}

	auto threadCount = TestConfiguration::GetInstance().drawThreadCount;
	for (auto i = 0; i < threadCount * framebufferCount; ++i) {
		//TODO: free properly. see command buffer allocation for distribution on pools.
		m_LogicalDevice.freeCommandBuffers(m_CommandPool[i], m_DrawCommandBuffers[i]);
	}

	m_LogicalDevice.freeCommandBuffers(m_StartCommandPool, m_StartCommandBuffers);

	m_LogicalDevice.destroyPipeline(m_GraphicsPipeline);

	m_LogicalDevice.destroyPipelineLayout(m_PipelineLayout);

	m_LogicalDevice.destroyRenderPass(m_RenderPass);

	for (auto imageView : m_SwapChainImageViews) {
		m_LogicalDevice.destroyImageView(imageView);
	}

	m_LogicalDevice.destroySwapchainKHR(m_SwapChain);
}

void VulkanApplication::copyBuffer(vk::Buffer source, vk::Buffer destination, vk::DeviceSize size)
{
	auto commandBuffer = beginSingleTimeCommands();

	vk::BufferCopy copyRegion;
	copyRegion.srcOffset = 0; 
	copyRegion.dstOffset = 0; 
	copyRegion.size = size;

	commandBuffer.copyBuffer(source, destination, { copyRegion });

	endSingleTimeCommands(commandBuffer);
}

void VulkanApplication::createTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/texture.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		throw std::runtime_error("Failed to load texture image!");
	}
	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	vk::BufferCreateInfo buffer_create_info;
	buffer_create_info.setSize(imageSize)
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc);

	Buffer buffer(m_PhysicalDevice, m_LogicalDevice, buffer_create_info, vk::MemoryPropertyFlagBits::eHostVisible| vk::MemoryPropertyFlagBits::eHostCoherent);

	memcpy(buffer.map(), pixels, imageSize);
	buffer.unmap();

	stbi_image_free(pixels);

	vk::ImageCreateInfo imageCreateInfo;
	imageCreateInfo.setImageType(vk::ImageType::e2D)
		.setFormat(vk::Format::eR8G8B8A8Unorm)
		.setExtent({ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 })
		.setMipLevels(1)
		.setArrayLayers(1)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
		.setInitialLayout(vk::ImageLayout::eUndefined);

	m_TextureImage = std::make_unique<Image>(m_LogicalDevice, m_PhysicalDevice, imageCreateInfo, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
	transitionImageLayout(m_TextureImage->m_Image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	copyBufferToImage(buffer.m_Buffer, m_TextureImage->m_Image, texWidth, texHeight);

	//prepare to use image in shader:
	transitionImageLayout(m_TextureImage->m_Image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}

vk::CommandBuffer VulkanApplication::beginSingleTimeCommands() const
{
	vk::CommandBufferAllocateInfo allocInfo = {};
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = m_SingleTimeCommandPool;
	allocInfo.commandBufferCount = 1;

	vk::CommandBuffer commandBuffer = m_LogicalDevice.allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo = {};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer.begin(beginInfo);
	return commandBuffer;
}

void VulkanApplication::endSingleTimeCommands(vk::CommandBuffer commandBuffer)
{
	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	m_GraphicsQueue.submit({ submitInfo }, vk::Fence());
	m_GraphicsQueue.waitIdle();

	m_LogicalDevice.freeCommandBuffers(m_SingleTimeCommandPool, { commandBuffer });
}

void VulkanApplication::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
	auto commandBuffer = beginSingleTimeCommands();

	vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor;

	// Special case for depth buffer image
	if(newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		aspect_mask = vk::ImageAspectFlagBits::eDepth;
		if(hasStencilComponent(format))
		{
			aspect_mask |= vk::ImageAspectFlagBits::eStencil;
		}
	}
	
	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;
	vk::AccessFlags source_access_mask;
	vk::AccessFlags destination_access_mask;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		source_access_mask = vk::AccessFlags();
		destination_access_mask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		source_access_mask = vk::AccessFlagBits::eTransferWrite;
		destination_access_mask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		source_access_mask = vk::AccessFlags();
		destination_access_mask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	auto image_memory_barrier = vk::ImageMemoryBarrier(
		source_access_mask, 
		destination_access_mask, 
		oldLayout, 
		newLayout, 
		VK_QUEUE_FAMILY_IGNORED, 
		VK_QUEUE_FAMILY_IGNORED, 
		image, 
		{ aspect_mask, 0, 1, 0, 1 });

	commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), {}, {}, { image_memory_barrier });

	endSingleTimeCommands(commandBuffer);
}

void VulkanApplication::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
	auto commandBuffer = beginSingleTimeCommands();

	vk::BufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = vk::Offset3D { 0,0,0 };
	region.imageExtent = vk::Extent3D {
		width,
		height,
		1
	};

	commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });

	endSingleTimeCommands(commandBuffer);
}



