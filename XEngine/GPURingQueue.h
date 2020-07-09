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

class GPUDownloadTaskWithSync
{
public:
	GPUDownloadTaskWithSync(std::shared_ptr<GraphicsSyncObject> sync, void *dest, int loc, int size) : Sync(sync), Loc(loc), Size(size), Dest(dest) {}
	std::shared_ptr<GraphicsSyncObject> Sync;
	int Loc;
	void *Dest;
	int Size;
};

class GPUUploadRingQueue
{
public:
	XENGINEAPI GPUUploadRingQueue(int size);
	XENGINEAPI ~GPUUploadRingQueue();
	template<class T>
	std::shared_ptr<GraphicsSyncObject> Upload(GraphicsCommandBuffer *cmdBuffer, T *src, PinnedGPUMemory dest,
		int srcOffset, int destOffset, int size)
	{
		std::lock_guard<std::mutex> lock(m_ringMutex);

		int loc = PopUntilEmptyOrEnoughSize(size * sizeof(T));

		std::memcpy(reinterpret_cast<char *>(m_mappedRingBuffer->GetMappedPointer()) + loc, src + srcOffset, size * sizeof(T));
		cmdBuffer->CopyBufferToBuffer(m_mappedRingBuffer, dest.GetBuffer(), loc, dest.GetPointer()->Pointer + destOffset, size * sizeof(T));
		std::shared_ptr<GraphicsSyncObject> obj = std::shared_ptr<GraphicsSyncObject>(m_context->CreateSync(false));
		cmdBuffer->SignalFence(obj.get());

		m_tasks.push(GPUTransferTaskWithSync(obj, size * sizeof(T)));
		return obj;
	}

	template<class T>
	std::shared_ptr<GraphicsSyncObject> Upload(GraphicsCommandBuffer *cmdBuffer, T *src, GraphicsImageObject dest,
		GraphicsBufferImageCopyRegion region, int size, VectorDataFormat format)
	{
		std::lock_guard<std::mutex> lock(m_ringMutex);

		int loc = PopUntilEmptyOrEnoughSize(size * sizeof(T));

		std::memcpy(reinterpret_cast<char *>(m_mappedRingBuffer->GetMappedPointer()) + loc, src + region.BufferOffset, size * sizeof(T));
		region.BufferOffset = loc;
		cmdBuffer->CopyBufferToImageWithConversion(m_mappedRingBuffer, format, dest, { region });
		std::shared_ptr<GraphicsSyncObject> obj = std::shared_ptr<GraphicsSyncObject>(m_context->CreateSync(false));
		cmdBuffer->SignalFence(obj.get());

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
public:
	XENGINEAPI GPUDownloadRingQueue(int size);
	XENGINEAPI ~GPUDownloadRingQueue();

	template<class T>
	std::shared_ptr<GraphicsSyncObject> Download(GraphicsCommandBuffer *cmdBuffer, PinnedGPUMemory src, int srcByteOffset,
		T *dest, int destOffset, int size)
	{
		std::lock_guard<std::mutex> lock(m_ringMutex);

		int loc = PopUntilEmptyOrEnoughSize(size * sizeof(T));
		cmdBuffer->CopyBufferToBuffer(src.GetBuffer(), m_mappedRingBuffer, srcByteOffset, loc, size * sizeof(T));
		std::shared_ptr<GraphicsSyncObject> obj = std::shared_ptr<GraphicsSyncObject>(m_context->CreateSync(false));
		cmdBuffer->SignalFence(obj.get());

		m_tasks.push(GPUDownloadTaskWithSync(obj, dest + destOffset, loc, size * sizeof(T)));
	}

	template<class T>
	std::shared_ptr<GraphicsSyncObject> Download(GraphicsCommandBuffer *cmdBuffer, GraphicsImageObject *src, GraphicsBufferImageCopyRegion region,
		T *dest, int size)
	{
		std::lock_guard<std::mutex> lock(m_ringMutex);

		int off = region.BufferOffset;
		int loc = PopUntilEmptyOrEnoughSize(size * sizeof(T));
		region.BufferOffset = loc;

		cmdBuffer->CopyImageToBuffer(src, m_mappedRingBuffer, { region });
		std::shared_ptr<GraphicsSyncObject> obj = std::shared_ptr<GraphicsSyncObject>(m_context->CreateSync(false));
		cmdBuffer->SignalFence(obj.get());

		m_tasks.push(GPUDownloadTaskWithSync(obj, dest + off, loc, size * sizeof(T)));
	}

	XENGINEAPI void DownloadNow(std::shared_ptr<GraphicsSyncObject> sync);
private:
	GraphicsContext *m_context;

	XENGINEAPI int PopUntilEmptyOrEnoughSize(int size);
	std::mutex m_ringMutex;

	RingAllocator m_allocator;
	GraphicsMemoryBuffer *m_mappedRingBuffer;

	std::queue<GPUDownloadTaskWithSync> m_tasks;
	std::vector<GraphicsSyncObject *> m_downloaded;

	int m_size;
};
