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
	GPUTransferTaskWithSync(std::shared_ptr<GraphicsSyncObject> sync, int32_t size) : Sync(sync), Size(size) {}
	std::shared_ptr<GraphicsSyncObject> Sync;
	int32_t Size;
};

class GPUDownloadTaskWithSync
{
public:
	GPUDownloadTaskWithSync(std::shared_ptr<GraphicsSyncObject> sync, void *dest, int32_t loc, int32_t size) : Sync(sync), Loc(loc), Size(size), Dest(dest) {}
	std::shared_ptr<GraphicsSyncObject> Sync;
	int32_t Loc;
	void *Dest;
	int32_t Size;
};

class GPUUploadRingQueue
{
public:
	XENGINEAPI GPUUploadRingQueue(int32_t size);
	XENGINEAPI ~GPUUploadRingQueue();
	template<class T>
	std::shared_ptr<GraphicsSyncObject> Upload(GraphicsCommandBuffer *cmdBuffer, T *src, PinnedGPUMemory dest,
		int32_t srcOffset, int32_t destOffset, int32_t size)
	{
		std::lock_guard lock(m_ringMutex);

		int32_t loc = PopUntilEmptyOrEnoughSize(size * sizeof(T));

		std::memcpy(reinterpret_cast<char *>(m_mappedRingBuffer->GetMappedPointer()) + loc, src + srcOffset, size * sizeof(T));
		cmdBuffer->CopyBufferToBuffer(m_mappedRingBuffer, dest.GetBuffer(), loc, dest.GetPointer()->Pointer + destOffset, size * sizeof(T));
		std::shared_ptr<GraphicsSyncObject> obj = std::shared_ptr<GraphicsSyncObject>(m_context->CreateSync(false));
		cmdBuffer->SignalFence(obj.get());

		m_tasks.push(GPUTransferTaskWithSync(obj, size * sizeof(T)));
		return obj;
	}

	template<class T>
	std::shared_ptr<GraphicsSyncObject> Upload(GraphicsCommandBuffer *cmdBuffer, T *src, ImageType type, GraphicsImageObject *dest,
		GraphicsBufferImageCopyRegion region, int32_t size, VectorDataFormat format)
	{
		std::lock_guard lock(m_ringMutex);

		int32_t loc = PopUntilEmptyOrEnoughSize(size * sizeof(T));
		
		int32_t bpp = size / (region.Size.x * region.Size.y * region.Size.z);
		CopyFromTo(type, reinterpret_cast<char *>(m_mappedRingBuffer->GetMappedPointer()) + loc, src + region.BufferOffset, size * sizeof(T),
			bpp, region.Size);
		region.BufferOffset = loc;
		cmdBuffer->CopyBufferToImageWithConversion(m_mappedRingBuffer, format, dest, { region });
		std::shared_ptr<GraphicsSyncObject> obj = std::shared_ptr<GraphicsSyncObject>(m_context->CreateSync(false));
		cmdBuffer->SignalFence(obj.get());

		m_tasks.push(GPUTransferTaskWithSync(obj, size * sizeof(T)));

		return obj;
	}

	void CopyFromTo(ImageType type, void *dest, void *src, int32_t dataSize, int32_t bytesPerPixel, glm::ivec3 mipSize);
private:
	GraphicsContext *m_context;

	XENGINEAPI int32_t PopUntilEmptyOrEnoughSize(int32_t size);
	std::mutex m_ringMutex;

	RingAllocator m_allocator;
	GraphicsMemoryBuffer *m_mappedRingBuffer;

	std::queue<GPUTransferTaskWithSync> m_tasks;

	int32_t m_size;
};

class GPUDownloadRingQueue
{
public:
	XENGINEAPI GPUDownloadRingQueue(int32_t size);
	XENGINEAPI ~GPUDownloadRingQueue();

	template<class T>
	std::shared_ptr<GraphicsSyncObject> Download(GraphicsCommandBuffer *cmdBuffer, PinnedGPUMemory src, int32_t srcByteOffset,
		T *dest, int32_t destOffset, int32_t size)
	{
		std::lock_guard lock(m_ringMutex);

		int32_t loc = PopUntilEmptyOrEnoughSize(size * sizeof(T));
		cmdBuffer->CopyBufferToBuffer(src.GetBuffer(), m_mappedRingBuffer, srcByteOffset, loc, size * sizeof(T));
		std::shared_ptr<GraphicsSyncObject> obj = std::shared_ptr<GraphicsSyncObject>(m_context->CreateSync(false));
		cmdBuffer->SignalFence(obj.get());

		m_tasks.push(GPUDownloadTaskWithSync(obj, dest + destOffset, loc, size * sizeof(T)));
	}

	template<class T>
	std::shared_ptr<GraphicsSyncObject> Download(GraphicsCommandBuffer *cmdBuffer, ImageType type, GraphicsImageObject *src, 
		GraphicsBufferImageCopyRegion region, T *dest, int32_t size)
	{
		std::lock_guard lock(m_ringMutex);

		int32_t off = region.BufferOffset;
		int32_t loc = PopUntilEmptyOrEnoughSize(size * sizeof(T));
		region.BufferOffset = loc;

		cmdBuffer->CopyImageToBuffer(src, m_mappedRingBuffer, { region });
		std::shared_ptr<GraphicsSyncObject> obj = std::shared_ptr<GraphicsSyncObject>(m_context->CreateSync(false));
		cmdBuffer->SignalFence(obj.get());

		m_tasks.push(GPUDownloadTaskWithSync(obj, dest + off, loc, size * sizeof(T)));

		return obj;
	}

	XENGINEAPI void DownloadNow(std::shared_ptr<GraphicsSyncObject> sync);
private:
	GraphicsContext *m_context;

	XENGINEAPI int32_t PopUntilEmptyOrEnoughSize(int32_t size);
	std::mutex m_ringMutex;

	RingAllocator m_allocator;
	GraphicsMemoryBuffer *m_mappedRingBuffer;

	std::queue<GPUDownloadTaskWithSync> m_tasks;
	std::vector<GraphicsSyncObject *> m_downloaded;

	int32_t m_size;
};
