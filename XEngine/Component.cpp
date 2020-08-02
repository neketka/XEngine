#include "pch.h"
#include <algorithm>

ComponentManager::ComponentManager(Scene *scene) : m_scene(scene)
{
	m_componentChunkSize = 32; // Set size of chunk for components
	m_componentDisposedChunkSize = 8; // Set size of chunk for disposed components
}

ComponentManager::~ComponentManager()
{
	for (auto pair : m_componentGroupTypes)
	{
		for (MemoryChunkAllocator& alloc : pair.second->Allocators)
			alloc.CleanupAllocator();
		for (MemoryChunkAllocator& alloc : pair.second->DisposedAllocators)
			alloc.CleanupAllocator();
		delete pair.second; // Delete all component group types
	}
}

void ComponentManager::InitializeFilteringGroups()
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	for (auto comp : registrar->GetComponentMap()) // Add filtering group for each possible component alone
	{
		AddFilteringGroup({ registrar->GetComponentIdByName("EntityIdComponent"), comp.first });
	}
}

FilteringGroupId ComponentManager::AddFilteringGroup(std::vector<ComponentTypeId> components)
{
	auto iter = m_filteringToId.find(components); // Iterator for the filering group
	if (iter == m_filteringToId.end()) // If filering group does not exist
	{
		ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
		std::set<UniqueId> setComp(components.begin(), components.end()); // Set of necessary components

		auto cgIter = m_filteringCompsToInternalFiltering.find(setComp); // Find the internal filtering group associated with that set of components

		UniqueId internalId;
		if (cgIter == m_filteringCompsToInternalFiltering.end())
		{
			internalId = m_filteringCompsToInternalFiltering[setComp] = GenerateID(); // Generate new unordered filtering group id
			auto& vec = m_internalFilteringIdToComponentGroup[internalId] = std::vector<ComponentGroupType *>(); // Create list of component group types associated with this unordered filtering group
			for (auto compGroup : m_componentGroupTypes)
			{
				if (std::includes(compGroup.first.begin(), compGroup.first.end(), setComp.begin(), setComp.end())) // Is this unordered filtering group contained in this component group type
				{
					vec.push_back(compGroup.second);
				}
			}
		}
		else internalId = cgIter->second;

		UniqueId id = m_filteringToId[components] = GenerateID(); // Generate new filtering group id
		m_idToFiltering[id] = components; // Set the component order associated with the filtering group
		m_filteringIdToInternalId[id] = internalId; // Set the unordered filtering group associated with this filtering group
		return id;
	}
	return iter->second;
}

std::vector<ComponentDataIterator> *ComponentManager::GetFilteringGroup(FilteringGroupId filteringGroup, bool disposed)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	
	std::vector<ComponentDataIterator> *filtering = new std::vector<ComponentDataIterator>; // Prepare a list for the iterators
	std::vector<UniqueId>& order = m_idToFiltering[filteringGroup]; // Get the order of the components
	auto& compTypes = m_internalFilteringIdToComponentGroup[m_filteringIdToInternalId[filteringGroup]]; // Get the types of components
	for (ComponentGroupType *type : compTypes)
	{
		int32_t chunkCount = (disposed ? type->DisposedAllocators[0] : type->Allocators[0]).GetActiveChunkCount(); // Get the chunk count
		for (int32_t chunk = 0; chunk < chunkCount; ++chunk)
		{
			std::vector<void *> compBlocks(type->ComponentTypes.size()); // Component pointers
			std::vector<int32_t> sizes(type->ComponentTypes.size()); // Component sizes

			for (int32_t comp = 0; comp < order.size(); ++comp)
			{
				auto allocator = (disposed ? type->CompTypeToDisposedAllocator : type->CompTypeToAllocator)[order[comp]]; // Find the correct allocator for this usage
				compBlocks[comp] = allocator->GetAllChunks()[chunk].Memory;
				sizes[comp] = allocator->GetPerObjectSize();
			}

			int32_t count = type->Allocators[0].GetAllChunks()[0].ObjectCount;
			filtering->push_back(ComponentDataIterator(sizes, compBlocks, 0, count));
		}
	}

	return filtering;
}

ComponentGroupId ComponentManager::AllocateComponentGroup(std::set<ComponentTypeId> components)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	components.insert(registrar->GetComponentIdByName("EntityIdComponent"));

	ComponentGroupId id = GenerateID();

	AllocCompGroup(components, false, id); // Allocate new, unmoved component group

	return id;
}

