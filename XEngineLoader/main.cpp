#include "pch.h"
#include <SDLInterface.h>
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	XEngine::InitializeEngine("Test", 9);
	XEngine::GetInstance().AddInterface(new SDLInterface, HardwareInterfaceType::Display);
	XEngine::GetInstance().Run();

	return 0;
}