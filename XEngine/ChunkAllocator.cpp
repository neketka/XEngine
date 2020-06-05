#include "pch.h"
#include "ChunkAllocator.h"

#include <memory>

// Initialize the variables
MemoryChunkAllocator::MemoryChunkAllocator(int objectsPerChunk, int bytesPerObject) : m_objectsPerChunk(objectsPerChunk), m_bytesPerObject(bytesPerObject), 
	m_bufferedCount(0), m_chunkCount(0), m_fullChunks(0)
{ 
	m_mutex = new std::mutex;
}

void MemoryChunkAllocator::CleanupAllocator()
{
	for (MemoryChunk& c : m_allChunks)
		std::free(c.Memory); // Free all chunk memory
	delete m_mutex;
}

MemoryChunkObjectPointer MemoryChunkAllocator::AllocateObject()
{
	m_mutex->lock();
	if (m_fullChunks == m_allChunks.size()) // If all chunks are full
	{
		for (int i = 0; i < m_bufferedCount + 1; ++i) // Add empty chunks for future allocations and this one
			AllocateNewChunk();
		++m_chunkCount; // Increase the amount of usable chunks
	}
	else if (m_fullChunks == m_chunkCount) // If the usable chunks are all full
	{
		++m_chunkCount;
	}

	MemoryChunk& chunk = m_allChunks[m_chunkCount - 1]; // Get last chunk
	MemoryChunkObject& obj = chunk.Objects[chunk.ObjectCount]; // Get next object slot

	/* No clue what this does
	if (chunk.ObjectCount == 0)
		chunk.Index = chunk.ObjectCount;*/

	++chunk.ObjectCount; // Increase object count

	if (chunk.ObjectCount == m_objectsPerChunk) // If there is no more space in this chunk
		++m_fullChunks; // Mark this chunk as full

	MemoryChunkObjectPointer ptr = ((chunk.Index * static_cast<long long>(m_objectsPerChunk) + chunk.ObjectCount) << 32) | obj.AllocCount; // Make a unique pointer to object

	m_objectIndirectionTable[ptr] = &obj; // Add this pointer as being a pointer to this object

	m_mutex->unlock();
	return ptr;
}

void MemoryChunkAllocator::FreeObject(MemoryChunkObjectPointer ptr)
{
	m_mutex->lock();
	MemoryChunkObject& obj = *m_objectIndirectionTable[ptr]; // Fetch the object
	MemoryChunk *chunk = &m_allChunks[obj.ChunkIndex]; // Fetch the chunk of the object
	if (obj.ChunkIndex == m_chunkCount - 1 && chunk->ObjectCount == obj.IntrachunkIndex + 1) // Last object of last chunk
	{
		if (chunk->ObjectCount == m_objectsPerChunk)
			--m_fullChunks; // This chunk is no longer full due to freed object
		--chunk->ObjectCount;
	}
	else // Object in any chunk
	{
		chunk = &m_allChunks[m_chunkCount - 1]; // Redeclare chunk as the last chunk
		if (chunk->ObjectCount == m_objectsPerChunk)
			--m_fullChunks;
		--chunk->ObjectCount; 
		
		MemoryChunkObject& lastObj = m_allChunks[chunk->Index].Objects[chunk->ObjectCount]; // Last object of last chunk
		
		m_objectIndirectionTable[lastObj.Pointer] = &obj; // Redeclare the pointer for the last object
		obj.Pointer = lastObj.Pointer; // Set the pointer correctly

		std::memcpy(obj.Memory, lastObj.Memory, m_bytesPerObject); // Copy the last object's memory
	}

	m_objectIndirectionTable.erase(ptr);
	if (chunk->ObjectCount == 0) // If the chunk is now empty
	{
		--m_chunkCount;
		std::memset(chunk->Memory, 0, m_objectsPerChunk * m_bytesPerObject); // Clear the chunk memory
		if (m_allChunks.size() - m_chunkCount > m_bufferedCount) // If the "buffered" boundary is overstepped (too many empty chunks around)
		{
			std::free(m_allChunks.back().Memory);
			m_allChunks.pop_back();
		}
	}
	m_mutex->unlock();
}

void *MemoryChunkAllocator::GetObjectMemory(MemoryChunkObjectPointer ptr)
{
	m_mutex->lock();
	auto val = m_objectIndirectionTable[ptr]->Memory;
	m_mutex->unlock();
	return val;
}

void MemoryChunkAllocator::SetBufferedChunkCount(int count)
{
	m_mutex->lock();
	m_bufferedCount = count;
	for (int i = 0; i < m_bufferedCount - (m_allChunks.size() - m_chunkCount); ++i) // Allocate the amount of chunks needed to meet the empty chunk requirement
		AllocateNewChunk();
	for (int i = 0; i < (m_allChunks.size() - m_chunkCount) - m_bufferedCount; ++i) // Delete the chunks if there are too many empty ones
	{
		std::free(m_allChunks.back().Memory);
		m_allChunks.pop_back();
	}
	m_mutex->unlock();
}

std::vector<MemoryChunk>& MemoryChunkAllocator::GetAllChunks()
{
	return m_allChunks;
}

int MemoryChunkAllocator::GetActiveChunkCount()
{
	return m_chunkCount;
}

void MemoryChunkAllocator::AllocateNewChunk()
{
	m_allChunks.push_back(MemoryChunk());
	MemoryChunk& chunk = m_allChunks.back();
	chunk.Index = m_allChunks.size() - 1; // Get the last chunk index
	chunk.ObjectCount = 0;
	chunk.Memory = std::calloc(m_objectsPerChunk + 1, m_bytesPerObject); // Allocate chunk memory with 0s in all bytes
	/*
	size_t space = (m_objectsPerChunk + 1) * m_bytesPerObject;
	chunk.Memory = std::align(16, m_objectsPerChunk * m_bytesPerObject, chunk.Memory, space); // Align for SIMD
	*/
	for (int i = 0; i < m_objectsPerChunk; ++i)
	{
		chunk.Objects.push_back(MemoryChunkObject());
		MemoryChunkObject& obj = chunk.Objects.back();
		obj.IntrachunkIndex = i;
		obj.ChunkIndex = chunk.Index;
		obj.Memory = reinterpret_cast<char *>(chunk.Memory) + m_bytesPerObject * i;
		obj.AllocCount = 0;
		obj.Pointer = 0;
	}
}
