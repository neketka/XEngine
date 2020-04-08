#include "pch.h"
#include "XEngine.h"

#include "TestSystem.h"

XEngine *XEngine::m_engineInstance;

void XEngine::InitializeEngine(std::string name, int threadCount, bool defaultSystems, std::string rootPath)
{
	m_engineInstance = new XEngine;
	m_engineInstance->m_name = name;
	m_engineInstance->m_maxECSThreads = std::max(0, threadCount - 1);
	m_engineInstance->m_workerManager = new WorkerManager(threadCount);
	
	m_engineInstance->m_ecsRegistrar->RegisterComponent<TestComponent>();
	m_engineInstance->m_ecsRegistrar->AddSystem(new TestSystem);

	Scene *scene = new Scene("Test Scene");
	scene->GetSystemManager()->AddSystem("TestSystem");
	m_engineInstance->m_ecsRegistrar->GetSystem("TestSystem")->SetEnabled(true);

	for (int i = 0; i < 1000; ++i)
		scene->GetEntityManager()->CreateEntity({ "TestComponent" });

	m_engineInstance->SetScene(scene);
}

XEngine& XEngine::GetInstance()
{
	return *XEngine::m_engineInstance;
}

void XEngine::DoIdleWork()
{
}

void XEngine::AddInterface(HardwareInterface *interface, HardwareInterfaceType type)
{
	bool hasAlreadyBeenAdded = false;
	for (int i = 0; i < 8; ++i)
	{
		unsigned int mask = (0b1 << i) & static_cast<unsigned int>(type);
		if (mask)
		{
			m_hwInterfaces[static_cast<HardwareInterfaceType>(mask)] = interface;
			m_excludeFromFrame[static_cast<HardwareInterfaceType>(mask)] = hasAlreadyBeenAdded;
			hasAlreadyBeenAdded = true;
		}
	}
}

XEngine::XEngine() : m_scene(nullptr)
{
	m_sysManager = new SubsystemManager;
	m_ecsRegistrar = new ECSRegistrar;
}

XEngine::~XEngine()
{
}

void XEngine::Run()
{
	Init();
	float sumOfTimeForInterval = 0;
	float deltaTime = 0;
	int intervalCount = 0;
	m_beginTime = std::chrono::high_resolution_clock::now();
	m_running = true;
	while (m_running)
	{
		std::chrono::time_point begin = std::chrono::high_resolution_clock::now();

		Tick(deltaTime);

		std::chrono::time_point end = std::chrono::high_resolution_clock::now();

		deltaTime = std::chrono::duration<float>(end - begin).count(); // Get a benchmark dt

		float minFrameTime = 1.f / static_cast<float>(m_maxFps);
		float timeDiff = minFrameTime - deltaTime; // Check if frame is too fast
		/*
		if (timeDiff >= 0.005f) // If the number is not too precise
			std::this_thread::sleep_for(std::chrono::duration<float>(timeDiff));
		else if (timeDiff > 0.f) // Too precise
		{
			long long then = std::chrono::high_resolution_clock::now().time_since_epoch().count() + timeDiff * 1e9;
			while (std::chrono::high_resolution_clock::now().time_since_epoch().count() < then); // Spinlock: try to avoid by doing extra work
		}
		*/
		end = std::chrono::high_resolution_clock::now();

		deltaTime = std::chrono::duration<float>(end - begin).count(); // Get net delta time

		++intervalCount;
		sumOfTimeForInterval += deltaTime;

		if (intervalCount >= m_fpsAvgInterval) // If the amount of intervals to find an average has been reached
		{
			m_frameTimeAvg = sumOfTimeForInterval / static_cast<float>(intervalCount);
			m_fps = 1.f / m_frameTimeAvg;
			intervalCount = 0;
			sumOfTimeForInterval = 0;
		}
	}
	Cleanup();
}

void XEngine::Shutdown()
{
	m_running = false;
}

bool XEngine::IsRunning()
{
	return m_running;
}

void XEngine::AddPropertySet(std::map<std::string, int> properties)
{
	for (auto kp : properties)
	{
		m_intProps[kp.first] = kp.second;
	}
}

void XEngine::AddPropertySet(std::map<std::string, std::string> properties)
{
	for (auto kp : properties)
	{
		m_stringProps[kp.first] = kp.second;
	}
}

void XEngine::SetProperty(std::string key, int value)
{
	m_intProps[key] = value;
}

void XEngine::SetProperty(std::string key, std::string value)
{
	m_stringProps[key] = value;
}

