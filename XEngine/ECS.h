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
	int32_t Size;
	UniqueId Identifier;
	int32_t ComponentOffset;
	int32_t BufferOffset;
	bool Buffered;
};

class ECSRegistrar
{
public:
	ECSRegistrar() {
		RegisterComponent<EntityIdComponent>();
	}
	XENGINEAPI ~ECSRegistrar();
	XENGINEAPI void AddSystem(ISystem *system);
	XENGINEAPI void AddSystems(std::vector<ISystem *> systems);
	XENGINEAPI ISystem *GetSystem(std::string name);
	XENGINEAPI UniqueId GetEventId(std::string name);
	template<class T>
	void RegisterComponent()
	{
		InternalTypeInfo info;
		info.Name = StaticComponentInfo<T>::GetName();
		info.Size = StaticComponentInfo<T>::GetSize();
		info.Identifier = StaticComponentInfo<T>::GetIdentifier();
		info.Buffered = StaticComponentInfo<T>::IsBuffered(); // Has a buffer
		info.ComponentOffset = StaticComponentInfo<T>::GetComponentPointerOffset();
		if (info.Buffered)
			info.BufferOffset = BufferedComponentInfo<T>::GetBufferPointerOffset();
		m_components[info.Identifier] = info;
		m_componentsByName[info.Name] = info.Identifier;
	}
	template<class T, class ...TArgs>
	void RegisterComponents() { RegisterComponent<T>(); RegisterComponents<TArgs>(); }

	XENGINEAPI UniqueId GetComponentIdByName(std::string name);
	XENGINEAPI std::string GetComponentName(UniqueId id);
	XENGINEAPI int32_t GetComponentSize(UniqueId id);
	XENGINEAPI int32_t GetComponentPointerOffset(UniqueId id);
	XENGINEAPI int32_t GetBufferPointerOffset(UniqueId id);
	XENGINEAPI bool IsComponentBuffered(UniqueId id);
	XENGINEAPI void InitSystems();
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
	XENGINEAPI Scene(std::string name);
	XENGINEAPI ~Scene();

	XENGINEAPI void EnableScene();
	XENGINEAPI void DisableScene();

	XENGINEAPI std::string GetName();

	inline SystemManager *GetSystemManager() { return m_sysManager; }
	inline ComponentManager *GetComponentManager() { return m_compManager; }
	inline EntityManager *GetEntityManager() { return m_entManager; }
private:
	std::string m_name;
	bool m_enabled = false;
	SystemManager *m_sysManager;
	ComponentManager *m_compManager;
	EntityManager *m_entManager;
};