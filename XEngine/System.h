#pragma once
#include <string>
#include <vector>
#include <map>
#include <typeinfo>
#include <set>

#include "Component.h"
#include "Entity.h"

enum class SystemOrdering
{
	First, Any, Last
};

enum class SystemThreading
{
	EntityMainPostMain, EntityMultiPostMain, EntitySinglePostMain,
	EntityMainPostMulti, EntityMultiPostMulti, EntitySinglePostMulti
};

class ISystem
{
public:
	virtual void Initialize() { }
	virtual void Destroy() { }

	virtual std::string GetName() = 0;

	virtual SystemThreading GetThreadingOptions() { return SystemThreading::EntityMultiPostMulti; }
	virtual SystemOrdering GetSystemOrdering() { return SystemOrdering::Any; }
	virtual int GetMaxPostThreadCount() { return 1; }

	virtual std::vector<std::string> GetSystemsBefore() { return {}; }
	virtual std::vector<std::string> GetSystemsAfter() { return {}; }
	virtual std::vector<std::string> GetRequiredSystems() { return {}; }
	virtual std::vector<std::string> GetComponentTypes() = 0;

	virtual void Update(float deltaTime, ComponentDataIterator& data) { }
	virtual void PostUpdate(float deltaTime, int threadIndex) { }
	virtual void ProcessEvent(float deltaTime, ComponentDataIterator& data, UniqueId eventId) { }
	virtual void Dispose(ComponentDataIterator& data) { }

	inline bool IsEnabled() { return m_enabled; }
	inline void SetEnabled(bool enabled) { m_enabled = enabled; }
	inline SubsystemManager *GetManager() { return m_manager; }
	inline void SetSubsystemManager(SubsystemManager *manager) { m_manager = manager; }
private:
	bool m_enabled = false;
	SubsystemManager *m_manager;
};

class Scene;
class SystemManager
{
public:
	XENGINEAPI SystemManager(Scene *scene);
	XENGINEAPI ~SystemManager();
	XENGINEAPI void AddSystem(std::string name);
	XENGINEAPI std::vector<ISystem *> GetEnabledSystems();
private:
	std::vector<ISystem *> m_systems;
};

class SubsystemManager
{
public:
	XENGINEAPI SubsystemManager();
	XENGINEAPI ~SubsystemManager();
	XENGINEAPI ISystem *GetSystem(std::string name);

	XENGINEAPI void SetSceneManager(SystemManager *manager);
	XENGINEAPI void RaiseEvent(std::vector<Entity> e, UniqueId eventId);

	XENGINEAPI void AddSystem(std::string name);
	XENGINEAPI void SetThreadsAvailable(int threads);
	XENGINEAPI void ScheduleJobs();
	XENGINEAPI void ExecuteJobs(int threadIndex, float deltaTime);
	XENGINEAPI void StallTillDone();
private:
	std::vector<ISystem *> m_systems;
};