ComponentGroupType *ComponentManager::GetComponentGroupType(std::set<ComponentTypeId> components)
{
	auto iter = m_componentGroupTypes.find(components);
	if (iter == m_componentGroupTypes.end()) // Component group type not found
	{
		if (!compGroupTypeAddMutex.try_lock())
		{
			compGroupTypeAddMutex.lock();
			iter = m_componentGroupTypes.find(components);
			if (iter != m_componentGroupTypes.end())
			{
				compGroupTypeAddMutex.unlock();
				return iter->second;
			}
		}

		ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();

		ComponentGroupType *type = m_componentGroupTypes[components] = new ComponentGroupType; // Create new type for these components
		type->ChunkSize = m_componentChunkSize;
		type->ComponentTypes = std::vector<UniqueId>(components.begin(), components.end()); // Copy the components to an internal vector
		type->Allocators.reserve(type->ComponentTypes.size()); // Preallocate space for vectors
		type->DisposedAllocators.reserve(type->ComponentTypes.size());
		for (UniqueId id : type->ComponentTypes)
		{
			int32_t size = registrar->GetComponentSize(id);
			type->Allocators.push_back(MemoryChunkAllocator(type->ChunkSize, size)); // Create allocators for the type
			type->DisposedAllocators.push_back(MemoryChunkAllocator(m_componentDisposedChunkSize, size));
			type->CompTypeToAllocator[id] = &type->Allocators.back();
			type->CompTypeToDisposedAllocator[id] = &type->DisposedAllocators.back();
		}
		for (auto pair : m_filteringCompsToInternalFiltering)
		{
			if (std::includes(components.begin(), components.end(), pair.first.begin(), pair.first.end())) // Do any filtering groups use this component group
			{
				m_internalFilteringIdToComponentGroup[pair.second].push_back(type); // If so add this group to the unordered filtering group
			}
		}

		compGroupTypeAddMutex.unlock();
		return type;
	}
	return iter->second;
}

void ComponentManager::DeleteComponentGroup(ComponentGroupId id)
{
	m_moveToDisposed.insert(id); // Stage for disposal loop
}

void ComponentManager::CopyComponentData(ComponentGroupId dest, ComponentGroupId src, ComponentTypeId compId)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();

	auto destPair = m_componentGroups[dest];
	auto srcPair = m_componentGroups[src];

	std::memcpy(destPair.second->CompTypeToAllocator[compId]->GetObjectMemory(destPair.first), // Copy memory to memory
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
	std::vector<Component *> data(pair.second->ComponentTypes.size()); // Preallocate with size

	for (int32_t i = 0; i < pair.second->ComponentTypes.size(); ++i)
	{
		data[i] = Upcast<Component>(pair.second->Allocators[i].GetObjectMemory(pair.first), // Cast from Derived to Component 
			registrar->GetComponentPointerOffset(pair.second->ComponentTypes[i]));
	}

	return data;
}

Component *ComponentManager::GetComponentGroupData(ComponentGroupId componentGroup, ComponentTypeId id)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	Component *ret = nullptr;

	auto pair = m_componentGroups[componentGroup];
	auto movedPair = m_componentGroups[componentGroup];

	if (std::find(pair.second->ComponentTypes.begin(), pair.second->ComponentTypes.end(), id) != pair.second->ComponentTypes.end()) // Is it in the non-moved components
		ret = Upcast<Component>(pair.second->CompTypeToAllocator[id]->GetObjectMemory(pair.first),
			registrar->GetComponentPointerOffset(id));
	else if (std::find(movedPair.second->ComponentTypes.begin(), movedPair.second->ComponentTypes.end(), id) != movedPair.second->ComponentTypes.end()) // Or is it in the moved components
		ret = Upcast<Component>(movedPair.second->CompTypeToAllocator[id]->GetObjectMemory(movedPair.first),
			registrar->GetComponentPointerOffset(id));

	return ret;
}

void ComponentManager::RebuildComponentGroup(ComponentGroupId componentGroup, std::set<ComponentTypeId> components)
{
	AllocCompGroup(components, true, componentGroup); // Allocate a moved component group
}

