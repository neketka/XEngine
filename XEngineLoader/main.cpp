#include "pch.h"
#include <SDLInterface.h>
#include <Windows.h>
#include <thread>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	int t = std::thread::hardware_concurrency() * 3 / 4;
	XEngine::InitializeEngine("Test", t);
	XEngine::GetInstance().AddInterface(new SDLInterface, HardwareInterfaceType::Display);
	XEngine::GetInstance().Run();

	return 0;
}