#include "pch.h"
#include "SDLVKInterface.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <iostream>

std::string SDLVKInterface::GetName()
{
	return "SDL Video, Keyboard, Joystick, and Mouse Interface";
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	SDL_MessageBoxData dat;
	dat.message = pCallbackData->pMessage;
	SDL_ShowMessageBox(&dat, nullptr);

	return VK_FALSE;
}

void SDLVKInterface::Initialize(HardwareInterfaceType type)
{
	if (type == HardwareInterfaceType::Display)
	{
		if (SDL_VideoInit(NULL) == -1)
		{
			XEngine::GetInstance().LogMessage("SDL Video did not initialize!", LogMessageType::Error);
			m_displayStatus = HardwareStatus::ErrorState;
			return;
		}

		m_window = SDL_CreateWindow(XEngine::GetInstance().GetName().c_str(), 100, 100, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

		uint32_t count;
		SDL_Vulkan_GetInstanceExtensions(reinterpret_cast<SDL_Window *>(m_window), &count, nullptr);
		
		std::vector<const char *> layers = {
			"VK_LAYER_KHRONOS_validation"
		};
		std::vector<const char *> exts(count + 1);
		SDL_Vulkan_GetInstanceExtensions(reinterpret_cast<SDL_Window *>(m_window), &count, exts.data());
		exts[count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

		vk::ApplicationInfo appInfo;
		appInfo.setApiVersion(VK_MAKE_VERSION(1, 2, 0));
		appInfo.setApplicationVersion(VK_MAKE_VERSION(0, 1, 0));
		appInfo.setEngineVersion(VK_MAKE_VERSION(0, 1, 0));
		appInfo.setPApplicationName("XEngine");
		appInfo.setPEngineName("XEngine");

		vk::InstanceCreateInfo createInfo;
		createInfo.setEnabledExtensionCount(exts.size());
		createInfo.setPpEnabledExtensionNames(exts.data());
		createInfo.setEnabledLayerCount(layers.size());
		createInfo.setPpEnabledLayerNames(layers.data());
		createInfo.setPApplicationInfo(&appInfo);

		m_vkInstance = vk::createInstance(createInfo);

		VkDebugUtilsMessengerCreateInfoEXT mcreateInfo{};
		mcreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		mcreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		mcreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		mcreateInfo.pfnUserCallback = debugCallback;
		mcreateInfo.pUserData = nullptr; // Optional

		PFN_vkCreateDebugUtilsMessengerEXT CreateDebugReportCallback = VK_NULL_HANDLE;
		CreateDebugReportCallback = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugUtilsMessengerEXT");

		CreateDebugReportCallback(m_vkInstance, &mcreateInfo, nullptr, &m_debugMessenger);

		std::vector<vk::PhysicalDevice> devices = m_vkInstance.enumeratePhysicalDevices();
		m_physicalDevice = devices[0];
		for (vk::PhysicalDevice& d : devices)
		{
			vk::PhysicalDeviceProperties props = d.getProperties();
			if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				m_physicalDevice = d;
				break;
			}
		}

		VkSurfaceKHR khr;
		SDL_Vulkan_CreateSurface(reinterpret_cast<SDL_Window *>(m_window), m_vkInstance, &khr);
		m_surface = khr;

		m_graphicsQueueFamilyIndex = 0;
		m_presentQueueFamilyIndex = 0;
		std::vector<vk::QueueFamilyProperties> queueFamilyProps = m_physicalDevice.getQueueFamilyProperties();
		for (int32_t i = 0; i < queueFamilyProps.size(); ++i)
		{
			if (queueFamilyProps[i].queueFlags & vk::QueueFlagBits::eGraphics)
				m_graphicsQueueFamilyIndex = i;
			if (m_physicalDevice.getSurfaceSupportKHR(i, m_surface))
				m_presentQueueFamilyIndex = i;
		}

		vk::PhysicalDeviceFeatures features;

		float queueP = 1.f;

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

		std::vector<const char *> deviceExts = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		vk::DeviceQueueCreateInfo queueCreateInfo;

		queueCreateInfo.setQueueCount(1);
		queueCreateInfo.setQueueFamilyIndex(m_graphicsQueueFamilyIndex);
		queueCreateInfo.setPQueuePriorities(&queueP);

		queueCreateInfos.push_back(queueCreateInfo);

		if (m_graphicsQueueFamilyIndex != m_presentQueueFamilyIndex)
		{
			queueCreateInfo.setQueueFamilyIndex(m_presentQueueFamilyIndex);
			queueCreateInfos.push_back(queueCreateInfo);
		}

		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.setEnabledLayerCount(layers.size());
		deviceCreateInfo.setPpEnabledLayerNames(layers.data());
		deviceCreateInfo.setEnabledExtensionCount(deviceExts.size());
		deviceCreateInfo.setPpEnabledExtensionNames(deviceExts.data());
		deviceCreateInfo.setPEnabledFeatures(&features);
		deviceCreateInfo.setQueueCreateInfoCount(queueCreateInfos.size());
		deviceCreateInfo.setPQueueCreateInfos(queueCreateInfos.data());

		m_device = m_physicalDevice.createDevice(deviceCreateInfo);
		m_graphicsQueue = m_device.getQueue(m_graphicsQueueFamilyIndex, 0);
		m_presentQueue = m_device.getQueue(m_presentQueueFamilyIndex, 0);

		m_sCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
		std::vector<vk::SurfaceFormatKHR> sFormats = m_physicalDevice.getSurfaceFormatsKHR(m_surface);
		std::vector<vk::PresentModeKHR> sPresents = m_physicalDevice.getSurfacePresentModesKHR(m_surface);

		for (vk::SurfaceFormatKHR& format : sFormats)
		{
			if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && format.format == vk::Format::eB8G8R8A8Srgb)
				m_surfaceFormat = format;
		}

		for (vk::PresentModeKHR& present : sPresents)
		{
			if (present == vk::PresentModeKHR::eMailbox)
			{
				m_presentMode = present;
				break;
			}
			m_presentMode = vk::PresentModeKHR::eFifo;
		}

		m_extent.setWidth(glm::clamp<uint32_t>(800, m_sCapabilities.minImageExtent.width, m_sCapabilities.maxImageExtent.width));
		m_extent.setHeight(glm::clamp<uint32_t>(800, m_sCapabilities.minImageExtent.height, m_sCapabilities.maxImageExtent.height));

		vk::SwapchainCreateInfoKHR swapInfo;
		swapInfo.setSurface(m_surface);
		swapInfo.setMinImageCount(m_sCapabilities.minImageCount + 1);
		swapInfo.setImageFormat(m_surfaceFormat.format);
		swapInfo.setClipped(true);
		swapInfo.setImageColorSpace(m_surfaceFormat.colorSpace);
		swapInfo.setImageExtent(m_extent);
		swapInfo.setImageArrayLayers(1);
		swapInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
		swapInfo.setImageSharingMode(vk::SharingMode::eExclusive);
		swapInfo.setPreTransform(m_sCapabilities.currentTransform);
		swapInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);

		m_swapchain = m_device.createSwapchainKHR(swapInfo);
		m_swapchainImages = m_device.getSwapchainImagesKHR(m_swapchain);

		for (vk::Image& image : m_swapchainImages)
		{
			vk::ImageViewCreateInfo imgInfo;
			imgInfo.setViewType(vk::ImageViewType::e2D);
			imgInfo.components.r = vk::ComponentSwizzle::eIdentity;
			imgInfo.components.g = vk::ComponentSwizzle::eIdentity;
			imgInfo.components.b = vk::ComponentSwizzle::eIdentity;
			imgInfo.components.a = vk::ComponentSwizzle::eIdentity;

			imgInfo.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
			imgInfo.subresourceRange.setBaseArrayLayer(0);
			imgInfo.subresourceRange.setBaseMipLevel(0);
			imgInfo.subresourceRange.setLayerCount(1);
			imgInfo.subresourceRange.setLevelCount(1);

			imgInfo.setFormat(m_surfaceFormat.format);
			imgInfo.setImage(image);

			m_swapchainImageViews.push_back(m_device.createImageView(imgInfo));
		}

		vk::SemaphoreCreateInfo seminfo;
		m_renderFinished = m_device.createSemaphore(seminfo);
		m_imageAvailable = m_device.createSemaphore(seminfo);

		vk::CommandPoolCreateInfo cpcInfo;
		cpcInfo.setQueueFamilyIndex(m_presentQueueFamilyIndex);
		cpcInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
		
		m_commandPool = m_device.createCommandPool(cpcInfo);

		vk::CommandBufferAllocateInfo cmdInfo;
		cmdInfo.setCommandPool(m_commandPool);
		cmdInfo.setLevel(vk::CommandBufferLevel::ePrimary);
		cmdInfo.setCommandBufferCount(m_swapchainImages.size());

		m_commandBuffers = m_device.allocateCommandBuffers(cmdInfo);

		m_displayStatus = HardwareStatus::Initialized;
	}
}

