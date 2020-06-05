#include "pch.h"
#include "GLBuffer.h"
#include "GLContext.h"

GLBuffer::GLBuffer(GraphicsContext *context, int byteSize, GraphicsMemoryTypeBit mem) : m_size(byteSize), m_context(context)
{
	m_bits = 0;
	if (!(mem & GraphicsMemoryTypeBit::DeviceResident))
		m_bits |= GL_CLIENT_STORAGE_BIT;
	if (mem & GraphicsMemoryTypeBit::HostVisible)
	{
		m_bits |= GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
		if (mem & GraphicsMemoryTypeBit::Coherent)
			m_bits |= GL_MAP_COHERENT_BIT;
	}
	if (mem & GraphicsMemoryTypeBit::DynamicAccess)
		m_bits |= GL_DYNAMIC_STORAGE_BIT;
}

GLBuffer::~GLBuffer()
{
	GLuint id = m_id;
	dynamic_cast<GLContext *>(m_context)->DeleteInitable([id]() {
		glDeleteBuffers(1, &id);
	});
}

void GLBuffer::MapBuffer(bool coherent)
{
	MapBuffer(0, m_size, coherent);
}

void GLBuffer::UnmapBuffer()
{
	m_request.Reset();
	m_request.RequestUnmap = true;

	m_mapped = nullptr;
	dynamic_cast<GLContext *>(m_context)->MapRequest(this);
}

void GLBuffer::InvalidateMapped(int offset, int size)
{
	m_request.Reset();
	m_request.RequestInvalidate = true;
	m_request.Offset = offset;
	m_request.Length = size;

	dynamic_cast<GLContext *>(m_context)->MapRequest(this);
}

void *GLBuffer::GetMappedPointer()
{
	return m_mapped;
}

void GLBuffer::InitInternal()
{
	glCreateBuffers(1, &m_id);
	glNamedBufferStorage(m_id, m_size, nullptr, m_bits);
}

void GLBuffer::MapBuffer(int offset, int length, bool coherent)
{
	m_request.Reset();
	m_request.RequestMap = true;
	m_request.Offset = offset;
	m_request.Length = length;

	m_request.Bits = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
	if (coherent)
		m_request.Bits |= GL_MAP_COHERENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;

	dynamic_cast<GLContext *>(m_context)->MapRequest(this);
	m_mapped = m_request.Mapped;
}

void GLBuffer::FlushMapped(int offset, int size)
{
	m_request.Reset();
	m_request.RequestFlush = true;
	m_request.Offset = offset;
	m_request.Length = size;

	dynamic_cast<GLContext *>(m_context)->MapRequest(this);
}
