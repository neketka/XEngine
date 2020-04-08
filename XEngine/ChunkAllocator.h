#pragma once
#include <vector>
#include <map>
#include <mutex>

#include "exports.h"

using MemoryChunkObjectPointer = long long;

class MemoryChunkObject
{
public:
	void *Memory; 
	MemoryChunkObjectPointer Pointer; // Pointer within the lookup table
	int IntrachunkIndex; // Index within a chunk
	int ChunkIndex; // Index of the chunk
	int AllocCount; // Amount of times allocated
};

class MemoryChunk
{
public:
	void *Memory = nullptr;
	int Index = 0;
	int ObjectCount = 0;
	std::vector<MemoryChunkObject> Objects;
};

class MemoryChunkAllocator
{
public:
	XENGINEAPI MemoryChunkAllocator(int objectsPerChunk, int bytesPerObject);
	XENGINEAPI void CleanupAllocator();
	XENGINEAPI MemoryChunkObjectPointer AllocateObject(); // Allocate an empty, new object
	XENGINEAPI void FreeObject(MemoryChunkObjectPointer obj); // Free an object from a chunk
	XENGINEAPI void *GetObjectMemory(MemoryChunkObjectPointer ptr); // Get the raw memory of an object
	XENGINEAPI void SetBufferedChunkCount(int count); // Set the amount of chunks that should be empty whenever all chunks fill up (performance improvement until more need to be allocated)
	XENGINEAPI std::vector<MemoryChunk>& GetAllChunks(); // Get all the chunks
	XENGINEAPI int GetActiveChunkCount();
	inline int GetPerObjectSize() { return m_bytesPerObject; } // Get the size of an individual object
private:
	std::mutex *m_mutex;

	std::map<long long, MemoryChunkObject> m_objectIndirectionTable; // Table used to keep pointers valid whenever an object swaps spots with another
	std::vector<MemoryChunk> m_allChunks;
	int m_fullChunks;
	int m_chunkCount; // Amount of usable chunks (non-empty)
	int m_bufferedCount; // Amount of empty chunks needed

	void AllocateNewChunk();

	int m_objectsPerChunk;
	int m_bytesPerObject;
};