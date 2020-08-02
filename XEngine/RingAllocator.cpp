#include "pch.h"
#include "RingAllocator.h"

RingAllocator::RingAllocator(int32_t size) : m_size(size), m_validRangeBegin(0), m_validRangeEnd(0)
{
}

int32_t RingAllocator::AllocateMemory(int32_t size, int32_t alignment, bool overwrite)
{
	int32_t padding = m_validRangeEnd % alignment;

	int32_t ptr = m_validRangeEnd + padding;
	int32_t newRangeEnd = ptr + size;

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

void RingAllocator::Remove(int32_t size, int32_t alignment)
{
	int32_t padding = m_validRangeBegin % alignment;

	int32_t ptr = m_validRangeBegin + padding;
	int32_t newRangeEnd = ptr + size;

	if (newRangeEnd >= m_size)
		m_validRangeBegin = size;
	else
		m_validRangeBegin = newRangeEnd;
}
