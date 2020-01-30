#include "pch.h"
#include <algorithm>

ComponentManager::ComponentManager(Scene *scene)
{
	m_componentChunkSize = 32;
	m_componentDisposedChunkSize = 8;
	m_scene = scene;
}

ComponentManager::~ComponentManager()
{
	for (auto pair : m_componentGroupTypes)
	{
		delete pair.second;
	}
}

void ComponentManager::InitializeFilteringGroups()
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	for (auto comp : registrar->GetComponentMap())
	{
		AddFilteringGroup({ comp.first });
	}
}

FilteringGroupId ComponentManager::AddFilteringGroup(std::vector<ComponentTypeId> components)
{
	auto iter = m_filteringToId.find(components);
	if (iter == m_filteringToId.end())
	{
		ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
		std::set<UniqueId> setComp(components.begin(), components.end());
		auto cgIter = m_filteringCompsToInternalFiltering.find(setComp);

		UniqueId internalId;
		if (cgIter == m_filteringCompsToInternalFiltering.end())
		{
			internalId = m_filteringCompsToInternalFiltering[setComp] = GenerateID();
			auto& vec = m_internalFilteringIdToComponentGroup[internalId] = concurrency::concurrent_vector<ComponentGroupType *>();
			for (auto compGroup : m_componentGroupTypes)
			{
				if (std::includes(compGroup.first.begin(), compGroup.first.end(), setComp.begin(), setComp.end()))
				{
					vec.push_back(compGroup.second);
				}
			}
		}
		else internalId = cgIter->second;

		UniqueId id = m_filteringToId[components] = GenerateID();
		m_idToFiltering[id] = components;
		m_filteringIdToInternalId[id] = cgIter->second;
		return id;
	}
	return iter->second;
}

std::vector<ComponentDataIterator> *ComponentManager::GetFilteringGroup(FilteringGroupId filteringGroup, bool disposed)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	
	std::vector<ComponentDataIterator> *filtering = new std::vector<ComponentDataIterator>;
	std::vector<UniqueId>& order = m_idToFiltering[filteringGroup];
	auto& compTypes = m_internalFilteringIdToComponentGroup[m_filteringIdToInternalId[filteringGroup]];
	for (ComponentGroupType *type : compTypes)
	{
		int chunkCount = type->Allocators[0].GetAllChunks().size();
		for (int chunk = 0; chunk < chunkCount; ++chunk)
		{
			std::vector<void *> compBlocks(type->ComponentTypes.size());
			std::vector<int> sizes(type->ComponentTypes.size());

			for (int comp = 0; comp < type->ComponentTypes.size(); ++comp)
			{
				auto allocator = (disposed ? type->CompTypeToDisposedAllocator : type->CompTypeToAllocator)[order[comp]];
				compBlocks[comp] = allocator->GetAllChunks()[chunk].Memory;
				sizes[comp] = allocator->GetPerObjectSize();
			}

			int count = type->Allocators[0].GetAllChunks()[0].ObjectCount;
			filtering->push_back(ComponentDataIterator(sizes, compBlocks, 0, count));
		}
	}

	return filtering;
}

ComponentGroupId ComponentManager::GetFirstComponentGroupWithType(ComponentTypeId id)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	UniqueId filtering = m_filteringCompsToInternalFiltering[{id}];
	auto groups = m_internalFilteringIdToComponentGroup[filtering];
	for (ComponentGroupType *type : groups)
	{
		if (type->Allocators[0].GetAllChunks().size() != 0)
		{
			Component *comp = Upcast<Component>(type->Allocators[0].GetAllChunks()[0].Memory,
				registrar->GetComponentPointerOffset(type->ComponentTypes[0]));
			return comp->EntityID;
		}
	}
	return 0;
}

ComponentGroupId ComponentManager::AllocateComponentGroup(std::set<ComponentTypeId> components)
{
	ComponentGroupId id = GenerateID();

	AllocCompGroup(components, false, id);

	return id;
}

