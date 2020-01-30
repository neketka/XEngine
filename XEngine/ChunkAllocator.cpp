#include "pch.h"
#include "ChunkAllocator.h"

MemoryChunkAllocator::MemoryChunkAllocator(int objectsPerChunk, int bytesPerObject) : m_objectsPerChunk(objectsPerChunk), m_bytesPerObject(bytesPerObject), 
	m_bufferedCount(0), m_chunkCount(0), m_fullChunks(0)
{
}

MemoryChunkAllocator::~MemoryChunkAllocator()
{
	for (MemoryChunk c : m_allChunks)
		std::free(c.Memory);
}

MemoryChunkObjectPointer MemoryChunkAllocator::AllocateObject()
{
	m_mutex.lock();
	if (m_fullChunks == m_allChunks.size())
	{
		for (int i = 0; i < m_bufferedCount; ++i)
			AllocateNewChunk();
		++m_chunkCount;
	}
	else if (m_fullChunks == m_chunkCount)
	{
		++m_chunkCount;
	}

	MemoryChunk& chunk = m_allChunks[m_chunkCount - 1];
	MemoryChunkObject& obj = chunk.Objects[chunk.ObjectCount];
	if (chunk.ObjectCount == 0)
		chunk.Index = chunk.ObjectCount;
	++chunk.ObjectCount;

	if (chunk.ObjectCount == m_objectsPerChunk)
		++m_fullChunks;

	MemoryChunkObjectPointer ptr = ((chunk.Index * static_cast<long long>(m_objectsPerChunk) + chunk.ObjectCount) << 32) | obj.AllocCount;

	m_objectIndirectionTable[ptr] = obj;

	m_mutex.unlock();
	return ptr;
}

void MemoryChunkAllocator::FreeObject(MemoryChunkObjectPointer ptr)
{
	m_mutex.lock();
	MemoryChunkObject& obj = m_objectIndirectionTable[ptr];
	MemoryChunk *chunk = &m_allChunks[obj.ChunkIndex];
	if (obj.ChunkIndex == m_chunkCount - 1 && chunk->ObjectCount == obj.IntrachunkIndex + 1) // Last object of last chunk
	{
		if (chunk->ObjectCount == m_objectsPerChunk)
			--m_fullChunks;
		--chunk->ObjectCount;
	}
	else // Object in any chunk
	{
		chunk = &m_allChunks[m_chunkCount - 1];
		if (chunk->ObjectCount == m_objectsPerChunk)
			--m_fullChunks;
		--chunk->ObjectCount;

		MemoryChunkObject& lastObj = m_allChunks[chunk->Index].Objects[chunk->ObjectCount];
		
		m_objectIndirectionTable[lastObj.Pointer] = obj;
		obj.Pointer = lastObj.Pointer;

		std::memcpy(obj.Memory, lastObj.Memory, m_bytesPerObject);
	}

	m_objectIndirectionTable.erase(ptr);
	if (chunk->ObjectCount == 0)
	{
		--m_chunkCount;
		std::memset(chunk->Memory, 0, m_objectsPerChunk * m_bytesPerObject);
		if (m_allChunks.size() - m_chunkCount > m_bufferedCount)
		{
			std::free(m_allChunks.back().Memory);
			m_allChunks.pop_back();
		}
	}
	m_mutex.unlock();
}

void *MemoryChunkAllocator::GetObjectMemory(MemoryChunkObjectPointer ptr)
{
	m_mutex.lock();
	auto val = m_objectIndirectionTable[ptr].Memory;
	m_mutex.unlock();
	return val;
}

void MemoryChunkAllocator::SetBufferedChunkCount(int count)
{
	m_mutex.lock();
	m_bufferedCount = count;
	for (int i = 0; i < m_bufferedCount - (m_allChunks.size() - m_chunkCount); ++i)
		AllocateNewChunk();
	for (int i = 0; i < (m_allChunks.size() - m_chunkCount) - m_bufferedCount; ++i)
	{
		std::free(m_allChunks.back().Memory);
		m_allChunks.pop_back();
	}
	m_mutex.unlock();
}

std::vector<MemoryChunk>& MemoryChunkAllocator::GetAllChunks()
{
	return m_allChunks;
}

void MemoryChunkAllocator::AllocateNewChunk()
{
	m_mutex.lock();
	m_allChunks.push_back(MemoryChunk());
	MemoryChunk& chunk = m_allChunks.back();
	chunk.Index = m_allChunks.size() - 1;
	chunk.ObjectCount = 0;
	chunk.Memory = std::calloc(m_objectsPerChunk, m_bytesPerObject);

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
	m_mutex.unlock();
}
