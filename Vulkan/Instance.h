#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

class Instance
{
public:
	Instance();
	~Instance();

	const vk::Instance* operator ->() const;
	vk::Instance operator *() const;
private:
	static const std::vector<const char*> s_RequiredExtensions;
	vk::Instance m_Instance;
#ifdef _DEBUG
	static const std::vector<const char*> s_ValidationLayers;
	static VkBool32 __stdcall debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userdata);
	bool checkValidationLayerSupport() const;
	VkDebugReportCallbackEXT m_Callback;
#endif
};
