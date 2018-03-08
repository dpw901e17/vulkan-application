#include "Instance.h"
#include <iostream>
#include "Utility.h"

const std::vector<const char*> Instance::s_RequiredExtensions = {
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef _DEBUG
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
};

#ifdef _DEBUG
const std::vector<const char*> Instance::s_ValidationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

bool Instance::checkValidationLayerSupport() const
{
	for (auto layerName : s_ValidationLayers) {
		auto layerFound = false;

		for (const auto& layerProperties : vk::enumerateInstanceLayerProperties()) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Instance::debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userdata)
{
	std::cerr << "Validation Layer: " << msg << std::endl;
	return false;
}
#endif

Instance::Instance()
{
	//VkApplicationInfo is technically optional, but can be used to optimize
	vk::ApplicationInfo application_info("Hello Triangle", VK_MAKE_VERSION(1, 0, 0), "No Engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

	//not optional!
	vk::InstanceCreateInfo instance_info;
	instance_info.setPApplicationInfo(&application_info)
		.setEnabledExtensionCount(s_RequiredExtensions.size())
		.setPpEnabledExtensionNames(s_RequiredExtensions.data());

#ifdef _DEBUG
	if (!checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}
	instance_info.setEnabledLayerCount(s_ValidationLayers.size())
		.setPpEnabledLayerNames(s_ValidationLayers.data());
#endif
	m_Instance = createInstance(instance_info);

#ifdef _DEBUG
	VkDebugReportCallbackCreateInfoEXT callback_info = {};
	callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	callback_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	callback_info.pfnCallback = debugCallback;

	auto result = CreateDebugReportCallbackEXT(static_cast<VkInstance>(m_Instance), &callback_info, nullptr, &m_Callback);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to set up debug callback!");
	}
#endif
}

Instance::~Instance()
{
#ifdef _DEBUG
	DestroyDebugReportCallbackEXT(static_cast<VkInstance>(m_Instance), m_Callback);
#endif
	m_Instance.destroy();
}

const vk::Instance* Instance::operator->() const
{
	return &m_Instance;
}

vk::Instance Instance::operator*() const
{
	return m_Instance;
}
