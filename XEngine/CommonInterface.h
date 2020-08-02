#pragma once
#include <string>
#include "exports.h"

enum class HardwareInterfaceType
{
	AudioRecording = 1, AudioRendering = 2, Joystick = 4,
	Keyboard = 8, Mouse = 16, Network = 32,
	VideoRecording = 64, Display = 128
};

inline HardwareInterfaceType operator|(HardwareInterfaceType a, HardwareInterfaceType b)
{
	return static_cast<HardwareInterfaceType>(static_cast<int32_t>(a) | static_cast<int32_t>(b));
}

inline bool operator&(HardwareInterfaceType a, HardwareInterfaceType b)
{
	return static_cast<bool>(static_cast<int32_t>(a) & static_cast<int32_t>(b));
}

enum class HardwareStatus 
{
	Uninitialized, Initialized, ErrorState, Disposed
};

class XENGINEAPI HardwareInterface
{
public:
	virtual std::string GetName() = 0;
	virtual HardwareStatus GetStatus(HardwareInterfaceType type) = 0;
	virtual void Initialize(HardwareInterfaceType type) = 0;
	virtual void BeginFrame() = 0;
	virtual void Destroy(HardwareInterfaceType type) = 0;
	virtual void EndFrame() = 0;
};