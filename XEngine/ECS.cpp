#include "pch.h"
#include "ECS.h"

ECSRegistrar::~ECSRegistrar()
{
	for (auto sys : m_systems)
	{
		delete sys.second;
	}
}

void ECSRegistrar::AddSystem(ISystem *system)
{
	m_systems[system->GetName()] = system;
}

ISystem *ECSRegistrar::GetSystem(std::string name)
{
	return m_systems[name];
}

UniqueId ECSRegistrar::GetEventId(std::string name)
{
	UniqueId id;
	auto itr = m_events.find(name);
	if (itr == m_events.end()) // Does the current event id exists
	{
		id = GenerateID(); // Create a new event id
		m_events[name] = id; // Store the new event by name
	}
	else
		id = itr->second;
	return id;
}

UniqueId ECSRegistrar::GetComponentIdByName(std::string name)
{
	return m_componentsByName[name];
}

std::string ECSRegistrar::GetComponentName(UniqueId id)
{
	return m_components[id].Name;
}

int ECSRegistrar::GetComponentSize(UniqueId id)
{
	return m_components[id].Size;
}

int ECSRegistrar::GetComponentPointerOffset(UniqueId id)
{
	return m_components[id].ComponentOffset;
}

int ECSRegistrar::GetBufferPointerOffset(UniqueId id)
{
	return m_components[id].BufferOffset;
}

bool ECSRegistrar::IsBufferedComponented(UniqueId id)
{
	return m_components[id].Buffered;
}

Scene::Scene(std::string name) : m_name(name)
{
	m_sysManager = new SystemManager(this);
	m_compManager = new ComponentManager(this);
	m_entManager = new EntityManager(this);
}

Scene::~Scene()
{
	delete m_sysManager;
	delete m_compManager;
	delete m_entManager;
}

void Scene::EnableScene()
{
}

void Scene::DisableScene()
{
}

std::string Scene::GetName()
{
	return m_name;
}
