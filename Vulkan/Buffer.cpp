#include "Buffer.h"
#include "Utility.h"

Buffer::Buffer(vk::PhysicalDevice physicalDevice, vk::Device device, vk::BufferCreateInfo buffer_info, vk::MemoryPropertyFlags properties)
	: m_Buffer(device.createBuffer(buffer_info)), m_Device(device), m_Size(buffer_info.size)
{
	auto memRequirements = device.getBufferMemoryRequirements(m_Buffer);

	vk::MemoryAllocateInfo allocInfo = {};
	allocInfo
		.setAllocationSize(memRequirements.size)
		.setMemoryTypeIndex(findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties));

	m_Memory = device.allocateMemory(allocInfo);

	device.bindBufferMemory(m_Buffer, m_Memory, 0);
}

Buffer::~Buffer()
{
	m_Device.destroyBuffer(m_Buffer);
	m_Device.freeMemory(m_Memory);
}

void* Buffer::map() const
{
	return m_Device.mapMemory(m_Memory, 0, VK_WHOLE_SIZE);
}

void Buffer::unmap() const
{
	m_Device.unmapMemory(m_Memory);
}
