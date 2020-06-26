#pragma once

#include "exports.h"
#include "GraphicsDefs.h"
#include "ListAllocator.h"
#include "GPUMemoryAllocator.h"
#include "RingAllocator.h"
#include "LocalMemoryAllocator.h"

#include <atomic>
#include <mutex>
#include <queue>

class GPUUploadRingQueue;

class GPUTransferTaskWithSync
{
public:
	GPUTransferTaskWithSync(std::shared_ptr<GraphicsSyncObject> sync, int size) : Sync(sync), Size(size) {}
	std::shared_ptr<GraphicsSyncObject> Sync;
	int Size;
};

class GPUUploadRingQueue
{
public:
	XENGINEAPI GPUUploadRingQueue(int size);
	XENGINEAPI ~GPUUploadRingQueue();
	template<class T>
	std::shared_ptr<GraphicsSyncObject> Upload(GraphicsCommandBuffer *cmdBuffer, PinnedLocalMemory<T> src, PinnedGPUMemory dest,
		int srcOffset, int destOffset, int size)
	{
		m_ringMutex.lock();
		int loc = PopUntilEmptyOrEnoughSize(size * sizeof(T));

		std::memcpy(m_mappedRingBuffer->GetMappedPointer() + loc, src.GetData() + srcOffset, src.GetSize() * sizeof(T));
		cmdBuffer->CopyBufferToBuffer(m_mappedRingBuffer, dest.GetBuffer(), loc, dest.GetPointer()->Pointer + destOffset, size * sizeof(T));
		std::shared_ptr<GraphicsSyncObject> obj = m_context->CreateSync(false);
		cmdBuffer->SignalFence(obj);

		m_ringMutex.unlock();

		m_tasks.push(GPUTransferTaskWithSync(obj, size * sizeof(T)));

		return obj;
	}

	template<class T>
	std::shared_ptr<GraphicsSyncObject> Upload(GraphicsCommandBuffer *cmdBuffer, PinnedLocalMemory<T> src, GraphicsImageObject dest,
		GraphicsBufferImageCopyRegion region, int size, VectorDataFormat format)
	{
		m_ringMutex.lock();
		int loc = PopUntilEmptyOrEnoughSize(size * sizeof(T));

		std::memcpy(m_mappedRingBuffer->GetMappedPointer() + loc, src.GetData() + region.BufferOffset, src.GetSize() * sizeof(T));
		region.BufferOffset = loc;
		cmdBuffer->CopyBufferToImageWithConversion(m_mappedRingBuffer, format, dest, { region });
		std::shared_ptr<GraphicsSyncObject> obj = m_context->CreateSync(false);
		cmdBuffer->SignalFence(obj);

		m_ringMutex.unlock();

		m_tasks.push(GPUTransferTaskWithSync(obj, size * sizeof(T)));

		return obj;
	}
private:
	GraphicsContext *m_context;

	XENGINEAPI int PopUntilEmptyOrEnoughSize(int size);
	std::mutex m_ringMutex;

	RingAllocator m_allocator;
	GraphicsMemoryBuffer *m_mappedRingBuffer;

	std::queue<GPUTransferTaskWithSync> m_tasks;

	int m_size;
};

class GPUDownloadRingQueue
{
};
