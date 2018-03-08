#pragma once
#include <vulkan/vulkan.hpp>

class Buffer
{
public:
	Buffer(vk::PhysicalDevice physicalDevice, vk::Device device, vk::BufferCreateInfo buffer_info, vk::MemoryPropertyFlags properties);
	~Buffer();

	void* map() const;
	void unmap() const;
	vk::DeviceSize size() const { return m_Size; }

	vk::Buffer m_Buffer;
	vk::DeviceMemory m_Memory;
private:
	vk::Device m_Device;
	vk::DeviceSize m_Size;
};