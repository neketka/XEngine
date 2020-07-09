#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <memory>

#include "exports.h"

using ListPointer = unsigned __int64;

class ListMemoryPointer
{
public:
	ListPointer Pointer;
};

class ListEntryHeader : public ListMemoryPointer
{
public:
	ListEntryHeader *Prev;
	ListEntryHeader *Next;
	int Index;
	int Padding;
	unsigned long long Size;
	unsigned long long Alignment;
	bool Pinned;
};

class MoveData
{
public:
	ListPointer SrcIndex;
	ListPointer DestIndex;
	int Size;
};

class ListAllocator;
class PinnedListMemory
{
public:
	XENGINEAPI PinnedListMemory(ListAllocator *allocator, ListMemoryPointer *ptr);
	XENGINEAPI ~PinnedListMemory();
	ListMemoryPointer *GetPointer() { return m_ptr; }
	XENGINEAPI void Unpin();
private:
	ListAllocator *m_allocator;
	ListMemoryPointer *m_ptr;
	bool m_unpinned;
};

class ListAllocator
{
public:
	XENGINEAPI ListAllocator(unsigned long long size, int maxAllocs);
	XENGINEAPI ~ListAllocator();

	XENGINEAPI ListMemoryPointer *AllocateMemory(int size, int alignment);
	XENGINEAPI void DeallocateMemory(ListMemoryPointer *ptr);

	XENGINEAPI int GetAllocationSize(ListMemoryPointer *ptr);

	XENGINEAPI void ShrinkToFit(int extraMemory);
	XENGINEAPI void SetSize(unsigned long long maxSize);

	XENGINEAPI std::shared_ptr<PinnedListMemory> PinMemory(ListMemoryPointer *ptr);
	XENGINEAPI void PinMemoryUnmanaged(ListMemoryPointer *ptr);
	XENGINEAPI void UnpinMemoryUnmanaged(ListMemoryPointer *ptr);

	XENGINEAPI int GetMaxSize();
	XENGINEAPI int GetFreeSpace();

	XENGINEAPI void SetMoveCallback(std::function<void(MoveData&)> move);
	XENGINEAPI void SetDefragBeginCallback(std::function<void()> begin);
	XENGINEAPI void SetDefragEndCallback(std::function<void()> end);
private:
	void Defragment();

	std::shared_mutex m_defragLock;
	std::mutex m_allocLock;
	
	std::vector<ListEntryHeader> m_headerLinks;
	concurrency::concurrent_queue<int> m_freeList;

	ListEntryHeader *m_first = nullptr;
	ListEntryHeader *m_last = nullptr;

	unsigned long long m_pointer;
	std::atomic<unsigned long long> m_freeSpace;

	unsigned long long m_maxSize;
	int m_maxHeaders;

	std::function<void(MoveData&)> m_move;
	std::function<void()> m_defragBegin = []() {};
	std::function<void()> m_defragEnd = []() {};
};