#include "pch.h"
#include <SDLVKInterface.h>
#include <thread>

int32_t __stdcall WinMain(void *hInstance, void *hPrevInstance, char *lpCmdLine, int32_t nShowCmd)
{
	int32_t t = std::thread::hardware_concurrency() * 3 / 4;
	XEngine::InitializeEngine("Test", t);
	XEngine::GetInstance().AddInterface(new SDLVKInterface, HardwareInterfaceType::Display);
	XEngine::GetInstance().Run();

	return 0;
}