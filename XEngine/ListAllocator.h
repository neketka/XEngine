#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include "UUID.h"

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
	int Size;
	int Alignment;
};

class MoveData
{
public:
	ListPointer SrcIndex;
	ListPointer DestIndex;
	int Size;
};

class ListAllocator
{
public:
	ListAllocator(int size, int maxAllocs);
	~ListAllocator();

	ListMemoryPointer *AllocateMemory(int size, int alignment);
	void DeallocateMemory(ListMemoryPointer *ptr);

	int GetAllocationSize(ListMemoryPointer *ptr);

	void ShrinkToFit(int extraMemory);
	void SetSize(int maxSize);

	int GetMaxSize();
	int GetFreeSpace();

	void SetMoveCallback(std::function<void(MoveData&)> move);
private:
	void Defragment();
	
	std::vector<ListEntryHeader> m_headerLinks;
	std::vector<int> m_freeList;

	ListEntryHeader *m_first = nullptr;
	ListEntryHeader *m_last = nullptr;

	int m_pointer;

	int m_maxSize;
	int m_maxHeaders;
	int m_freeSpace;

	std::function<void(MoveData&)> m_move;
};