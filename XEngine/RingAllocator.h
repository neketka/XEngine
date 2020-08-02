#pragma once

#include <atomic>

class RingAllocator
{
public:
	RingAllocator(int32_t size);
	int32_t AllocateMemory(int32_t size, int32_t alignment, bool overwrite);
	void Remove(int32_t size, int32_t alignment);
private:
	int32_t m_size;
	int32_t m_validRangeBegin;
	int32_t m_validRangeEnd;
};

