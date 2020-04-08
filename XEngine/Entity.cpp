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
	m_manager->RemoveComponentFromEntity(m_id, id);
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
		compIds.emplace(registrar->GetComponentIdByName(comp)); // Add component id of component to 
	}
	return Entity(m_scene->GetComponentManager()->AllocateComponentGroup(compIds), this);
}

Entity EntityManager::CreateEntity(std::set<ComponentTypeId> components)
{
	return Entity(m_scene->GetComponentManager()->AllocateComponentGroup(components), this);
}

void EntityManager::DestroyEntity(EntityId id)
{
	m_scene->GetComponentManager()->DeleteComponentGroup(id);
}

void EntityManager::AddComponentToEntity(EntityId id, ComponentTypeId componentId)
{
	std::vector<ComponentTypeId>& types = m_scene->GetComponentManager()->GetComponentIdsFromComponentGroup(componentId);
	std::set<ComponentTypeId> typeSet(types.begin(), types.end());
	typeSet.emplace(id); // Add component id to the other components
	m_scene->GetComponentManager()->RebuildComponentGroup(id, typeSet);
}

void EntityManager::RemoveComponentFromEntity(EntityId id, ComponentTypeId componentId)
{
	std::vector<ComponentTypeId>& types = m_scene->GetComponentManager()->GetComponentIdsFromComponentGroup(componentId);
	std::set<ComponentTypeId> typeSet(types.begin(), types.end());
	typeSet.erase(id); // Remove component id from components
	m_scene->GetComponentManager()->RebuildComponentGroup(id, typeSet);
}

std::vector<Component *> EntityManager::GetEntityComponents(EntityId id)
{
	return m_scene->GetComponentManager()->GetComponentGroupData(id);
}

Component *EntityManager::GetEntityComponent(EntityId id, ComponentTypeId componentId)
{
	return m_scene->GetComponentManager()->GetComponentGroupData(id, componentId);
}

Scene *EntityManager::GetScene()
{
	return m_scene;
}

Entity EntityManager::GetEntityByComponent(EntityId id, ComponentTypeId componentId)
{
	return Entity(m_scene->GetComponentManager()->GetFirstComponentGroupWithType(componentId), this); 
}

std::vector<Entity> EntityManager::GetEntitiesByComponent(ComponentTypeId componentId)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	UniqueId group = m_scene->GetComponentManager()->AddFilteringGroup({ componentId });
	int pointerOffset = registrar->GetComponentPointerOffset(componentId); // Get polymorphic pointer conversion offset
	std::vector<Entity> ents;
	std::vector<ComponentDataIterator> *compIterators = m_scene->GetComponentManager()->GetFilteringGroup(group, false); // Get all iterators of the filtering group containing only that component type
	for (ComponentDataIterator iter : *compIterators)
	{
		for (void **dat = iter.Next<void *>(); dat; dat = iter.Next<void *>()) // Iterator though the components
		{
			EntityId id = m_scene->GetComponentManager()->Upcast<Component>(dat, pointerOffset)->EntityID;
			ents.push_back(Entity(id, this));
		}
	}
	
	return ents;
}