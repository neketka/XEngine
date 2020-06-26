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

	for (int i = 0; i < 3; ++i)
	{
		m_syncQueue.push(nullptr);
		m_cmdBuffers.push(nullptr);
	}
}

GPUMemoryAllocator::~GPUMemoryAllocator()
{
	delete m_memory;
	if (m_temp)
		delete m_temp;
	while (!m_syncQueue.empty())
	{
		if (m_syncQueue.front())
			delete m_syncQueue.front();
		if (m_cmdBuffers.front())
			delete m_cmdBuffers.front();
	}
}

PinnedGPUMemory GPUMemoryAllocator::GetMemory(ListMemoryPointer *ptr)
{
	return PinnedGPUMemory(m_allocator.PinMemory(ptr), m_memory, m_allocator.GetAllocationSize(ptr));
}

ListMemoryPointer *GPUMemoryAllocator::RequestSpace(int bytes, int alignment)
{
	return m_allocator.AllocateMemory(bytes, alignment);
}

void GPUMemoryAllocator::FreeSpace(ListMemoryPointer *ptr)
{
	m_allocator.DeallocateMemory(ptr);
}

void GPUMemoryAllocator::DefragBegin()
{
	GraphicsSyncObject *obj = m_syncQueue.front();
	if (obj)
	{
		obj->Wait(1e12);
		thisBuf = m_cmdBuffers.front();
	}
	else
	{
		thisBuf = m_context->CreateGraphicsCommandBuffers(1, false, false, true)[0];
	}
	thisBuf->BeginRecording();
}

void GPUMemoryAllocator::DefragEnd()
{
	GraphicsSyncObject *obj = m_syncQueue.front();
	if (!obj)
	{
		obj = m_context->CreateSync(false);
	}
	else
	{
		obj->Reset();
	}
	thisBuf->SignalFence(obj);
	thisBuf->StopRecording();

	XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)
		->GetGraphicsContext()->SubmitCommands(thisBuf, GraphicsQueueType::Transfer);

	m_syncQueue.pop();
	m_cmdBuffers.pop();

	m_syncQueue.push(obj);
	m_cmdBuffers.push(thisBuf);
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
