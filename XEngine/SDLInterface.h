#pragma once

#include "exports.h"

#include "DisplayInterface.h"
#include "KeyboardInterface.h"
#include "MouseInterface.h"
#include "JoystickInterface.h"

class SDLInterface : public DisplayInterface
{
public:
	SDLInterface() {}
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
	XENGINEAPI virtual GraphicsContext *GetGraphicsContext() override;
private:
	HardwareStatus m_displayStatus = HardwareStatus::Uninitialized;
	bool m_vsync = false;
	void *m_window = nullptr;
	void *m_glcontext = nullptr;
};
