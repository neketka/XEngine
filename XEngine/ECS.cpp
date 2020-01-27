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
	if (itr == m_events.end())
	{
		id = GenerateID();
		m_events[name] = id;
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

bool ECSRegistrar::IsComponentBuffered(UniqueId id)
{
	return m_components[id].Buffered;
}

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::EnableScene()
{
}

void Scene::DisableScene()
{
}
