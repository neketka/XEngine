#pragma once
#include <string>
#include <vector>
#include <map>
#include <typeinfo>
#include <set>

#include "System.h"
#include "Component.h"
#include "Entity.h"

#include "ChunkAllocator.h"
#include "UUID.h"

class InternalTypeInfo
{
public:
	std::string Name;
	int Size;
	UniqueId Identifier;
	bool Buffered;
	int ComponentOffset;
	int BufferOffset;
};

class ECSRegistrar
{
public:
	XENGINEAPI ~ECSRegistrar();
	XENGINEAPI void AddSystem(ISystem *system);
	XENGINEAPI ISystem *GetSystem(std::string name);
	XENGINEAPI UniqueId GetEventId(std::string name);
	template<class T>
	void RegisterComponent()
	{
		UniqueId id = GenerateID();
		InternalTypeInfo info;
		info.Name = StaticComponentInfo<T>::GetName();
		info.Size = StaticComponentInfo<T>::GetSize();
		info.Identifier = StaticComponentInfo<T>::GetIdentifier();
		info.Buffered = StaticComponentInfo<T>::IsBuffered();
		info.ComponentOffset = StaticComponentInfo<T>::GetComponentPointerOffset();
		info.BufferOffset = StaticComponentInfo<T>::GetBufferPointerOffset();
		m_components[id] = info;
		m_componentsByName[info.Name] = id;
	}
	XENGINEAPI UniqueId GetComponentIdByName(std::string name);
	XENGINEAPI std::string GetComponentName(UniqueId id);
	XENGINEAPI int GetComponentSize(UniqueId id);
	XENGINEAPI int GetComponentPointerOffset(UniqueId id);
	XENGINEAPI int GetBufferPointerOffset(UniqueId id);
	XENGINEAPI bool IsComponentBuffered(UniqueId id);
	inline std::map<UniqueId, InternalTypeInfo>& GetComponentMap() { return m_components; }
private:

	std::map<std::string, UniqueId> m_events;
	std::map<UniqueId, InternalTypeInfo> m_components;
	std::map<std::string, UniqueId> m_componentsByName;
	std::map<std::string, ISystem *> m_systems;
};

class Scene
{
public:
	XENGINEAPI Scene();
	XENGINEAPI ~Scene();

	XENGINEAPI void EnableScene();
	XENGINEAPI void DisableScene();

	inline SystemManager *GetSystemManager() { return m_sysManager; }
	inline ComponentManager *GetComponentManager() { return m_compManager; }
	inline EntityManager *GetEntityManager() { return m_entManager; }
private:
	bool m_enabled = false;
	SystemManager *m_sysManager;
	ComponentManager *m_compManager;
	EntityManager *m_entManager;
};