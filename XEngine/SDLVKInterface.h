#pragma once

#include "exports.h"

#include "DisplayInterface.h"
#include "KeyboardInterface.h"
#include "MouseInterface.h"
#include "JoystickInterface.h"

#include <vulkan/vulkan.hpp>

class SDLVKInterface : public DisplayInterface
{
public:
	SDLVKInterface() {}
	XENGINEAPI virtual std::string GetName() override;
	XENGINEAPI virtual void Initialize(HardwareInterfaceType type) override;
	XENGINEAPI virtual void Destroy(HardwareInterfaceType type) override;
	XENGINEAPI virtual HardwareStatus GetStatus(HardwareInterfaceType type) override;
	XENGINEAPI virtual void BeginFrame() override;
	XENGINEAPI virtual void EndFrame() override;
	XENGINEAPI virtual void SetVSyncEnabled(bool state) override;
	XENGINEAPI virtual bool IsVSyncEnabled() override;
	XENGINEAPI virtual void SetDisplayWindowMode(DisplayWindowMode mode) override;
	XENGINEAPI virtual DisplayWindowMode GetDisplayWindowMode() override;
private:
	HardwareStatus m_displayStatus = HardwareStatus::Uninitialized;
	bool m_vsync = false;
	void *m_window = nullptr;

	vk::Queue m_graphicsQueue;
	vk::Queue m_presentQueue;

	vk::Device m_device;

	VkDebugUtilsMessengerEXT m_debugMessenger;
	vk::Instance m_vkInstance;
	vk::PhysicalDevice m_physicalDevice;
	vk::SurfaceKHR m_surface;

	vk::SurfaceFormatKHR m_surfaceFormat;
	vk::PresentModeKHR m_presentMode;
	vk::SurfaceCapabilitiesKHR m_sCapabilities;
	vk::Extent2D m_extent;

	vk::SwapchainKHR m_swapchain;
	std::vector<vk::Image> m_swapchainImages;
	std::vector<vk::ImageView> m_swapchainImageViews;

	vk::Semaphore m_renderFinished;
	vk::Semaphore m_imageAvailable;

	vk::CommandPool m_commandPool;
	std::vector<vk::CommandBuffer> m_commandBuffers;

	int32_t m_graphicsQueueFamilyIndex;
	int32_t m_presentQueueFamilyIndex;
};
