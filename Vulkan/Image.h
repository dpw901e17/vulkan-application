#pragma once
#include <vulkan/vulkan.hpp>
#include "Utility.h"

class Image
{
public:
	Image(vk::Device device, vk::PhysicalDevice physical_device, const vk::ImageCreateInfo& imageCreateInfo, vk::MemoryPropertyFlags properties, vk::ImageAspectFlagBits aspect_flags)
		: m_Format(imageCreateInfo.format), m_Device(device)
	{
		m_Image = device.createImage(imageCreateInfo);

		//copy to buffer
		VkMemoryRequirements memRequirements = device.getImageMemoryRequirements(m_Image);

		vk::MemoryAllocateInfo allocInfo = {};
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physical_device, memRequirements.memoryTypeBits, properties);

		
		m_Memory = device.allocateMemory(allocInfo);

		device.bindImageMemory(m_Image, m_Memory, 0);

		vk::ImageSubresourceRange subresourceRange;
		subresourceRange.setAspectMask(aspect_flags)
			.setLevelCount(1)
			.setLayerCount(1);

		vk::ImageViewCreateInfo view_info;
		view_info.setImage(m_Image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(m_Format)
			.setSubresourceRange(subresourceRange);

		m_ImageView = device.createImageView(view_info);
	}

	~Image()
    {

		m_Device.destroyImageView(m_ImageView);
		m_Device.destroyImage(m_Image);
		m_Device.freeMemory(m_Memory);
    }

	vk::Image m_Image;
	vk::DeviceMemory m_Memory;
	vk::ImageView m_ImageView;
	vk::Format m_Format;
private:
	vk::Device m_Device;
};