std::string *XEngine::GetStringProperty(std::string key)
{
	auto itr = m_stringProps.find(key);
	return (itr == m_stringProps.end() ? nullptr : &itr->second);
}

int *XEngine::GetIntProperty(std::string key)
{
	auto itr = m_intProps.find(key);
	return (itr == m_intProps.end() ? nullptr : &itr->second);
}

void XEngine::SaveProperties() // Do not implement, load on start, save on stop
{
}

void XEngine::LoadProperties()
{
}

float XEngine::GetTime()
{
	return std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - m_beginTime).count();
}

void XEngine::AddBeginMarker(std::string label)
{
}

void XEngine::AddEndMarker(std::string label)
{
}

void XEngine::LogMessage(std::string message, LogMessageType type)
{
	switch (type)
	{
	case LogMessageType::Error:
		break;
	case LogMessageType::Warning:
		break;
	case LogMessageType::Message:
		break;
	case LogMessageType::Internal:
		break;
	}
}

void XEngine::RaiseCriticalError(std::string error)
{
	LogMessage("CRITICAL ERROR (shutdown necessary) " + error, LogMessageType::Error);
	Shutdown();
}

void XEngine::SetRootPath(std::string rootPath)
{
	m_rootPath = rootPath;
}

void XEngine::SetScene(Scene *scene)
{
	if (m_scene)
	{
		LogMessage("Disabling scene \"" + m_scene->GetName() + "\"", LogMessageType::Message);
		m_scene->DisableScene();
	}

	m_scene = scene;
	m_sysManager->SetSceneManager(scene->GetSystemManager());
	LogMessage("Enabling scene \"" + scene->GetName() + "\"", LogMessageType::Message);
	scene->EnableScene();
}

Scene *XEngine::GetScene()
{
	return m_scene;
}

SubsystemManager *XEngine::GetSubsystemManager()
{
	return m_sysManager;
}

ECSRegistrar *XEngine::GetECSRegistrar()
{
	return m_ecsRegistrar;
}

WorkerManager *XEngine::GetWorkerManager()
{
	return m_workerManager;
}

void XEngine::Tick(float deltaTime)
{
	m_workerManager->CheckForFree();

	for (auto kp : m_hwInterfaces)
	{
		if (kp.second && kp.second->GetStatus(kp.first) == HardwareStatus::Initialized && !m_excludeFromFrame[kp.first])
			kp.second->BeginFrame();
	}

	if (m_scene)
	{
		m_sysManager->ScheduleJobs();
		m_sysManager->ExecuteJobs(0, deltaTime); // Figure out threading later
		m_scene->GetComponentManager()->ExecuteSingleThreadOps();
	}

	for (auto kp : m_hwInterfaces)
	{
		if (kp.second && kp.second->GetStatus(kp.first) == HardwareStatus::Initialized && !m_excludeFromFrame[kp.first])
			kp.second->EndFrame();
	}
}

std::map<HardwareInterfaceType, std::string> interfaceToName{
	{ HardwareInterfaceType::Display, "display" }, { HardwareInterfaceType::AudioRecording, "audio recording" }, { HardwareInterfaceType::AudioRendering, "audio rendering" }, { HardwareInterfaceType::Joystick, "joystick" },
	{ HardwareInterfaceType::Keyboard, "keyboard" }, { HardwareInterfaceType::Mouse, "mouse" }, { HardwareInterfaceType::Network, "network" }, { HardwareInterfaceType::VideoRecording, "video recording" }
};

void XEngine::Init()
{
	for (auto kp : m_hwInterfaces)
	{
		if (kp.second && kp.second->GetStatus(kp.first) == HardwareStatus::Uninitialized)
		{
			LogMessage("Initializing " + interfaceToName[kp.first] + " interface of " + kp.second->GetName(), LogMessageType::Message);
			kp.second->Initialize(kp.first);
			if (kp.second->GetStatus(kp.first) == HardwareStatus::ErrorState)
				LogMessage(kp.second->GetName() + " reported error state during initialization", LogMessageType::Message);
		}
	}
}

void XEngine::Cleanup()
{
	for (auto kp : m_hwInterfaces)
	{
		if (kp.second && kp.second->GetStatus(kp.first) == HardwareStatus::Initialized)
		{
			LogMessage("Cleaning up " + interfaceToName[kp.first] + " interface of " + kp.second->GetName(), LogMessageType::Message);
			kp.second->Destroy(kp.first);
		}
	}

	if (m_scene)
	{
		m_scene->DisableScene();
		delete m_scene;
	}
}
