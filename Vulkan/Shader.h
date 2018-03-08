#pragma once
#include <vulkan/vulkan.hpp>
#include "Utility.h"

class Shader
{
public:
	Shader(vk::Device device, std::string file_path, vk::ShaderStageFlagBits stage);
	~Shader();

	vk::PipelineShaderStageCreateInfo m_Info;
private:
	vk::Device m_Device;
	vk::ShaderModule m_Module;

};
