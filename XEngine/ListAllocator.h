#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <memory>

#include "exports.h"

using ListPointer = uint64_t;

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
	int32_t Index;
	int32_t Padding;
	uint64_t Size;
	uint64_t Alignment;
	bool Pinned;
};

class MoveData
{
public:
	ListPointer SrcIndex;
	ListPointer DestIndex;
	int32_t Size;
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
	XENGINEAPI ListAllocator(uint64_t size, int32_t maxAllocs);
	XENGINEAPI ~ListAllocator();

	XENGINEAPI ListMemoryPointer *AllocateMemory(int32_t size, int32_t alignment);
	XENGINEAPI void DeallocateMemory(ListMemoryPointer *ptr);

	XENGINEAPI int32_t GetAllocationSize(ListMemoryPointer *ptr);

	XENGINEAPI void ShrinkToFit(int32_t extraMemory);
	XENGINEAPI void SetSize(uint64_t maxSize);

	XENGINEAPI std::shared_ptr<PinnedListMemory> PinMemory(ListMemoryPointer *ptr);
	XENGINEAPI void PinMemoryUnmanaged(ListMemoryPointer *ptr);
	XENGINEAPI void UnpinMemoryUnmanaged(ListMemoryPointer *ptr);

	XENGINEAPI int32_t GetMaxSize();
	XENGINEAPI int32_t GetFreeSpace();

	XENGINEAPI void SetMoveCallback(std::function<void(MoveData&)> move);
	XENGINEAPI void SetDefragBeginCallback(std::function<void()> begin);
	XENGINEAPI void SetDefragEndCallback(std::function<void()> end);
private:
	void Defragment();

	std::shared_mutex m_defragLock;
	std::mutex m_allocLock;
	
	std::vector<ListEntryHeader> m_headerLinks;
	concurrency::concurrent_queue<int32_t> m_freeList;

	ListEntryHeader *m_first = nullptr;
	ListEntryHeader *m_last = nullptr;

	uint64_t m_pointer;
	std::atomic<uint64_t> m_freeSpace;

	uint64_t m_maxSize;
	int32_t m_maxHeaders;

	std::function<void(MoveData&)> m_move;
	std::function<void()> m_defragBegin = []() {};
	std::function<void()> m_defragEnd = []() {};
};