ComponentGroupType *ComponentManager::GetComponentGroupType(std::set<ComponentTypeId> components)
{
	auto iter = m_componentGroupTypes.find(components);
	if (iter == m_componentGroupTypes.end())
	{
		ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();

		ComponentGroupType *type = m_componentGroupTypes[components] = new ComponentGroupType;
		type->ChunkSize = m_componentChunkSize;
		type->ComponentTypes = std::vector<UniqueId>(components.begin(), components.end());
		type->Allocators.reserve(type->ComponentTypes.size());
		type->DisposedAllocators.reserve(type->ComponentTypes.size());
		for (UniqueId id : type->ComponentTypes)
		{
			int size = registrar->GetComponentSize(id);
			type->Allocators.push_back(MemoryChunkAllocator(type->ChunkSize, size));
			type->DisposedAllocators.push_back(MemoryChunkAllocator(m_componentDisposedChunkSize, size));
			type->CompTypeToAllocator[id] = &type->Allocators.back();
			type->CompTypeToDisposedAllocator[id] = &type->DisposedAllocators.back();
		}
		for (auto pair : m_filteringCompsToInternalFiltering)
		{
			if (std::includes(components.begin(), components.end(), pair.first.begin(), pair.first.end()))
			{
				m_internalFilteringIdToComponentGroup[pair.second].push_back(type);
			}
		}
		return type;
	}
	return iter->second;
}

void ComponentManager::DeleteComponentGroup(ComponentGroupId id)
{
	m_moveToDisposed.insert(id);
}

void ComponentManager::CopyComponentData(ComponentGroupId dest, ComponentGroupId src, ComponentTypeId compId)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();

	auto destPair = m_componentGroups[dest];
	auto srcPair = m_componentGroups[src];

	std::memcpy(destPair.second->CompTypeToAllocator[compId]->GetObjectMemory(destPair.first),
		srcPair.second->CompTypeToAllocator[compId]->GetObjectMemory(srcPair.first), registrar->GetComponentSize(compId));
}

std::vector<UniqueId>& ComponentManager::GetComponentIdsFromComponentGroup(ComponentGroupId componentGroup)
{
	return m_componentGroups[componentGroup].second->ComponentTypes;
}

std::vector<Component *> ComponentManager::GetComponentGroupData(ComponentGroupId componentGroup)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();

	auto pair = m_componentGroups[componentGroup];
	std::vector<Component *> data(pair.second->ComponentTypes.size());

	for (int i = 0; i < pair.second->ComponentTypes.size(); ++i)
	{
		data[i] = Upcast<Component>(pair.second->Allocators[i].GetObjectMemory(pair.first), 
			registrar->GetComponentPointerOffset(pair.second->ComponentTypes[i]));
	}

	return data;
}

Component *ComponentManager::GetComponentGroupData(ComponentGroupId componentGroup, ComponentTypeId id)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	Component *ret = nullptr;

	auto pair = m_componentGroups[componentGroup];
	auto copiedPair = m_componentGroups[componentGroup];
	if (std::find(pair.second->ComponentTypes.begin(), pair.second->ComponentTypes.end(), id) != pair.second->ComponentTypes.end())
		ret = Upcast<Component>(pair.second->CompTypeToAllocator[id]->GetObjectMemory(pair.first),
			registrar->GetComponentPointerOffset(id));
	else if (std::find(copiedPair.second->ComponentTypes.begin(), copiedPair.second->ComponentTypes.end(), id) != copiedPair.second->ComponentTypes.end())
		ret = Upcast<Component>(copiedPair.second->CompTypeToAllocator[id]->GetObjectMemory(copiedPair.first),
			registrar->GetComponentPointerOffset(id));

	return ret;
}

