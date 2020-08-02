#include "pch.h"
#include "XEngine.h"

#include "TestSystem.h"

#include "MeshAsset.h"
#include "TextureAsset.h"

#include "OBJMeshImporter.h"
#include "ImageImporter.h"

XEngine *XEngineInstance;
XEngine *XEngine::m_engineInstance;

void XEngine::InitializeEngine(std::string name, int32_t threadCount, bool defaultSystems, std::string rootPath)
{
	XEngineInstance = m_engineInstance = new XEngine;
	m_engineInstance->m_name = name;
	m_engineInstance->m_maxECSThreads = std::max(0, threadCount - 2);
	m_engineInstance->m_rootPath = rootPath;
	m_engineInstance->m_ecsThreads = new std::thread *[m_engineInstance->m_maxECSThreads];
}

XEngine& XEngine::GetInstance()
{
	return *XEngine::m_engineInstance;
}

void XEngine::DoIdleWork()
{
	//std::this_thread::sleep_for(std::chrono::milliseconds(0));
	std::this_thread::yield();
}

void XEngine::AddInterface(HardwareInterface *interface, HardwareInterfaceType type)
{
	bool hasAlreadyBeenAdded = false;
	for (int32_t i = 0; i < 8; ++i)
	{
		uint32_t mask = (0b1 << i) & static_cast<uint32_t>(type);
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
	m_engineInstanceId = GenerateID();

	m_assetManager = new AssetManager(8e8, 2e9); // Hardcoded numbers

	m_sysManager = new SubsystemManager;
	m_ecsRegistrar = new ECSRegistrar;
}

XEngine::~XEngine()
{
	delete m_sysManager;
	delete m_ecsRegistrar;
	delete m_assetManager;
	delete[] m_ecsThreads;
}

void XEngine::Run()
{
	float sumOfTimeForInterval = 0;
	float deltaTime = 0;
	int32_t intervalCount = 0;
	m_beginTime = std::chrono::high_resolution_clock::now();
	m_running = true;
	Init();
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

void XEngine::AddPropertySet(std::map<std::string, int32_t> properties)
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

void XEngine::SetProperty(std::string key, int32_t value)
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

int32_t *XEngine::GetIntProperty(std::string key)
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

std::string& XEngine::GetRootPath()
{
	return m_rootPath;
}

UniqueId& XEngine::GetInstanceId()
{
	return m_engineInstanceId;
}

float XEngine::GetTime(UniqueId id)
{
	float timeDelta = (std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - m_beginTime).count() - m_time) * m_globalTimeScale;
	m_time += timeDelta;
	auto scaleElement = m_timeAndScale.find(id);
	if (id == 0 || scaleElement == m_timeAndScale.end())
		return m_time;
	else
		return scaleElement->second.x += timeDelta * scaleElement->second.y;
}

void XEngine::SetGlobalTimeScale(float scale)
{
	GetTime(0);
	m_globalTimeScale = scale;
}

void XEngine::SetLocalTimeScale(UniqueId id, float scale)
{
	if (GetTime(id) == m_time)
		m_timeAndScale[id] = glm::ivec2(m_time, scale);
	else
		m_timeAndScale[id].y = scale;
}

float XEngine::AdjustDeltaTime(UniqueId id, float deltaTime)
{
	auto scaleElement = m_timeAndScale.find(id);
	if (id == 0 || scaleElement == m_timeAndScale.end())
		return deltaTime;
	else
		return scaleElement->second.x += deltaTime * scaleElement->second.y;
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

AssetManager *XEngine::GetAssetManager()
{
	return m_assetManager;
}

void XEngine::Tick(float deltaTime)
{
	for (auto kp : m_hwInterfaces)
	{
		if (kp.second && kp.second->GetStatus(kp.first) == HardwareStatus::Initialized && !m_excludeFromFrame[kp.first])
			kp.second->BeginFrame();
	}

	if (m_scene)
	{
		m_sysManager->ScheduleJobs();
		m_ecsDt = deltaTime;
		m_ecsQueued = m_maxECSThreads;
		m_sysManager->ExecuteJobs(0, deltaTime);
		m_scene->GetComponentManager()->ExecuteSingleThreadOps();
	}

	for (auto kp : m_hwInterfaces)
	{
		if (kp.second && kp.second->GetStatus(kp.first) == HardwareStatus::Initialized && !m_excludeFromFrame[kp.first])
			kp.second->EndFrame();
	}
}

std::map<HardwareInterfaceType, std::string> interfaceToName {
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

	m_ecsQueued = 0;
	for (int32_t i = 0; i < m_maxECSThreads; ++i)
	{
		std::thread *t = new std::thread(&XEngine::RunECSThread, this, i + 1);
		m_ecsThreads[i] = t;
	}

	m_engineInstance->m_ecsRegistrar->RegisterComponent<TestComponent>();
	m_engineInstance->m_ecsRegistrar->AddSystem(new TestSystem);

	m_assetManager->RegisterLoader(new MeshAssetLoader(1e12, 1e8));
	m_assetManager->RegisterLoader(new TextureAssetLoader);
	m_assetManager->RegisterImporter(new OBJMeshImporter);
	m_assetManager->RegisterImporter(new ImageImporter);

	Scene *scene = new Scene("Test Scene");
	scene->GetSystemManager()->AddSystem("TestSystem");

	m_engineInstance->m_ecsRegistrar->GetSystem("TestSystem")->SetEnabled(true);

	for (int32_t i = 0; i < 800; ++i)
		scene->GetEntityManager()->CreateEntity({ "TestComponent" });

	m_engineInstance->SetScene(scene);
}

void XEngine::Cleanup()
{
	if (m_scene)
	{
		m_scene->DisableScene();
		delete m_scene;
	}

	for (int32_t i = 0; i < m_maxECSThreads; ++i)
	{
		m_ecsThreads[i]->join();
		delete m_ecsThreads[i];
	}

	for (auto kp : m_hwInterfaces)
	{
		if (kp.second && kp.second->GetStatus(kp.first) == HardwareStatus::Initialized)
		{
			LogMessage("Cleaning up " + interfaceToName[kp.first] + " interface of " + kp.second->GetName(), LogMessageType::Message);
			kp.second->Destroy(kp.first);
		}
	}
}

void XEngine::RunECSThread(int32_t index)
{
	while (m_running)
	{
		if (m_ecsQueued > 0)
		{
			--m_ecsQueued;
			m_sysManager->ExecuteJobs(index, m_ecsDt);
		}
		else DoIdleWork();
	}
}
