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


class BufferedComponentHolder
{
public:
	virtual ~BufferedComponentHolder() = 0;
	virtual void *GetMemory() = 0;
	virtual int GetTotalSize() = 0;
};

class Component
{
public:
	UniqueId ComponentTypeID;
	UniqueId EntityID;
};

template<class T>
class TypedBufferHolder : public BufferedComponentHolder
{
public:
	virtual ~TypedBufferHolder() {}

	virtual void *GetMemory() override
	{
		return m_vec.data();
	}

	virtual int GetTotalSize() override
	{
		return sizeof(T) * m_vec.size();
	}

	std::vector<T>& GetVector()
	{
		return m_vec;
	}
private:
	std::vector<T> m_vec;
};

class BufferedComponent : public Component
{
public:
	void InitializeBufferStore();
	void DestroyBufferStore();
	void *GetRawBufferData();
	int GetTotalBufferSize();
protected:
	BufferedComponentHolder *m_holder;
	char m_padding[16 - sizeof(BufferedComponentHolder *)]; // Usually the size of char[8]: Padding for SIMD
};

template<class T>
class TypedBufferedComponent : public BufferedComponent
{
public:
	std::vector<T>& GetVector() // Lazy initialize to prevent usage of vftable through inheritance
	{
		TypedBufferHolder<T> *typed;
		if (m_holder)
			typed = dynamic_cast<TypedBufferHolder<T>>(m_holder);
		else
			m_holder = typed = new TypedBufferHolder<T>();
		return typed->GetVector();
	}
};

template<class T>
class StaticComponentInfo
{
public:
	static constexpr const char *GetName()
	{
		return typeid(T).name() + 6;
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
		return std::is_base_of<BufferedComponent, T>();
	}
	static constexpr int GetComponentPointerOffset()
	{
		T *derived = reinterpret_cast<T *>(1);
		Component *base = static_cast<Component *>(derived);
		auto diff = reinterpret_cast<char *>(base) - reinterpret_cast<char *>(derived);
		return diff;
	}
};

template<class T>
class BufferedComponentInfo
{
public:
	static constexpr int GetBufferPointerOffset()
	{
		T *derived = reinterpret_cast<T *>(1);
		Component *base = static_cast<Component *>(derived);
		auto diff1 = reinterpret_cast<char *>(base) - reinterpret_cast<char *>(derived);

		BufferedComponent *buffcomp = reinterpret_cast<BufferedComponent *>(1);
		Component *comp = static_cast<Component *>(buffcomp);
		auto diff2 = reinterpret_cast<char *>(buffcomp) - reinterpret_cast<char *>(comp);
		
		return diff1 + diff2;
	}
};

class ComponentGroupType
{
public:
	int ChunkSize;
	std::unordered_map<UniqueId, MemoryChunkAllocator *> CompTypeToAllocator;
	std::unordered_map<UniqueId, MemoryChunkAllocator *> CompTypeToDisposedAllocator;

	std::vector<MemoryChunkAllocator> Allocators;
	std::vector<MemoryChunkAllocator> DisposedAllocators;

	std::vector<UniqueId> ComponentTypes;
};

class ComponentDataIterator
{
public:
	ComponentDataIterator(std::vector<int> sizes, std::vector<void *> memoryBlocks, int first, int count)
		: m_sizes(sizes), m_memoryBlocks(memoryBlocks), m_first(first), m_count(count), m_curComps(sizes.size()) { }

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

	void *UserPointer = nullptr;
	bool UserFlag = false;
private:
	std::vector<int> m_sizes;
	std::vector<void *> m_memoryBlocks;
	std::vector<void *> m_curComps;
	int m_index = 0;
	int m_first;
	int m_count;
	void AcquireNext()
	{
		for (int i = 0; i < m_sizes.size(); ++i)
			m_curComps[i] = reinterpret_cast<char *>(m_memoryBlocks[i]) + (m_first + m_index) * m_sizes[i];
		++m_index;
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
	XENGINEAPI ComponentGroupId GetFirstComponentGroupWithType(ComponentTypeId id);
	XENGINEAPI ComponentGroupId AllocateComponentGroup(std::set<ComponentTypeId> components);
	XENGINEAPI ComponentGroupType *GetComponentGroupType(std::set<ComponentTypeId> components);
	XENGINEAPI void DeleteComponentGroup(ComponentGroupId id);
	XENGINEAPI void CopyComponentData(ComponentGroupId dest, ComponentGroupId src, ComponentTypeId compId);
	XENGINEAPI std::vector<UniqueId>& GetComponentIdsFromComponentGroup(ComponentGroupId componentGroup);
	XENGINEAPI std::vector<Component *> GetComponentGroupData(ComponentGroupId componentGroup);
	XENGINEAPI Component *GetComponentGroupData(ComponentGroupId componentGroup, ComponentTypeId id);
	XENGINEAPI void RebuildComponentGroup(ComponentGroupId componentGroup, std::set<ComponentTypeId> components);
	XENGINEAPI void ExecuteSingleThreadOps(); // Operations to be executed on one thread after no operations are done to components
	XENGINEAPI std::vector<ComponentTypeId>& GetComponentTypes(ComponentGroupId id);

	template<class T>
	T *Upcast(void *memory, int offset)
	{
		return reinterpret_cast<T *>(static_cast<char *>(memory) + offset);
	}
private:
	void AllocCompGroup(std::set<ComponentTypeId> components, bool moved, UniqueId id);
	Scene *m_scene;

	int m_componentChunkSize;
	int m_componentDisposedChunkSize;

	std::map<std::vector<ComponentTypeId>, UniqueId> m_filteringToId; // Map from the ordered filtering groups to their ids
	std::map<UniqueId, std::vector<ComponentTypeId>> m_idToFiltering; // Map from a filtering group id to its ordered components list
	std::map<UniqueId, UniqueId> m_filteringIdToInternalId; // Map from an ordered filtering group id to its unordered equivalent's id

	std::map<std::set<ComponentTypeId>, UniqueId> m_filteringCompsToInternalFiltering; // Map from a set of components to an unordered filtering id
	std::map<std::set<ComponentTypeId>, ComponentGroupType *> m_componentGroupTypes; // Map from a set of components to a matching component group type
	std::unordered_map<UniqueId, std::vector<ComponentGroupType *>> m_internalFilteringIdToComponentGroup; // Map from an unordered filtering group to a list of component group types

	std::mutex compGroupTypeAddMutex;

	concurrency::concurrent_unordered_map<UniqueId, std::pair<MemoryChunkObjectPointer, ComponentGroupType *>> m_componentGroups; // Map from a component group id to its pointer and component group type
	concurrency::concurrent_unordered_map<UniqueId, std::pair<MemoryChunkObjectPointer, ComponentGroupType *>> m_movedComponentGroups; // Map from a component group about to be moved id to its pointer and component group type

	concurrency::concurrent_unordered_set<UniqueId> m_moveToDisposed; // Components about to be disposed by systems
	std::vector<UniqueId> m_disposed; // Components to be disposed by deallocators
};