void ComponentManager::RebuildComponentGroup(ComponentGroupId componentGroup, std::set<ComponentTypeId> components)
{
	AllocCompGroup(components, true, componentGroup);
}

void ComponentManager::ExecuteSingleThreadOps()
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	for (auto& pair : m_copiedComponentGroups) 
	{
		auto originalPair = m_componentGroups[pair.first];
		for (ComponentTypeId comp : pair.second.second->ComponentTypes)
		{
			std::memcpy(pair.second.second->CompTypeToAllocator[comp]->GetObjectMemory(pair.second.first),
				originalPair.second->CompTypeToAllocator[comp]->GetObjectMemory(originalPair.first), registrar->GetComponentSize(comp));
			originalPair.second->CompTypeToAllocator[comp]->FreeObject(originalPair.first);
		}
		m_componentGroups[pair.first] = pair.second;
	}
	m_copiedComponentGroups.clear();
	for (UniqueId id : m_disposed)
	{
		auto pair = m_componentGroups[id];
		for (int i = 0; i < pair.second->ComponentTypes.size(); ++i)
		{
			if (registrar->IsComponentBuffered(pair.second->ComponentTypes[i]))
				Upcast<ComponentBuffer>(pair.second->DisposedAllocators[i].GetObjectMemory(pair.first),
					registrar->GetBufferPointerOffset(pair.second->ComponentTypes[i]))->DestroyBufferStore();
			pair.second->DisposedAllocators[i].FreeObject(pair.first);
		}
		m_componentGroups.unsafe_erase(id);
	}
	m_disposed.clear();
	for (UniqueId id : m_moveToDisposed)
	{
		auto& pair = m_componentGroups[id];
		MemoryChunkObjectPointer ptr = 0;
		for (int i = 0; i < pair.second->ComponentTypes.size(); ++i)
		{
			MemoryChunkAllocator& alloc = pair.second->Allocators[i];
			MemoryChunkAllocator& dealloc = pair.second->DisposedAllocators[i];

			ptr = dealloc.AllocateObject();
			std::memcpy(dealloc.GetObjectMemory(ptr), alloc.GetObjectMemory(pair.first), alloc.GetPerObjectSize());
			alloc.FreeObject(pair.first);
		}
		pair.first = ptr;
		m_disposed.push_back(id);
	}
	m_moveToDisposed.clear();
}

std::vector<ComponentTypeId>& ComponentManager::GetComponentTypes(ComponentGroupId id)
{
	return m_componentGroups[id].second->ComponentTypes;
}

void ComponentManager::AllocCompGroup(std::set<ComponentTypeId> components, bool copied, UniqueId id)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	ComponentGroupType *type = GetComponentGroupType(components);
	MemoryChunkObjectPointer ptr;
	for (int i = 0; i < type->ComponentTypes.size(); ++i)
	{
		ptr = type->Allocators[i].AllocateObject();
		void *memory = type->Allocators[i].GetObjectMemory(ptr);

		Component *comp = Upcast<Component>(memory, registrar->GetComponentPointerOffset(type->ComponentTypes[i]));
		comp->ComponentTypeID = type->ComponentTypes[i];
		comp->EntityID = id;
		comp->Initialized = false;

		if (registrar->IsComponentBuffered(comp->ComponentTypeID))
			Upcast<ComponentBuffer>(memory, registrar->GetBufferPointerOffset(comp->ComponentTypeID))->InitializeBufferStore();
	}

	(copied ? m_copiedComponentGroups[id] : m_componentGroups[id]) = std::make_pair(ptr, type);
}

void ComponentBuffer::InitializeBufferStore()
{
	m_holder = nullptr;
}

void ComponentBuffer::DestroyBufferStore()
{
	if (m_holder)
		delete m_holder;
}

void *ComponentBuffer::GetRawBufferData()
{
	return m_holder->GetMemory();
}

int ComponentBuffer::GetTotalBufferSize()
{
	return m_holder->GetTotalSize();
}
