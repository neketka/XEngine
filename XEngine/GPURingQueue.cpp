#include "pch.h"
#include "GPURingQueue.h"

GPUUploadRingQueue::GPUUploadRingQueue(int32_t mipSize) : m_allocator(mipSize), m_size(mipSize)
{
	m_context = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();
	m_mappedRingBuffer = m_context->CreateBuffer(mipSize, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::Coherent | GraphicsMemoryTypeBit::HostVisible);
	m_mappedRingBuffer->MapBuffer(0, m_size, true, true);
}

GPUUploadRingQueue::~GPUUploadRingQueue()
{
	m_mappedRingBuffer->UnmapBuffer();
	delete m_mappedRingBuffer;
}

void FlipYCpy(int32_t rowLen, int32_t rowCount, void *src, void *dest)
{
	for (int32_t slice = 1; slice <= rowCount; ++slice)
	{
		std::memcpy(reinterpret_cast<char *>(dest) + rowLen * (slice - 1), reinterpret_cast<char *>(src) + (rowCount - slice) * rowLen, rowLen);
	}
}

void GPUUploadRingQueue::CopyFromTo(ImageType type, void *dest, void *src, int32_t dataSize, int32_t bytesPerPixel, glm::ivec3 mipSize)
{
	int32_t i = 0;
	int32_t ptr = 0;
	int32_t stride = 0;
	switch (type)
	{
	case ImageType::Image1D:
	case ImageType::Image1DArray:
		std::memcpy(reinterpret_cast<char *>(dest), src, dataSize);
		break;
	case ImageType::Image2D:
		if (m_context->IsOpenGLTextureFlipYConvention())
			FlipYCpy(bytesPerPixel * mipSize.x, mipSize.y, src, reinterpret_cast<char *>(dest));
		else
			std::memcpy(reinterpret_cast<char *>(dest), src, dataSize);
		break;
	case ImageType::Image3D:
		if (m_context->IsOpenGLTextureFlipYConvention())
			FlipYCpy(bytesPerPixel * mipSize.x * mipSize.z, mipSize.y, src, reinterpret_cast<char *>(dest));
		else
			std::memcpy(reinterpret_cast<char *>(dest), src, dataSize);
		break;
	case ImageType::Image2DArray:
		stride = mipSize.x * mipSize.y * bytesPerPixel;
		if (m_context->IsOpenGLTextureFlipYConvention())
		{
			for (; i < mipSize.x; ++i)
			{
				FlipYCpy(bytesPerPixel * mipSize.x, mipSize.y, reinterpret_cast<char *>(src) + ptr, reinterpret_cast<char *>(dest) + ptr);
				ptr += stride;
			}
		}
		else
			std::memcpy(reinterpret_cast<char *>(dest), src, dataSize);
		break;
	case ImageType::ImageCubemap:
	case ImageType::ImageCubemapArray:
		stride = mipSize.x * mipSize.y * bytesPerPixel;
		for (int i = 0; i < mipSize.z / 6; ++i)
		{
			if (m_context->IsOpenGLTextureFlipYConvention())
			{
				FlipYCpy(bytesPerPixel * mipSize.x, mipSize.y, reinterpret_cast<char *>(src), reinterpret_cast<char *>(dest) +
					static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Back)) * stride);
				ptr += stride;
				FlipYCpy(bytesPerPixel * mipSize.x, mipSize.y, reinterpret_cast<char *>(src) + ptr, reinterpret_cast<char *>(dest) +
					static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Front)) * stride);
				ptr += stride;
				FlipYCpy(bytesPerPixel * mipSize.x, mipSize.y, reinterpret_cast<char *>(src) + ptr, reinterpret_cast<char *>(dest) +
					static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Left)) * stride);
				ptr += stride;
				FlipYCpy(bytesPerPixel * mipSize.x, mipSize.y, reinterpret_cast<char *>(src) + ptr, reinterpret_cast<char *>(dest) +
					static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Right)) * stride);
				ptr += stride;
				FlipYCpy(bytesPerPixel * mipSize.x, mipSize.y, reinterpret_cast<char *>(src) + ptr, reinterpret_cast<char *>(dest) +
					static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Bottom)) * stride);
				ptr += stride;
				FlipYCpy(bytesPerPixel * mipSize.x, mipSize.y, reinterpret_cast<char *>(src) + ptr, reinterpret_cast<char *>(dest) +
					static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Top)) * stride);
			}
			else
			{
				std::memcpy(reinterpret_cast<char *>(dest) + static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Back)) * stride,
					reinterpret_cast<char *>(src) + ptr, dataSize);
				ptr += stride;
				std::memcpy(reinterpret_cast<char *>(dest) + static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Front)) * stride,
					reinterpret_cast<char *>(src) + ptr, dataSize);
				ptr += stride;
				std::memcpy(reinterpret_cast<char *>(dest) + static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Left)) * stride,
					reinterpret_cast<char *>(src) + ptr, dataSize);
				ptr += stride;
				std::memcpy(reinterpret_cast<char *>(dest) + static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Right)) * stride,
					reinterpret_cast<char *>(src) + ptr, dataSize);
				ptr += stride;
				std::memcpy(reinterpret_cast<char *>(dest) + static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Bottom)) * stride,
					reinterpret_cast<char *>(src) + ptr, dataSize);
				ptr += stride;
				std::memcpy(reinterpret_cast<char *>(dest) + static_cast<int64_t>(m_context->ConvertCubemapFaceToLayer(CubemapFace::Top)) * stride,
					reinterpret_cast<char *>(src) + ptr, dataSize);
			}
			ptr += stride;
		}
		break;
	}
}

