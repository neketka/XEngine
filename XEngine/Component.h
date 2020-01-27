#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <typeinfo>
#include <set>

#include "UUID.h"
#include "ChunkAllocator.h"

#include <mutex>

#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>

class ComponentBuffer
{
public:
	void InitializeBufferStore();
	void DestroyBufferStore();
	void *GetRawBufferData();
	int GetTotalBufferSize();
};

class Component
{
public:
	UniqueId ComponentTypeID;
	UniqueId EntityID;
	bool Initialized;
private:
	bool padding[3];
};

template<class T>
class StaticComponentInfo
{
public:
	static constexpr char *GetName()
	{
		return typeid(T).name() + 5;
	}
	static constexpr int GetSize()
	{
		return sizeof(T);
	}
	static constexpr UniqueId GetIdentifier()
	{
		return std::hash<std::string>()(GetName());
	}
	static constexpr bool IsBuffered()
	{
		return std::is_base_of<ComponentBuffer, T>();
	}
	static constexpr int GetComponentPointerOffset()
	{
		T *derived = reinterpret_cast<T *>(1);
		Component *base = static_cast<Component *>(derived);
		return reinterpret_cast<int *>(base) - reinterpret_cast<int *>(derived);
	}
	static constexpr int GetBufferPointerOffset()
	{
		if (!IsBuffered())
			return -1;
		T *derived = reinterpret_cast<T *>(1);
		Component *base = static_cast<ComponentBuffer *>(derived);
		return reinterpret_cast<int *>(base) - reinterpret_cast<int *>(derived);
	}
};

class ComponentGroupType
{
public:
	int ChunkSize;
	std::map<UniqueId, MemoryChunkAllocator *> CompTypeToAllocator;
	std::vector<UniqueId> ComponentTypes;
	std::vector<MemoryChunkAllocator> Allocators;
	std::vector<MemoryChunkAllocator> DisposedAllocators;
};

class ComponentDataIterator
{
public:
	ComponentDataIterator(std::vector<int> sizes, std::vector<void *> memoryBlocks, int first, int count)
		: m_sizes(sizes), m_memoryBlocks(memoryBlocks), m_first(first), m_count(count), 
		m_curComps(reinterpret_cast<Component **>(memoryBlocks.data()), reinterpret_cast<Component **>(memoryBlocks.data()) + memoryBlocks.size()) { }

	template<class T>
	T *Next()
	{
		if (m_first + m_index >= m_count)
			return nullptr;
		AcquireNext();
		return reinterpret_cast<T *>(m_curComps.data());
	}

	template<class T>
	T *GetAllMemory(int componentIndex)
	{
		m_first = m_count;
		return reinterpret_cast<T *>(m_memoryBlocks[componentIndex]);
	}

	int GetChunkOffset()
	{
		return m_first;
	}

	int GetChunkSize()
	{
		return m_count;
	}
private:
	std::vector<int> m_sizes;
	std::vector<void *> m_memoryBlocks;
	int m_index = -1;
	int m_first;
	int m_count;
	std::vector<Component *> m_curComps;
	void AcquireNext()
	{
		for (int i = 0; i < m_sizes.size(); ++i)
			m_curComps[i] += i * m_sizes[i];
	}
};

using FilteringGroupId = UniqueId;
using ComponentGroupId = UniqueId;
using ComponentTypeId = UniqueId;

class Scene;
class ComponentManager
{
public:
	XENGINEAPI ComponentManager(Scene *scene);
	XENGINEAPI ~ComponentManager();
	XENGINEAPI void InitializeFilteringGroups();
	XENGINEAPI FilteringGroupId AddFilteringGroup(std::vector<ComponentTypeId> components);
	XENGINEAPI std::vector<ComponentDataIterator> *GetFilteringGroup(FilteringGroupId filteringGroup, bool disposed);
	XENGINEAPI ComponentGroupId AllocateComponentGroup(std::set<ComponentTypeId> components);
	XENGINEAPI ComponentGroupType *GetComponentGroupType(std::set<ComponentTypeId> components);
	XENGINEAPI void DeleteComponentGroup(ComponentGroupId id);
	XENGINEAPI void CopyComponentData(ComponentGroupId dest, ComponentGroupId src, ComponentTypeId compId);
	XENGINEAPI std::vector<UniqueId>& GetComponentIdsFromComponentGroup(ComponentGroupId componentGroup);
	XENGINEAPI std::vector<Component *> GetComponentGroupData(ComponentGroupId componentGroup);
	XENGINEAPI Component *GetComponentGroupData(ComponentGroupId componentGroup, ComponentTypeId id);
	XENGINEAPI void ClearDeletedSingleThread();
	template<class T>
	T *Upcast(void *memory, int offset)
	{
		return reinterpret_cast<T *>(static_cast<char *>(memory) + offset);
	}
private:
	int m_componentChunkSize;
	int m_componentDisposedChunkSize;
	Scene *m_scene;
	std::map<std::vector<ComponentTypeId>, UniqueId> m_filteringToId;
	std::map<UniqueId, std::vector<ComponentTypeId>> m_idToFiltering;
	std::map<UniqueId, UniqueId> m_filteringIdToInternalId;

	std::map<std::set<ComponentTypeId>, UniqueId> m_filteringCompsToInternalFiltering;
	std::map<std::set<ComponentTypeId>, ComponentGroupType *> m_componentGroupTypes;
	std::unordered_map<UniqueId, concurrency::concurrent_vector<ComponentGroupType *>> m_internalFilteringIdToComponentGroup;
	concurrency::concurrent_unordered_map<UniqueId, std::pair<MemoryChunkObjectPointer, ComponentGroupType *>> m_componentGroups;

	concurrency::concurrent_unordered_set<UniqueId> m_moveToDisposed;
	std::vector<UniqueId> m_disposed;
};
