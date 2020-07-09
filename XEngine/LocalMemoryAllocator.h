#pragma once

#include "exports.h"
#include "ListAllocator.h"

template<class T>
class PinnedLocalMemory
{
public:
	PinnedLocalMemory() {}
	PinnedLocalMemory(std::shared_ptr<PinnedListMemory> mem, T *data, int size) : m_mem(mem), m_data(data), m_size(size) {}
	T *GetData() { return m_data; }
	int GetSize() { return m_size; }
	T *operator->() { return m_data; }
	T& operator* (){ return *m_data; }
	void Unpin() { m_mem->Unpin(); }
private:
	std::shared_ptr<PinnedListMemory> m_mem;
	T *m_data;
	int m_size;
};

class LocalMemoryAllocator
{
public:
	XENGINEAPI LocalMemoryAllocator(unsigned long long size, int maxAllocs, bool resizable);
	XENGINEAPI ~LocalMemoryAllocator();
	XENGINEAPI bool WillFit(int size);
	template<class T>
	PinnedLocalMemory<T> GetMemory(ListMemoryPointer *ptr)
	{
		return PinnedLocalMemory<T>(m_allocator.PinMemory(ptr),
			reinterpret_cast<T *>(reinterpret_cast<char *>(m_memory) + ptr->Pointer), m_allocator.GetAllocationSize(ptr) / sizeof(T));
	}
	XENGINEAPI ListMemoryPointer *RequestSpace(int bytes, int alignment = 1);
	XENGINEAPI void FreeSpace(ListMemoryPointer *ptr);
private:
	void MoveMemory(MoveData& data);
	unsigned long long m_size;
	void *m_memory;
	ListAllocator m_allocator;
	bool m_resizable;
};