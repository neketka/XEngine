#include "pch.h"
#include "GPURingQueue.h"

GPUUploadRingQueue::GPUUploadRingQueue(int size) : m_allocator(size)
{
	m_context = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();
	m_mappedRingBuffer = m_context->CreateBuffer(size, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::Coherent | GraphicsMemoryTypeBit::HostVisible);
	m_mappedRingBuffer->MapBuffer(0, m_size, true, true);
}

GPUUploadRingQueue::~GPUUploadRingQueue()
{
	m_mappedRingBuffer->UnmapBuffer();
	delete m_mappedRingBuffer;
	while (!m_tasks.empty())
		m_tasks.pop();
}

int GPUUploadRingQueue::PopUntilEmptyOrEnoughSize(int size)
{
	int loc = m_allocator.AllocateMemory(size, 0, false);

	if (m_size < size)
	{
		m_size = size;
		m_allocator = RingAllocator(size);
		m_mappedRingBuffer->UnmapBuffer();
		delete m_mappedRingBuffer;
		m_mappedRingBuffer = m_context->CreateBuffer(size, BufferUsageBit::TransferSource, GraphicsMemoryTypeBit::Coherent | 
			GraphicsMemoryTypeBit::HostVisible);
		m_mappedRingBuffer->MapBuffer(0, m_size, true, true);
	}
	else
	{
		while (loc < 0)
		{
			GPUTransferTaskWithSync& task = m_tasks.front();
			task.Sync->Wait(1e12);
			m_allocator.Remove(task.Size, 0);
			m_tasks.pop();
			loc = m_allocator.AllocateMemory(size, 0, false);
		}
	}

	return loc;
}
