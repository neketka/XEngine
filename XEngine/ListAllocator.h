#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include "UUID.h"

using ListPointer = unsigned __int64;

class ListEntryHeader
{
public:
	ListEntryHeader(UniqueId id, ListPointer ptr, int size, int padding, int alignment) :
		Id(id), Pointer(ptr), Size(size), PaddingSize(padding), Alignment(alignment) {}
	UniqueId Id;
	ListPointer Pointer;
	int Size;
	short PaddingSize;
	short Alignment;
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

	UniqueId AllocateMemory(int size, int alignment);
	void DeallocateMemory(UniqueId block);
	ListPointer GetPointer(UniqueId block);

	void SetMoveCallback(std::function<void(MoveData&)> move);
private:
	void Defragment();
	
	std::unordered_map<UniqueId, ListEntryHeader *> m_pointers;
	std::vector<ListEntryHeader> m_headers;

	int m_pointer;

	int m_maxSize;
	int m_maxHeaders;

	std::function<void(MoveData&)> m_move;
};