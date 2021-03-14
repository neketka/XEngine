#pragma once

#include <string>
#include <vector>
#include <map>
#include <typeinfo>

#include "ECS.h"
#include "HardwareInterfaces.h"
#include "WorkerManager.h"
#include "AssetManager.h"

enum class LogMessageType
{
	Message, Warning, Error, Internal
};

class XEngine
{
public:
	XENGINEAPI XEngine();
	XENGINEAPI ~XEngine();

	XENGINEAPI void Run();
	XENGINEAPI void Shutdown();
	XENGINEAPI bool IsRunning();

	XENGINEAPI void AddPropertySet(std::map<std::string, int32_t> properties);
	XENGINEAPI void AddPropertySet(std::map<std::string, std::string> properties);
	XENGINEAPI void SetProperty(std::string key, int32_t value);
	XENGINEAPI void SetProperty(std::string key, std::string value);
	XENGINEAPI std::string *GetStringProperty(std::string key);
	XENGINEAPI int32_t *GetIntProperty(std::string key);

	XENGINEAPI void SaveProperties(); 
	XENGINEAPI void LoadProperties();

	XENGINEAPI std::string& GetRootPath();

	XENGINEAPI UniqueId& GetInstanceId();

	XENGINEAPI float GetTime(UniqueId id);
	XENGINEAPI void SetGlobalTimeScale(float scale);
	XENGINEAPI void SetLocalTimeScale(UniqueId id, float scale);
	XENGINEAPI float AdjustDeltaTime(UniqueId id, float deltaTime);

	XENGINEAPI void AddBeginMarker(std::string label);
	XENGINEAPI void AddEndMarker(std::string label);

	XENGINEAPI void LogMessage(std::string message, LogMessageType type);
	XENGINEAPI void RaiseCriticalError(std::string error);

	XENGINEAPI void SetRootPath(std::string rootPath);

	XENGINEAPI void SetScene(Scene *scene);
	XENGINEAPI Scene *GetScene();
	XENGINEAPI SubsystemManager *GetSubsystemManager();
	XENGINEAPI ECSRegistrar *GetECSRegistrar();
	XENGINEAPI AssetManager *GetAssetManager();

	XENGINEAPI static void InitializeEngine(std::string name, int32_t threadCount, bool defaultSystems = true, std::string rootPath = "");
	XENGINEAPI static XEngine& GetInstance();

	XENGINEAPI void DoIdleWork();

	XENGINEAPI void AddInterface(HardwareInterface *interface, HardwareInterfaceType type); // Set the interface based on user input

	template<class T> 
	T *GetInterface(HardwareInterfaceType type)
	{
		return dynamic_cast<T *>(m_hwInterfaces[type]);
	}

	void SetMaxFPS(int32_t fps) { m_maxFps = fps; }
	void SetFPSAverageInterval(int32_t interval) { m_fpsAvgInterval = interval; }

	int32_t GetMaxFPS() { return m_maxFps; }
	int32_t GetAverageFrametime() { return m_frameTimeAvg; }
	int32_t GetFPSAverageInterval() { return m_fpsAvgInterval; }
	int32_t GetAverageFramerate() { return m_fps; }

	std::string GetName() { return m_name; }

private:
	void Tick(float deltaTime);
	void Init();
	void Cleanup();

	UniqueId m_engineInstanceId;
	
	void RunECSThread(int32_t index);
	std::thread **m_ecsThreads;
	std::atomic_int m_ecsQueued;
	float m_ecsDt;

	static XEngine *m_engineInstance;
	std::string m_rootPath;
	std::string m_name;

	int32_t m_maxECSThreads;

	int32_t m_maxFps = 240;
	int32_t m_fps = 0;
	float m_frameTimeAvg = 0;
	int32_t m_fpsAvgInterval = 10;

	bool m_running = false;

	ECSRegistrar *m_ecsRegistrar = nullptr;
	Scene *m_scene;
	SubsystemManager *m_sysManager;
	AssetManager *m_assetManager;

	std::map<UniqueId, glm::ivec2> m_timeAndScale;
	float m_globalTimeScale = 1.f;
	float m_time = 0.f;

	std::map<std::string, int32_t> m_intProps;
	std::map<std::string, std::string> m_stringProps;

	std::chrono::time_point<std::chrono::steady_clock> m_beginTime;

	std::map<HardwareInterfaceType, HardwareInterface *> m_hwInterfaces;
	std::map<HardwareInterfaceType, bool> m_excludeFromFrame;
};

XENGINEAPI extern XEngine *XEngineInstance;