void SDLVKInterface::Destroy(HardwareInterfaceType type)
{
	if (type == HardwareInterfaceType::Display)
	{
		m_device.resetCommandPool(m_commandPool, vk::CommandPoolResetFlagBits::eReleaseResources);
		m_device.destroyCommandPool(m_commandPool);

		for (vk::ImageView& view : m_swapchainImageViews)
			m_device.destroyImageView(view);

		m_device.destroySemaphore(m_renderFinished);
		m_device.destroySemaphore(m_imageAvailable);

		PFN_vkDestroyDebugUtilsMessengerEXT DestroyCallback = VK_NULL_HANDLE;
		DestroyCallback = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugUtilsMessengerEXT");

		m_device.destroySwapchainKHR(m_swapchain);
		vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
		m_device.destroy();

		DestroyCallback(m_vkInstance, m_debugMessenger, nullptr);
		m_vkInstance.destroy();

		SDL_DestroyWindow(static_cast<SDL_Window *>(m_window));
		SDL_VideoQuit();
		m_displayStatus = HardwareStatus::Disposed;
	}
}

void SDLVKInterface::BeginFrame()
{
	if (m_displayStatus == HardwareStatus::Initialized)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				XEngine::GetInstance().Shutdown();
				break;
			case SDL_WINDOWEVENT:
				if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				{
					glm::ivec2 size;
					SDL_Vulkan_GetDrawableSize(static_cast<SDL_Window *>(m_window), &size.x, &size.y);
				}
				break;
			}
		}
	}
}

