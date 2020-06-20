#pragma once
#include <string>
#include <vector>
#include <map>
#include <typeinfo>
#include <set>

#include "Component.h"
#include "Entity.h"

using EventId = UniqueId;

enum class PostOrdering
{
	First, Last, Anywhere
};

class SubsystemManager;
class ISystem
{
public:
	virtual void Initialize() { }
	virtual void Destroy() { }

	virtual std::string GetName() = 0;

	virtual int GetMaxPostThreadCount() { return 0; }
	virtual bool IsPostMainThread() { return false; }

	virtual std::vector<std::string> GetSystemsBefore() { return {}; }
	virtual std::vector<std::string> GetSystemsAfter() { return {}; }
	virtual std::vector<std::string> GetRequiredSystems() { return {}; }
	virtual std::vector<std::string> GetComponentTypes() = 0;
	virtual std::vector<std::string> GetReadOnlyComponentTypes() { return {}; }

	virtual void Update(float deltaTime, ComponentDataIterator& data) { }
	virtual void AfterEntityUpdate(float deltaTime) {}
	virtual void PostUpdate(float deltaTime, int threadIndex) { }
	virtual void Dispose(ComponentDataIterator& data) { }

	inline bool IsEnabled() { return m_enabled; }
	inline void SetEnabled(bool enabled) { m_enabled = enabled; }
	inline SubsystemManager *GetManager() { return m_manager; }
	inline void SetSubsystemManager(SubsystemManager *manager) { m_manager = manager; }

	UniqueId __filteringGroup;
	std::atomic_int __jobsLeft;
private:
	bool m_enabled = false;
	SubsystemManager *m_manager;
};

class Scene;
class SystemManager
{
public:
	SystemManager(Scene *scene) : m_scene(scene) {}
	void AddSystem(std::string name);
	std::vector<ISystem *>& GetSystems() { return m_systems; }
	Scene *GetScene() { return m_scene; }
private:
	Scene *m_scene;
	std::vector<ISystem *> m_systems;
};

class SystemGraphSorter;
class SubsystemManager
{
public:
	XENGINEAPI SubsystemManager();
	XENGINEAPI ~SubsystemManager();
	XENGINEAPI ISystem *GetSystem(std::string name);

	XENGINEAPI void SetSceneManager(SystemManager *manager);

	XENGINEAPI void AddSystem(std::string name);

	XENGINEAPI void InitializeSystemOrdering(); // Run when the scene's collection of systems changes
	XENGINEAPI void ScheduleJobs(); // Run every frame from one thread
	XENGINEAPI void ExecuteJobs(int threadIndex, float deltaTime); // Run from every thread
private:
	std::vector<ISystem *> m_mainThreadSystems; // PostUpdate to be run only from main thread
	std::vector<ISystem *> m_nonMainThreadSystems;

	SystemGraphSorter *m_systemGraph = nullptr;
	SystemManager *m_sceneManager = nullptr;

	std::vector<ISystem *> m_systems;
};