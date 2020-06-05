#include "pch.h"
#include "ListAllocator.h"

ListAllocator::ListAllocator(int size, int maxAllocs)
{
	m_maxSize = size;
	m_pointer = 0;
	m_maxHeaders = maxAllocs;

	m_headers.reserve(maxAllocs);
	m_pointers.reserve(maxAllocs);
}

ListAllocator::~ListAllocator()
{
}

UniqueId ListAllocator::AllocateMemory(int size, int alignment)
{
	int alignmentPad = m_pointer % alignment;

	int newPtr = m_pointer + alignmentPad + size;

	if (newPtr >= m_maxSize)
	{
		Defragment();
		alignmentPad = m_pointer % alignment;
		newPtr = m_pointer + alignmentPad + size;
		if (newPtr >= m_maxSize)
			return 0;
	}

	UniqueId id = GenerateID();
	m_headers.push_back(ListEntryHeader(id, m_pointer + alignmentPad, size, alignmentPad, alignment));
	m_pointer = newPtr;
	m_pointers[id] = &m_headers.back();

	return id;
}

void ListAllocator::DeallocateMemory(UniqueId block)
{
	m_pointers[block]->Id = 0;
	m_pointers.erase(block);
}

ListPointer ListAllocator::GetPointer(UniqueId block)
{
	ListEntryHeader *h = m_pointers[block];
	return h->Pointer + h->PaddingSize;
}

void ListAllocator::SetMoveCallback(std::function<void(MoveData&)> move)
{
	m_move = move;
}

void ListAllocator::Defragment()
{
	for (int i = m_headers.size() - 1; i >= 0; --i)
	{
		if (m_headers[i].Id == 0)
		{
			m_headers.erase(m_headers.begin() + i);
			--i;
		}
	}

	int pointer = 0;	
	MoveData mv;
	for (ListEntryHeader& h : m_headers)
	{
		mv.SrcIndex = h.Pointer;
		mv.Size = h.Size;

		int alignmentPad = pointer % h.Alignment;
		int newPtr = pointer + alignmentPad + h.Size;

		h.PaddingSize = alignmentPad;
		mv.DestIndex = h.Pointer = pointer + alignmentPad;

		m_pointers[h.Id] = &h;
		pointer = newPtr;

		m_move(mv);
	}
	m_pointer = pointer;
}
