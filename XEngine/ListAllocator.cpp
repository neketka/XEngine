#include "pch.h"
#include "ListAllocator.h"

ListAllocator::ListAllocator(int size, int maxAllocs)
{
	m_maxSize = size;
	m_pointer = 0;
	m_maxHeaders = maxAllocs;

	m_headerLinks.resize(maxAllocs);
	m_freeList.resize(maxAllocs);
	for (int i = 0; i < maxAllocs; ++i)
		m_freeList[i] = i;
}

ListAllocator::~ListAllocator()
{
}

ListMemoryPointer *ListAllocator::AllocateMemory(int size, int alignment)
{
	int alignmentPad = m_pointer % alignment;

	int thisPtr = m_pointer + alignmentPad;
	int newPtr = thisPtr + size;

	if (newPtr >= m_maxSize)
	{
		Defragment();
		alignmentPad = m_pointer % alignment;
		thisPtr = m_pointer + alignmentPad;
		newPtr = thisPtr + size;
		if (newPtr >= m_maxSize)
			return nullptr;
	}

	int index = m_freeList.back();
	m_freeList.pop_back();

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
		m_last->Next = h;
	}

	m_pointer = newPtr;

	return dynamic_cast<ListMemoryPointer *>(m_last);
}

void ListAllocator::DeallocateMemory(ListMemoryPointer *ptr)
{
	ListEntryHeader *h = static_cast<ListEntryHeader *>(ptr);
	h->Prev->Next = h->Next;
	h->Next->Prev = h->Prev;
	m_freeList.push_back(h->Index);
	m_freeSpace += h->Size + h->Padding;
}

int ListAllocator::GetAllocationSize(ListMemoryPointer *ptr)
{
	ListEntryHeader *h = static_cast<ListEntryHeader *>(ptr);
	return h->Size;
}

void ListAllocator::ShrinkToFit(int extraMemory)
{
	m_maxSize = m_pointer + extraMemory + 1;
}

void ListAllocator::SetSize(int maxSize)
{
	m_maxSize = maxSize;
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

void ListAllocator::Defragment()
{
	int pointer = 0;

	MoveData mv;
	for (ListEntryHeader *h = m_first; h->Next != nullptr; h = h->Next)
	{
		mv.SrcIndex = h->Pointer;
		mv.Size = h->Size;

		int alignmentPad = pointer % h->Alignment;
		int thisPtr = pointer + alignmentPad;
		int newPtr = thisPtr + h->Size;

		h->Pointer = thisPtr;
		h->Padding = alignmentPad;

		mv.DestIndex = h->Pointer = thisPtr;
		pointer = newPtr;

		if (mv.SrcIndex != mv.DestIndex)
			m_move(mv);
	}
	m_pointer = pointer;
}
