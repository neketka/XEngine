#pragma once

#include "exports.h"
#include "GraphicsDefs.h"
#include "ListAllocator.h"

#include <atomic>
#include <mutex>
#include <queue>

class PinnedGPUMemory
{
public:
	PinnedGPUMemory() {}
	PinnedGPUMemory(std::shared_ptr<PinnedListMemory> mem, GraphicsMemoryBuffer *buffer, int size) 
		: m_buffer(buffer), m_mem(mem), m_size(size) {}
	GraphicsMemoryBuffer *GetBuffer() { return m_buffer; }
	ListMemoryPointer *GetPointer() { return m_mem->GetPointer(); }
	int GetSize() { return m_size; }
	void Unpin() { m_mem->Unpin(); }
private:
	GraphicsMemoryBuffer *m_buffer;
	int m_size;
	std::shared_ptr<PinnedListMemory> m_mem;
};

class GPUMemoryAllocator
{
public:
	XENGINEAPI GPUMemoryAllocator(unsigned long long size, int maxAllocs, bool resizable);
	XENGINEAPI ~GPUMemoryAllocator();
	XENGINEAPI bool WillFit(int size);
	XENGINEAPI PinnedGPUMemory GetMemory(ListMemoryPointer *ptr);
	XENGINEAPI ListMemoryPointer *RequestSpace(int bytes, int alignment = 1);
	XENGINEAPI GraphicsMemoryBuffer *GetMemoryBuffer();
	XENGINEAPI void FreeSpace(ListMemoryPointer *ptr);
private:
	void DefragBegin();
	void DefragEnd();
	void MoveMemory(MoveData& data);
	
	std::shared_mutex m_resizeMutex;

	GraphicsContext *m_context;

	GraphicsCommandBuffer *thisBuf;

	GraphicsMemoryBuffer *m_memory;
	GraphicsMemoryBuffer *m_temp;
	unsigned long long m_size;
	int m_tempSize;
	ListAllocator m_allocator;
	bool m_resizable;
};
