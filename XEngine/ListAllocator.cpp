#include "pch.h"
#include "ListAllocator.h"

ListAllocator::ListAllocator(unsigned long long size, int maxAllocs)
{
	m_maxSize = size;
	m_pointer = 0;
	m_maxHeaders = maxAllocs;
	m_freeSpace = size;

	m_headerLinks.resize(maxAllocs);
	for (int i = 0; i < maxAllocs; ++i)
		m_freeList.push(i);
}

ListAllocator::~ListAllocator()
{
}

ListMemoryPointer *ListAllocator::AllocateMemory(int size, int alignment)
{
	std::lock_guard<std::mutex> lock0(m_allocLock);
	//std::shared_lock<std::shared_mutex> lock1(m_defragLock);

	int alignmentPad = m_pointer % alignment;
	unsigned long long thisPtr = m_pointer + alignmentPad;
	unsigned long long newPtr = thisPtr + size;

	if (newPtr >= m_maxSize)
	{
		Defragment();
		alignmentPad = m_pointer % alignment;
		thisPtr = m_pointer + alignmentPad;
		newPtr = thisPtr + size;
		if (newPtr >= m_maxSize)
			return nullptr;
	}

	int index = 0;
	if (!m_freeList.try_pop(index))
		return nullptr;

	m_freeSpace -= alignmentPad + size;

	if (!m_first)
	{
		m_first = m_last = &m_headerLinks[index];
		m_first->Pointer = thisPtr;
		m_first->Alignment = alignment;
		m_first->Prev = nullptr;
		m_first->Next = nullptr;
		m_first->Size = size;
		m_first->Index = index;
		m_first->Padding = alignmentPad;
		m_first->Pinned = false;
	}
	else
	{
		ListEntryHeader *h = &m_headerLinks[index];
		h->Pointer = thisPtr;
		h->Alignment = alignment;
		h->Prev = m_last;
		h->Next = nullptr;
		h->Size = size;
		h->Index = index;
		h->Padding = alignmentPad;
		h->Pinned = false;
		m_last->Next = h;
		m_last = h;
	}

	m_pointer = newPtr;

	return dynamic_cast<ListMemoryPointer *>(m_last);
}

void ListAllocator::DeallocateMemory(ListMemoryPointer *ptr)
{
	UnpinMemoryUnmanaged(ptr);

	m_defragLock.lock_shared();
	ListEntryHeader *h = static_cast<ListEntryHeader *>(ptr);
	if (h->Prev)
		h->Prev->Next = h->Next;
	if (h->Next)
		h->Next->Prev = h->Prev;
	m_freeList.push(h->Index);
	m_freeSpace += h->Size + h->Padding;
	m_defragLock.unlock_shared();
}

int ListAllocator::GetAllocationSize(ListMemoryPointer *ptr)
{
	ListEntryHeader *h = static_cast<ListEntryHeader *>(ptr);
	return h->Size;
}

void ListAllocator::ShrinkToFit(int extraMemory)
{
	m_maxSize = m_pointer + static_cast<unsigned long long>(extraMemory) + 1;
}

void ListAllocator::SetSize(unsigned long long maxSize)
{
	m_maxSize = maxSize;
}

std::shared_ptr<PinnedListMemory> ListAllocator::PinMemory(ListMemoryPointer *ptr)
{
	PinMemoryUnmanaged(ptr);
	return std::make_shared<PinnedListMemory>(this, ptr);
}

void ListAllocator::PinMemoryUnmanaged(ListMemoryPointer *ptr)
{
	m_defragLock.lock_shared();
	ListEntryHeader *h = static_cast<ListEntryHeader *>(ptr);
	h->Pinned = true;
	m_defragLock.unlock_shared();
}

void ListAllocator::UnpinMemoryUnmanaged(ListMemoryPointer *ptr)
{
	ListEntryHeader *h = static_cast<ListEntryHeader *>(ptr);
	h->Pinned = false;
}

int ListAllocator::GetMaxSize()
{
	return m_maxSize;
}

int ListAllocator::GetFreeSpace()
{
	return m_freeSpace;
}

void ListAllocator::SetMoveCallback(std::function<void(MoveData&)> move)
{
	m_move = move;
}

void ListAllocator::SetDefragBeginCallback(std::function<void()> begin)
{
	m_defragBegin = begin;
}

void ListAllocator::SetDefragEndCallback(std::function<void()> end)
{
	m_defragEnd = end;
}

void ListAllocator::Defragment()
{
	m_defragLock.lock();
	m_defragBegin();

	unsigned long long pointer = 0;

	MoveData mv;
	for (ListEntryHeader *h = m_first; h->Next != nullptr; h = h->Next)
	{
		if (h->Pinned)
		{
			pointer = h->Pointer + h->Size;
			continue;
		}
		mv.SrcIndex = h->Pointer;
		mv.Size = h->Size;

		int alignmentPad = pointer % h->Alignment;
		unsigned long long thisPtr = pointer + alignmentPad;
		unsigned long long newPtr = thisPtr + h->Size;

		h->Pointer = thisPtr;
		h->Padding = alignmentPad;

		mv.DestIndex = h->Pointer = thisPtr;
		pointer = newPtr;

		if (mv.SrcIndex != mv.DestIndex)
			m_move(mv);
	}
	m_pointer = pointer;

	m_defragEnd();
	m_defragLock.unlock();
}

PinnedListMemory::PinnedListMemory(ListAllocator *allocator, ListMemoryPointer *ptr) : m_allocator(allocator), m_ptr(ptr), m_unpinned(false)
{
}

PinnedListMemory::~PinnedListMemory()
{
	if (!m_unpinned)
	{
		m_unpinned = true;
		m_allocator->UnpinMemoryUnmanaged(m_ptr);
	}
}

void PinnedListMemory::Unpin()
{
	if (!m_unpinned)
	{
		m_unpinned = true;
		m_allocator->UnpinMemoryUnmanaged(m_ptr);
	}
}
