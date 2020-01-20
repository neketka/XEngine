#pragma once

#include <string>
#include <vector>
#include <map>
#include <typeinfo>
#include <set>
#include <concurrent_unordered_map.h>


class EntityManager;
class Entity
{
public:
	Entity(UniqueId id, EntityManager *manager) : m_id(id), m_manager(manager) {}
	XENGINEAPI void Kill();
	XENGINEAPI std::vector<Component *>& GetComponents();
	template<class T>
	T *GetComponent()
	{
		return dynamic_cast<T *>(m_manager->GetEntityComponent(m_id, StaticComponentInfo<T>::GetIdentifier()));
	}
	template<class T>
	void AddComponent()
	{
		m_manager->AddComponentToEntity(m_id, StaticComponentInfo<T>::GetIdentifier());
	}
	template<class T>
	void RemoveComponent()
	{
		m_manager->RemoveComponentFromEntity(m_id, StaticComponentInfo<T>::GetIdentifier());
	}
	inline UniqueId GetId() { return m_id; }
private:
	UniqueId m_id;
	EntityManager *m_manager;
};

class Scene;
class EntityManager
{
public:
	XENGINEAPI EntityManager(Scene *scene);
	XENGINEAPI Entity CreateEntity(std::vector<std::string> components);
	XENGINEAPI void DestroyEntity(UniqueId id);
	XENGINEAPI Entity GetEntityByComponent(UniqueId id, UniqueId componentId);
	XENGINEAPI void AddComponentToEntity(UniqueId id, UniqueId componentId);
	XENGINEAPI void RemoveComponentFromEntity(UniqueId componentId);
	XENGINEAPI std::vector<Entity> GetEntitiesByComponent(UniqueId componentId);
	XENGINEAPI std::vector<Component *> GetEntityComponents(UniqueId id);
private:
	concurrency::concurrent_unordered_map<UniqueId, UniqueId> m_entityComponentGroup;
};