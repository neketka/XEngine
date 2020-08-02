#include "pch.h"
#include <algorithm>
#include "SystemGraphSorter.h"

SubsystemManager::SubsystemManager()
{
}

SubsystemManager::~SubsystemManager()
{
	for (auto sys : m_systems)
	{
		sys->Destroy();
	}
}

ISystem *SubsystemManager::GetSystem(std::string name)
{
	return XEngine::GetInstance().GetECSRegistrar()->GetSystem(name);
}

void SubsystemManager::SetSceneManager(SystemManager *manager)
{
	m_sceneManager = manager;
	InitializeSystemOrdering();
}

void SubsystemManager::AddSystem(std::string name)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	m_systems.push_back(registrar->GetSystem(name));
	m_systems.back()->Initialize();
}

void SubsystemManager::InitializeSystemOrdering()
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();

	if (m_systemGraph)
		delete m_systemGraph;
	std::vector<ISystem *> systems(m_systems);

	std::vector<ISystem *>& sceneSystems = m_sceneManager->GetSystems();
	systems.insert(systems.begin(), sceneSystems.begin(), sceneSystems.end());

	m_systemGraph = new SystemGraphSorter(m_sceneManager->GetScene()->GetComponentManager(), systems);

	for (ISystem *system : systems) // Insert the PostUpdate systems correctly
	{
		std::vector<ComponentTypeId> compIds;
		for (std::string comp : system->GetComponentTypes()) // Add all component type ids to this vector
			compIds.push_back(registrar->GetComponentIdByName(comp));

		system->__filteringGroup = m_sceneManager->GetScene()->GetComponentManager()->AddFilteringGroup(compIds); // Find filtering group
		if (system->IsPostMainThread()) 
			m_mainThreadSystems.push_back(system);
		else
			m_nonMainThreadSystems.push_back(system);
	}
}

void SubsystemManager::ScheduleJobs()
{
	m_systemGraph->QueueLayers();
	for (ISystem *system : m_nonMainThreadSystems)
	{
		system->__jobsLeft = system->GetMaxPostThreadCount();
	}
}

void SubsystemManager::ExecuteJobs(int32_t threadIndex, float deltaTime)
{
	m_systemGraph->SetDeltaTime(deltaTime); // Presumably, all the deltaTime is the same across all threads

	m_systemGraph->RunFromThread(threadIndex == 0);

	if (threadIndex == 0)
	{
		for (ISystem *system : m_mainThreadSystems)
		{
			system->PostUpdate(deltaTime, 0);
		}
	}
	
	for (ISystem *system : m_nonMainThreadSystems)
	{
		int32_t jobIndex = --system->__jobsLeft;
		while (jobIndex >= 0)
		{
			system->PostUpdate(deltaTime, jobIndex);
			jobIndex = --system->__jobsLeft;
		}
	}
}

void SystemManager::AddSystem(std::string name)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	m_systems.push_back(registrar->GetSystem(name));
}