void SDLVKInterface::EndFrame()
{
	if (m_displayStatus == HardwareStatus::Initialized)
	{
		vk::ResultValue<uint32_t> imgIndex = m_device.acquireNextImageKHR(m_swapchain, 1e10, m_imageAvailable, nullptr);

		vk::CommandBuffer cmdBuf = m_commandBuffers[imgIndex.value];
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
		
		cmdBuf.begin(beginInfo);

		vk::ClearColorValue clearVal;
		clearVal.setFloat32({ 1, 0, 0, 1 });

		vk::ImageSubresourceRange range;

		range.setAspectMask(vk::ImageAspectFlagBits::eColor);
		range.setBaseArrayLayer(0);
		range.setBaseMipLevel(0);
		range.setLayerCount(1);
		range.setLevelCount(1);

		vk::ImageMemoryBarrier srcBarrier;
		srcBarrier.setImage(m_swapchainImages[imgIndex.value]);
		srcBarrier.setOldLayout(vk::ImageLayout::ePresentSrcKHR);
		srcBarrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
		srcBarrier.setSrcQueueFamilyIndex(m_presentQueueFamilyIndex);
		srcBarrier.setDstQueueFamilyIndex(m_presentQueueFamilyIndex);
		srcBarrier.setSubresourceRange(range);
		srcBarrier.setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite);
		srcBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

		vk::ImageMemoryBarrier dstBarrier;
		dstBarrier.setImage(m_swapchainImages[imgIndex.value]);
		dstBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
		dstBarrier.setNewLayout(vk::ImageLayout::ePresentSrcKHR);
		dstBarrier.setSrcQueueFamilyIndex(m_presentQueueFamilyIndex);
		dstBarrier.setDstQueueFamilyIndex(m_presentQueueFamilyIndex);
		dstBarrier.setSubresourceRange(range);
		dstBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		dstBarrier.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);

		cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlagBits::eByRegion, {}, {}, { srcBarrier });
		cmdBuf.clearColorImage(m_swapchainImages[imgIndex.value], vk::ImageLayout::eTransferDstOptimal, clearVal, { range });
		cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::DependencyFlagBits::eByRegion, {}, {}, { dstBarrier });

		cmdBuf.end();

		vk::PipelineStageFlags pipeFlags = vk::PipelineStageFlagBits::eBottomOfPipe;

		vk::SubmitInfo submitInfo;
		submitInfo.setCommandBufferCount(1);
		submitInfo.setPCommandBuffers(&cmdBuf);
		submitInfo.setPSignalSemaphores(&m_renderFinished);
		submitInfo.setPWaitSemaphores(&m_imageAvailable);
		submitInfo.setSignalSemaphoreCount(1);
		submitInfo.setWaitSemaphoreCount(1);
		submitInfo.setPWaitDstStageMask(&pipeFlags);

		m_presentQueue.submit({ submitInfo }, nullptr);

		vk::PresentInfoKHR presentInfo;
		presentInfo.setPImageIndices(&imgIndex.value);
		presentInfo.setPSwapchains(&m_swapchain);
		presentInfo.setPWaitSemaphores(&m_renderFinished);
		presentInfo.setSwapchainCount(1);
		presentInfo.setWaitSemaphoreCount(1);

		m_presentQueue.presentKHR(presentInfo);
	}
}

void SDLVKInterface::SetVSyncEnabled(bool state)
{
	m_vsync = state;
}

bool SDLVKInterface::IsVSyncEnabled()
{
	return m_vsync;
}

void SDLVKInterface::SetDisplayWindowMode(DisplayWindowMode mode)
{
}

DisplayWindowMode SDLVKInterface::GetDisplayWindowMode()
{
	return DisplayWindowMode::Windowed;
}

HardwareStatus SDLVKInterface::GetStatus(HardwareInterfaceType type)
{
	if (type == HardwareInterfaceType::Display)
	{
		return m_displayStatus;
	}
	return HardwareStatus::ErrorState;
}
