#pragma once
#include <vector>
#include <map>

#include "exports.h"

using MemoryChunkObjectPointer = long long;

class MemoryChunkObject
{
public:
	void *Memory;
	MemoryChunkObjectPointer Pointer;
	int IntrachunkIndex;
	int ChunkIndex;
	int AllocCount;
};

class MemoryChunk
{
public:
	void *Memory;
	int Index;
	int ObjectCount;
	std::vector<MemoryChunkObject> Objects;
};

class MemoryChunkAllocator
{
public:
	XENGINEAPI MemoryChunkAllocator(int objectsPerChunk, int bytesPerObject);
	XENGINEAPI ~MemoryChunkAllocator();
	XENGINEAPI MemoryChunkObjectPointer AllocateObject();
	XENGINEAPI void FreeObject(MemoryChunkObjectPointer obj);
	XENGINEAPI void *GetObjectMemory(MemoryChunkObjectPointer ptr);
	XENGINEAPI void SetBufferedChunkCount(int count);
	XENGINEAPI std::vector<MemoryChunk>& GetAllChunks();
private:
	std::map<long long, MemoryChunkObject> m_objectIndirectionTable;
	std::vector<MemoryChunk> m_allChunks;
	int m_fullChunks;
	int m_chunkCount;
	int m_bufferedCount;

	void AllocateNewChunk();

	int m_objectsPerChunk;
	int m_bytesPerObject;
};