int32_t GPUUploadRingQueue::PopUntilEmptyOrEnoughSize(int32_t mipSize)
{
	int32_t loc = m_allocator.AllocateMemory(mipSize, 1, false);
	while (loc < 0 && !m_tasks.empty())
	{
		GPUTransferTaskWithSync& task = m_tasks.front();
		task.Sync->Wait(1e12);
		m_allocator.Remove(task.Size, 1);
		m_tasks.pop();
		loc = m_allocator.AllocateMemory(mipSize, 1, false);
	}

	if (m_size <= mipSize)
	{
		m_size = mipSize;
		m_allocator = RingAllocator(mipSize + 1);
		m_mappedRingBuffer->UnmapBuffer();
		delete m_mappedRingBuffer;
		m_mappedRingBuffer = m_context->CreateBuffer(mipSize, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::Coherent | 
			GraphicsMemoryTypeBit::HostVisible);
		m_mappedRingBuffer->MapBuffer(0, m_size, true, true);

		loc = m_allocator.AllocateMemory(mipSize, 1, false);
	}

	return loc;
}

GPUDownloadRingQueue::GPUDownloadRingQueue(int32_t mipSize) : m_allocator(mipSize), m_size(mipSize)
{
	m_context = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();
	m_mappedRingBuffer = m_context->CreateBuffer(mipSize, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::Coherent | GraphicsMemoryTypeBit::HostVisible);
	m_mappedRingBuffer->MapBuffer(0, m_size, true, true);
}

GPUDownloadRingQueue::~GPUDownloadRingQueue()
{
	m_mappedRingBuffer->UnmapBuffer();
	delete m_mappedRingBuffer;
}

void GPUDownloadRingQueue::DownloadNow(std::shared_ptr<GraphicsSyncObject> sync)
{
	std::lock_guard lock(m_ringMutex);
	auto iter = std::find(m_downloaded.begin(), m_downloaded.end(), sync.get());
	if (iter != m_downloaded.end())
	{
		m_downloaded.erase(iter);
		return;
	}

	while (!m_tasks.empty())
	{
		GPUDownloadTaskWithSync& task = m_tasks.front();
		task.Sync->Wait(1e12);

		std::memcpy(task.Dest, reinterpret_cast<char *>(m_mappedRingBuffer->GetMappedPointer()) + task.Loc, task.Size);
		
		if (task.Sync.get() == sync.get())
			return;

		m_downloaded.push_back(task.Sync.get());

		m_allocator.Remove(task.Size, 1);
		m_tasks.pop();
	}
}

int32_t GPUDownloadRingQueue::PopUntilEmptyOrEnoughSize(int32_t mipSize)
{
	while (!m_tasks.empty())
	{
		GPUDownloadTaskWithSync& task = m_tasks.front();
		if (task.Sync->GetCurrentStatus())
		{
			std::memcpy(task.Dest, reinterpret_cast<char *>(m_mappedRingBuffer->GetMappedPointer()) + task.Loc, task.Size);
			m_downloaded.push_back(task.Sync.get());
			m_tasks.pop();
		}
	}

	int32_t loc = m_allocator.AllocateMemory(mipSize, 1, false);

	while (loc < 0 && !m_tasks.empty())
	{
		GPUDownloadTaskWithSync& task = m_tasks.front();
		task.Sync->Wait(1e12);
		m_downloaded.push_back(task.Sync.get());

		std::memcpy(task.Dest, reinterpret_cast<char *>(m_mappedRingBuffer->GetMappedPointer()) + task.Loc, task.Size);

		m_allocator.Remove(task.Size, 1);
		m_tasks.pop();
		loc = m_allocator.AllocateMemory(mipSize, 1, false);
	}

	if (m_size <= mipSize)
	{
		m_size = mipSize;
		m_allocator = RingAllocator(mipSize + 1);
		m_mappedRingBuffer->UnmapBuffer();
		delete m_mappedRingBuffer;
		m_mappedRingBuffer = m_context->CreateBuffer(mipSize, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::Coherent |
			GraphicsMemoryTypeBit::HostVisible);
		m_mappedRingBuffer->MapBuffer(0, m_size, true, true);

		loc = m_allocator.AllocateMemory(mipSize, 1, false);
	}

	return loc;
}
