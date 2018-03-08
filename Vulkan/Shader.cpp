#include "Shader.h"

vk::ShaderModule createShaderModule(vk::Device device, const std::vector<char>& source_code)
{
	vk::ShaderModuleCreateInfo createInfo = {};
	createInfo.codeSize = source_code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(source_code.data());
	return device.createShaderModule(createInfo);
}

Shader::Shader(vk::Device device, std::string file_path, vk::ShaderStageFlagBits stage)
	: m_Device(device)
{
	auto source = readFile(file_path);
	m_Module = createShaderModule(device, source);

	m_Info.stage = stage;
	m_Info.module = m_Module;
	m_Info.pName = "main";
}

Shader::~Shader()
{
	m_Device.destroyShaderModule(m_Module);
}
