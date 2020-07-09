#include "pch.h"
#include "GPUMemoryAllocator.h"

GPUMemoryAllocator::GPUMemoryAllocator(unsigned long long size, int maxAllocs, bool resizable)
	: m_allocator(size, maxAllocs), m_resizable(resizable), m_size(size), m_temp(nullptr), m_tempSize(-1)
{
	m_context = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();

	m_memory = m_context->CreateBuffer(size, BufferUsageBit::TransferSource | BufferUsageBit::TransferDest,
		GraphicsMemoryTypeBit::DeviceResident | GraphicsMemoryTypeBit::DynamicAccess);

	m_allocator.SetMoveCallback(std::bind(&GPUMemoryAllocator::MoveMemory, this, std::placeholders::_1));
	m_allocator.SetDefragBeginCallback(std::bind(&GPUMemoryAllocator::DefragBegin, this));
	m_allocator.SetDefragEndCallback(std::bind(&GPUMemoryAllocator::DefragEnd, this));
}

GPUMemoryAllocator::~GPUMemoryAllocator()
{
	delete m_memory;
	if (m_temp)
		delete m_temp;
}

bool GPUMemoryAllocator::WillFit(int size)
{
	return m_allocator.GetFreeSpace() >= size;
}

PinnedGPUMemory GPUMemoryAllocator::GetMemory(ListMemoryPointer *ptr)
{
	return PinnedGPUMemory(m_allocator.PinMemory(ptr), m_memory, m_allocator.GetAllocationSize(ptr));
}

ListMemoryPointer *GPUMemoryAllocator::RequestSpace(int bytes, int alignment)
{
	m_resizeMutex.lock_shared();
	if (!WillFit(bytes + alignment - 1))
		alignment = 1;

	if (!WillFit(bytes) && m_resizable)
	{
		m_resizeMutex.unlock_shared();
		m_resizeMutex.lock();

		unsigned long long oldSize = m_size;
		unsigned long long newSize = std::exp2l(std::floorl(std::log2l(oldSize)) + 1);

		GraphicsMemoryBuffer *newBuf = m_context->CreateBuffer(newSize, BufferUsageBit::TransferSource | BufferUsageBit::TransferDest,
			GraphicsMemoryTypeBit::DeviceResident | GraphicsMemoryTypeBit::DynamicAccess);

		GraphicsSyncObject *sync = m_context->CreateSync(false);

		GraphicsCommandBuffer *buf = m_context->GetTransferBufferFromPool();
		buf->BeginRecording();
		buf->CopyBufferToBuffer(m_memory, newBuf, 0, 0, oldSize);
		buf->SignalFence(sync);
		buf->StopRecording();

		m_context->SubmitCommands(buf, GraphicsQueueType::Transfer);

		sync->Wait(1e32);
		delete sync;

		delete m_memory;
		m_memory = newBuf;
		m_size = newSize;
		m_allocator.SetSize(newSize);

		m_resizeMutex.unlock();
		m_resizeMutex.lock_shared();
	}

	m_resizeMutex.unlock_shared();
	return m_allocator.AllocateMemory(bytes, alignment);
}

GraphicsMemoryBuffer *GPUMemoryAllocator::GetMemoryBuffer()
{
	return m_memory;
}

void GPUMemoryAllocator::FreeSpace(ListMemoryPointer *ptr)
{
	m_allocator.DeallocateMemory(ptr);
}

void GPUMemoryAllocator::DefragBegin()
{
	thisBuf = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)
		->GetGraphicsContext()->GetTransferBufferFromPool();
	thisBuf->BeginRecording();
}

void GPUMemoryAllocator::DefragEnd()
{
	thisBuf->StopRecording();

	XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)
		->GetGraphicsContext()->SubmitCommands(thisBuf, GraphicsQueueType::Transfer);
}

void GPUMemoryAllocator::MoveMemory(MoveData& data)
{
	if (data.DestIndex + data.Size > data.SrcIndex)
	{
		int diffSize = data.DestIndex + data.Size - data.SrcIndex;
		if (m_tempSize < diffSize)
		{
			if (m_temp)
				delete m_temp;
			m_temp = m_context->CreateBuffer(diffSize, BufferUsageBit::TransferSource | BufferUsageBit::TransferDest,
				GraphicsMemoryTypeBit::DeviceResident | GraphicsMemoryTypeBit::DynamicAccess);
		}
		thisBuf->CopyBufferToBuffer(m_memory, m_temp, data.SrcIndex, 0, diffSize);
		thisBuf->CopyBufferToBuffer(m_temp, m_memory, 0, data.DestIndex, diffSize);
		thisBuf->CopyBufferToBuffer(m_memory, m_memory, data.SrcIndex + diffSize, data.DestIndex + diffSize, data.Size - diffSize);
	}
	else
	{
		thisBuf->CopyBufferToBuffer(m_memory, m_memory, data.SrcIndex, data.DestIndex, data.Size);
	}
}
