#pragma once
#include "exports.h"

#include "CommonInterface.h"
#include "GraphicsDefs.h"

enum class DisplayWindowMode
{
	Windowed, Fullscreen, WindowedFullscreen
};

class XENGINEAPI DisplayInterface : public HardwareInterface
{
public:
	virtual void SetVSyncEnabled(bool state) = 0;
	virtual bool IsVSyncEnabled() = 0;
	virtual void SetDisplayWindowMode(DisplayWindowMode mode) = 0;
	virtual DisplayWindowMode GetDisplayWindowMode() = 0;
	virtual GraphicsContext *GetGraphicsContext() = 0;
};