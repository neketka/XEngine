#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <typeinfo>
#include <set>

#include "UUID.h"

#include <mutex>

#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>

class Component
{
public:
	UniqueId ComponentTypeID;
	UniqueId EntityID;
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
};

class ComponentGroupType
{
public:
	int ChunkSize;
	std::vector<UniqueId> ComponentTypes;
	std::vector<MemoryChunkAllocator> Allocators;
	std::vector<MemoryChunkAllocator> DisposedAllocators;
};

class ComponentDataIterator
{
public:
	ComponentDataIterator(std::vector<int>& sizes, std::vector<void *>& memoryBlocks, int first, int count)
		: m_sizes(sizes), m_memoryBlocks(memoryBlocks), m_first(first), m_count(count), m_curComps(memoryBlocks) { }

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
	std::vector<int>& m_sizes;
	std::vector<void *>& m_memoryBlocks;
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

class Scene;
class ComponentManager
{
public:
	XENGINEAPI ComponentManager(Scene *scene);
	XENGINEAPI ~ComponentManager();
	XENGINEAPI void InitializeFilteringGroups();
	XENGINEAPI UniqueId AddFilteringGroup(std::vector<UniqueId> components);
	XENGINEAPI std::vector<ComponentDataIterator> GetFilteringGroup(UniqueId filteringGroup, bool disposed, int minimumSplit);
	XENGINEAPI UniqueId AllocateComponentGroup(std::set<UniqueId> components);
	XENGINEAPI void DeleteComponentGroup(UniqueId id);
	XENGINEAPI void CopyComponentData(UniqueId dest, UniqueId src);
	XENGINEAPI std::vector<Component *> GetComponentGroupData(UniqueId componentGroup);
private:
	int m_componentChunkSize;
	std::map<std::vector<UniqueId>, UniqueId> m_filteringToId;
	std::map<UniqueId, std::vector<UniqueId>> m_idToFiltering;
	std::map<std::set<UniqueId>, ComponentGroupType *> m_componentGroupTypes;
	std::unordered_map<UniqueId, concurrency::concurrent_vector<ComponentGroupType *>> m_filteringIdToComponentGroup;
	concurrency::concurrent_unordered_map<UniqueId, std::pair<std::vector<MemoryChunkObjectPointer>, ComponentGroupType *>> m_componentGroups;
};
