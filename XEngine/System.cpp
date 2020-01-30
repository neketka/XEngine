#include "pch.h"
#include <algorithm>

SubsystemManager::SubsystemManager()
{
}

SubsystemManager::~SubsystemManager()
{
}

ISystem *SubsystemManager::GetSystem(std::string name)
{
	return XEngine::GetInstance().GetECSRegistrar()->GetSystem(name);
}

void SubsystemManager::SetSceneManager(SystemManager *manager)
{
	m_sceneManager = manager;
}

void SubsystemManager::RaiseEvent(Entity e, EventId eventId)
{
	std::vector<ComponentTypeId> compTypes;
	for (Component *ptr : e.GetComponents())
		compTypes.push_back(ptr->ComponentTypeID);
	e.GetManager()->GetScene()->GetComponentManager()->AddFilteringGroup(compTypes);
	m_raisedEvents.insert(e.GetId(), eventId);
}

void SubsystemManager::AddSystem(std::string name)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	m_systems.push_back(registrar->GetSystem(name));
}

void SubsystemManager::SetThreadsAvailable(int threads)
{
	m_maxThreads = threads;
}

void SubsystemManager::InitializeSystemOrdering()
{
}

void SubsystemManager::ScheduleJobs()
{
}

void SubsystemManager::ExecuteJobs(int threadIndex, float deltaTime)
{
}

void SubsystemManager::StallTillDone()
{
}
