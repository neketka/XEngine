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
	int32_t IntrachunkIndex; // Index within a chunk
	int32_t ChunkIndex; // Index of the chunk
	int32_t AllocCount; // Amount of times allocated
};

class MemoryChunk
{
public:
	void *Memory = nullptr;
	int32_t Index = 0;
	int32_t ObjectCount = 0;
	std::vector<MemoryChunkObject> Objects;
};

class MemoryChunkAllocator
{
public:
	XENGINEAPI MemoryChunkAllocator(int32_t objectsPerChunk, int32_t bytesPerObject);
	XENGINEAPI void CleanupAllocator();
	XENGINEAPI MemoryChunkObjectPointer AllocateObject(); // Allocate an empty, new object
	XENGINEAPI void FreeObject(MemoryChunkObjectPointer obj); // Free an object from a chunk
	XENGINEAPI void *GetObjectMemory(MemoryChunkObjectPointer ptr); // Get the raw memory of an object
	XENGINEAPI void SetBufferedChunkCount(int32_t count); // Set the amount of chunks that should be empty whenever all chunks fill up (performance improvement until more need to be allocated)
	XENGINEAPI std::vector<MemoryChunk>& GetAllChunks(); // Get all the chunks
	XENGINEAPI int32_t GetActiveChunkCount();
	inline int32_t GetPerObjectSize() { return m_bytesPerObject; } // Get the size of an individual object
private:
	std::mutex *m_mutex;

	std::map<long long, MemoryChunkObject *> m_objectIndirectionTable; // Table used to keep pointers valid whenever an object swaps spots with another
	std::vector<MemoryChunk> m_allChunks;
	int32_t m_fullChunks;
	int32_t m_chunkCount; // Amount of usable chunks (non-empty)
	int32_t m_bufferedCount; // Amount of empty chunks needed

	void AllocateNewChunk();

	int32_t m_objectsPerChunk;
	int32_t m_bytesPerObject;
};