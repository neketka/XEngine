#pragma once

#include <atomic>

class RingAllocator
{
public:
	RingAllocator(int size);
	int AllocateMemory(int size, int alignment, bool overwrite);
	void Remove(int size, int alignment);
private:
	int m_size;
	int m_validRangeBegin;
	int m_validRangeEnd;
};

