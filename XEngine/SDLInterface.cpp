#include "pch.h"
#include "SDLInterface.h"

#include <SDL.h>

std::string SDLInterface::GetName()
{
	return "SDL Video, Keyboard, Joystick, and Mouse Interface";
}

void SDLInterface::Initialize(HardwareInterfaceType type)
{
	if (type == HardwareInterfaceType::Display)
	{
		if (SDL_VideoInit(NULL) == -1)
		{
			XEngine::GetInstance().LogMessage("SDL Video did not initialize!", LogMessageType::Error);
			m_displayStatus = HardwareStatus::ErrorState;
			return;
		}

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

		m_window = SDL_CreateWindow(XEngine::GetInstance().GetName().c_str(), 100, 100, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		m_glcontext = SDL_GL_CreateContext(static_cast<SDL_Window *>(m_window));

		m_displayStatus = HardwareStatus::Initialized;
	}
}

void SDLInterface::Destroy(HardwareInterfaceType type)
{
	if (type == HardwareInterfaceType::Display)
	{
		SDL_GL_DeleteContext(m_glcontext);
		SDL_DestroyWindow(static_cast<SDL_Window *>(m_window));
		SDL_VideoQuit();
		m_displayStatus = HardwareStatus::Disposed;
	}
}

void SDLInterface::BeginFrame()
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
			}
		}
	}
}

void SDLInterface::EndFrame()
{
	if (m_displayStatus == HardwareStatus::Initialized)
	{
		SDL_GL_SwapWindow(static_cast<SDL_Window *>(m_window));
	}
}

void SDLInterface::SetVSyncEnabled(bool state)
{
	SDL_GL_SetSwapInterval(state ? 1 : 0);
	m_vsync = state;
}

bool SDLInterface::IsVSyncEnabled()
{
	return m_vsync;
}

void SDLInterface::SetDisplayWindowMode(DisplayWindowMode mode)
{
}

DisplayWindowMode SDLInterface::GetDisplayWindowMode()
{
	return DisplayWindowMode::Windowed;
}

GraphicsContext *SDLInterface::GetGraphicsContext()
{
	return nullptr;
}

HardwareStatus SDLInterface::GetStatus(HardwareInterfaceType type)
{
	if (type == HardwareInterfaceType::Display)
	{
		return m_displayStatus;
	}
	return HardwareStatus::ErrorState;
}