void ComponentManager::ExecuteSingleThreadOps()
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	for (auto& pair : m_movedComponentGroups) 
	{
		auto originalPair = m_componentGroups[pair.first];
		for (ComponentTypeId comp : pair.second.second->ComponentTypes) // Loop through all components in the original component group
		{
			std::memcpy(pair.second.second->CompTypeToAllocator[comp]->GetObjectMemory(pair.second.first), // Copy memory from original to moved
				originalPair.second->CompTypeToAllocator[comp]->GetObjectMemory(originalPair.first), registrar->GetComponentSize(comp));
			originalPair.second->CompTypeToAllocator[comp]->FreeObject(originalPair.first); // Destroy original component group
		}
		m_componentGroups[pair.first] = pair.second;
	}
	m_movedComponentGroups.clear(); // Clear "to be moved"
	for (UniqueId id : m_disposed)
	{
		auto pair = m_componentGroups[id];
		for (int32_t i = 0; i < pair.second->ComponentTypes.size(); ++i)
		{
			if (registrar->IsComponentBuffered(pair.second->ComponentTypes[i]))
				Upcast<BufferedComponent>(pair.second->DisposedAllocators[i].GetObjectMemory(pair.first), // Destroy buffer if necessary
					registrar->GetBufferPointerOffset(pair.second->ComponentTypes[i]))->DestroyBufferStore();
			pair.second->DisposedAllocators[i].FreeObject(pair.first); // Dispose of the cleaned up object
		}
		m_componentGroups.unsafe_erase(id); // Erase component group from list
	}
	m_disposed.clear(); // Clear "disposed"
	for (UniqueId id : m_moveToDisposed)
	{
		auto& pair = m_componentGroups[id];
		MemoryChunkObjectPointer ptr = 0;
		for (int32_t i = 0; i < pair.second->ComponentTypes.size(); ++i)
		{
			MemoryChunkAllocator& alloc = pair.second->Allocators[i];
			MemoryChunkAllocator& dealloc = pair.second->DisposedAllocators[i];

			ptr = dealloc.AllocateObject();
			// Move the component group to "to be disposed" for systems to clean up and to be deleted later
			std::memcpy(dealloc.GetObjectMemory(ptr), alloc.GetObjectMemory(pair.first), alloc.GetPerObjectSize());
			alloc.FreeObject(pair.first); // Delete the object from the regular component groups
		}
		pair.first = ptr;
		m_disposed.push_back(id);
	}
	m_moveToDisposed.clear(); // Clear "to be disposed"
}

std::vector<ComponentTypeId>& ComponentManager::GetComponentTypes(ComponentGroupId id)
{
	return m_componentGroups[id].second->ComponentTypes;
}

void ComponentManager::AllocCompGroup(std::set<ComponentTypeId> components, bool moved, UniqueId id)
{
	ECSRegistrar *registrar = XEngine::GetInstance().GetECSRegistrar();
	ComponentGroupType *type = GetComponentGroupType(components);
	MemoryChunkObjectPointer ptr;
	for (int32_t i = 0; i < type->ComponentTypes.size(); ++i)
	{
		ptr = type->Allocators[i].AllocateObject();
		void *memory = type->Allocators[i].GetObjectMemory(ptr); // Get component memory

		Component *comp = Upcast<Component>(memory, registrar->GetComponentPointerOffset(type->ComponentTypes[i]));
		ComponentTypeId tid = type->ComponentTypes[i];

		if (registrar->IsComponentBuffered(tid)) // Initialize buffer if necessary
			Upcast<BufferedComponent>(memory, registrar->GetBufferPointerOffset(tid))->InitializeBufferStore();
	}

	UniqueId *idPtr = reinterpret_cast<UniqueId *>(type->CompTypeToAllocator[registrar->GetComponentIdByName("EntityIdComponent")]
		->GetObjectMemory(ptr));
	*idPtr = id;

	(moved ? m_movedComponentGroups : m_componentGroups)[id] = std::make_pair(ptr, type);
}

void BufferedComponent::InitializeBufferStore()
{
	m_holder = nullptr;
}

void BufferedComponent::DestroyBufferStore()
{
	if (m_holder)
		delete m_holder;
}

void *BufferedComponent::GetRawBufferData()
{
	return m_holder->GetMemory();
}

int32_t BufferedComponent::GetTotalBufferSize()
{
	return m_holder->GetTotalSize();
}
