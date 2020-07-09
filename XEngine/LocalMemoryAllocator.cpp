#include "pch.h"

#include "LocalMemoryAllocator.h"
#include "ListAllocator.h"

LocalMemoryAllocator::LocalMemoryAllocator(unsigned long long size, int maxAllocs, bool resizable)
	: m_allocator(size, maxAllocs), m_resizable(resizable), m_size(size)
{
	m_memory = std::malloc(size);
	m_allocator.SetMoveCallback(std::bind(&LocalMemoryAllocator::MoveMemory, this, std::placeholders::_1));
}

LocalMemoryAllocator::~LocalMemoryAllocator()
{
	std::free(m_memory);
}

bool LocalMemoryAllocator::WillFit(int size)
{
	return m_allocator.GetFreeSpace() >= size;
}

ListMemoryPointer *LocalMemoryAllocator::RequestSpace(int bytes, int alignment)
{
	if (!WillFit(bytes + alignment - 1))
		alignment = 1;

	if (!WillFit(bytes) && m_resizable)
	{
	}

	return m_allocator.AllocateMemory(bytes, alignment);
}

void LocalMemoryAllocator::FreeSpace(ListMemoryPointer *ptr)
{
	m_allocator.DeallocateMemory(ptr);
}

void LocalMemoryAllocator::MoveMemory(MoveData& data)
{
	std::memmove(reinterpret_cast<char *>(m_memory) + data.DestIndex, reinterpret_cast<char *>(m_memory) + data.SrcIndex, data.Size);
}