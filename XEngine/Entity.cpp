#include "pch.h"

void Entity::Kill()
{
	m_manager->DestroyEntity(m_id);
}

std::vector<Component *> Entity::GetComponents()
{
	return m_manager->GetEntityComponents(m_id);
}

void Entity::AddComponent(ComponentTypeId id)
{
	m_manager->AddComponentToEntity(m_id, id);
}

void Entity::RemoveComponent(ComponentTypeId id)
{
	m_manager->RemoveComponentFromEntity()
}

Component *Entity::GetComponent(ComponentTypeId id)
{
	return m_manager->GetEntityComponent(m_id, id);
}

EntityManager::EntityManager(Scene *scene)
{
	m_scene = scene;
}

Entity EntityManager::CreateEntity(std::vector<std::string> components)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	std::set<ComponentTypeId> compIds;
	for (std::string comp : components)
	{
		compIds.emplace(registrar->GetComponentIdByName(comp));
	}
	return Entity(m_scene->GetComponentManager()->AllocateComponentGroup(compIds), this);
}

void EntityManager::DestroyEntity(EntityId id)
{

}

Entity EntityManager::GetEntityByComponent(EntityId id, ComponentTypeId componentId)
{
	return Entity();
}

void EntityManager::AddComponentToEntity(EntityId id, ComponentTypeId componentId)
{
}

void EntityManager::RemoveComponentFromEntity(EntityId id, ComponentTypeId componentId)
{
}

std::vector<Entity> EntityManager::GetEntitiesByComponent(ComponentTypeId componentId)
{
	return std::vector<Entity>();
}

std::vector<Component*> EntityManager::GetEntityComponents(EntityId id)
{
	return std::vector<Component*>();
}

Component * EntityManager::GetEntityComponent(EntityId id, ComponentTypeId componentId)
{
	return nullptr;
}
