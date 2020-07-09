#include "pch.h"
#include "RingAllocator.h"

RingAllocator::RingAllocator(int size) : m_size(size), m_validRangeBegin(0), m_validRangeEnd(0)
{
}

int RingAllocator::AllocateMemory(int size, int alignment, bool overwrite)
{
	int padding = m_validRangeEnd % alignment;

	int ptr = m_validRangeEnd + padding;
	int newRangeEnd = ptr + size;

	if ((m_validRangeEnd < m_validRangeBegin) && (newRangeEnd >= m_validRangeBegin) && !overwrite)
		return -1;
	
	if (newRangeEnd >= m_size)
	{
		if (size > m_validRangeBegin && !overwrite)
			return -1;
		ptr = 0;
		newRangeEnd = size;
	}

	if (overwrite && m_validRangeEnd > m_validRangeBegin)
		m_validRangeBegin = m_validRangeEnd;

	m_validRangeEnd = newRangeEnd;
	return ptr;
}

void RingAllocator::Remove(int size, int alignment)
{
	int padding = m_validRangeBegin % alignment;

	int ptr = m_validRangeBegin + padding;
	int newRangeEnd = ptr + size;

	if (newRangeEnd >= m_size)
		m_validRangeBegin = size;
	else
		m_validRangeBegin = newRangeEnd;
}
