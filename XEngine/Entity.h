#pragma once

#include <string>
#include <vector>
#include <map>
#include <typeinfo>
#include <set>
#include <concurrent_unordered_map.h>

using EntityId = UniqueId;
class EntityManager;
class Entity
{
public:
	Entity(EntityId id, EntityManager *manager) : m_id(id), m_manager(manager) {}
	XENGINEAPI void Kill();
	XENGINEAPI std::vector<Component *> GetComponents();
	template<class T>
	T *GetComponent()
	{
		return dynamic_cast<T *>(GetComponent(StaticComponentInfo<T>::GetIdentifier()));
	}
	template<class T>
	void AddComponent()
	{
		AddComponent(StaticComponentInfo<T>::GetIdentifier());
	}
	template<class T>
	void RemoveComponent()
	{
		RemoveComponent(StaticComponentInfo<T>::GetIdentifier());
	}
	inline UniqueId GetId() { return m_id; }
private:
	void AddComponent(ComponentTypeId id);
	void RemoveComponent(ComponentTypeId id);
	Component *GetComponent(ComponentTypeId id);
	
	EntityId m_id;
	EntityManager *m_manager;
};

class Scene;
class EntityManager
{
public:
	XENGINEAPI EntityManager(Scene *scene);
	XENGINEAPI Entity CreateEntity(std::vector<std::string> components);
	XENGINEAPI void DestroyEntity(EntityId id);
	XENGINEAPI Entity GetEntityByComponent(EntityId id, ComponentTypeId componentId);
	XENGINEAPI void AddComponentToEntity(EntityId id, ComponentTypeId componentId);
	XENGINEAPI void RemoveComponentFromEntity(EntityId id, ComponentTypeId componentId);
	XENGINEAPI std::vector<Entity> GetEntitiesByComponent(ComponentTypeId componentId);
	XENGINEAPI std::vector<Component *> GetEntityComponents(EntityId id);
	XENGINEAPI Component *GetEntityComponent(EntityId id, ComponentTypeId componentId);
private:
	Scene *m_scene;
};