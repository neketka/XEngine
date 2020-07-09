#include "pch.h"
#include "GPURingQueue.h"

GPUUploadRingQueue::GPUUploadRingQueue(int size) : m_allocator(size), m_size(size)
{
	m_context = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();
	m_mappedRingBuffer = m_context->CreateBuffer(size, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::Coherent | GraphicsMemoryTypeBit::HostVisible);
	m_mappedRingBuffer->MapBuffer(0, m_size, true, true);
}

GPUUploadRingQueue::~GPUUploadRingQueue()
{
	m_mappedRingBuffer->UnmapBuffer();
	delete m_mappedRingBuffer;
}

int GPUUploadRingQueue::PopUntilEmptyOrEnoughSize(int size)
{
	int loc = m_allocator.AllocateMemory(size, 1, false);
	while (loc < 0 && !m_tasks.empty())
	{
		GPUTransferTaskWithSync& task = m_tasks.front();
		task.Sync->Wait(1e12);
		m_allocator.Remove(task.Size, 1);
		m_tasks.pop();
		loc = m_allocator.AllocateMemory(size, 1, false);
	}

	if (m_size <= size)
	{
		m_size = size;
		m_allocator = RingAllocator(size + 1);
		m_mappedRingBuffer->UnmapBuffer();
		delete m_mappedRingBuffer;
		m_mappedRingBuffer = m_context->CreateBuffer(size, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::Coherent | 
			GraphicsMemoryTypeBit::HostVisible);
		m_mappedRingBuffer->MapBuffer(0, m_size, true, true);

		loc = m_allocator.AllocateMemory(size, 1, false);
	}

	return loc;
}

GPUDownloadRingQueue::GPUDownloadRingQueue(int size) : m_allocator(size), m_size(size)
{
	m_context = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();
	m_mappedRingBuffer = m_context->CreateBuffer(size, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::Coherent | GraphicsMemoryTypeBit::HostVisible);
	m_mappedRingBuffer->MapBuffer(0, m_size, true, true);
}

GPUDownloadRingQueue::~GPUDownloadRingQueue()
{
	m_mappedRingBuffer->UnmapBuffer();
	delete m_mappedRingBuffer;
}

void GPUDownloadRingQueue::DownloadNow(std::shared_ptr<GraphicsSyncObject> sync)
{
	std::lock_guard<std::mutex> lock(m_ringMutex);
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

int GPUDownloadRingQueue::PopUntilEmptyOrEnoughSize(int size)
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

	int loc = m_allocator.AllocateMemory(size, 1, false);

	while (loc < 0 && !m_tasks.empty())
	{
		GPUDownloadTaskWithSync& task = m_tasks.front();
		task.Sync->Wait(1e12);
		m_downloaded.push_back(task.Sync.get());

		std::memcpy(task.Dest, reinterpret_cast<char *>(m_mappedRingBuffer->GetMappedPointer()) + task.Loc, task.Size);

		m_allocator.Remove(task.Size, 1);
		m_tasks.pop();
		loc = m_allocator.AllocateMemory(size, 1, false);
	}

	if (m_size <= size)
	{
		m_size = size;
		m_allocator = RingAllocator(size + 1);
		m_mappedRingBuffer->UnmapBuffer();
		delete m_mappedRingBuffer;
		m_mappedRingBuffer = m_context->CreateBuffer(size, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::Coherent |
			GraphicsMemoryTypeBit::HostVisible);
		m_mappedRingBuffer->MapBuffer(0, m_size, true, true);

		loc = m_allocator.AllocateMemory(size, 1, false);
	}

	return loc;
}
