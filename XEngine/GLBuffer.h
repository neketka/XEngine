#pragma once

#include <GL/glew.h>
#include "GraphicsDefs.h"
#include "GLInitable.h"
#include <atomic>

class GLBufferMapRequest
{
public:
	bool RequestUnmap;
	bool RequestMap;
	bool RequestFlush;
	bool RequestInvalidate;

	GLbitfield Bits;

	uint64_t Offset;
	int32_t Length;

	void *Mapped;

	std::atomic_bool Helper;

	void Reset()
	{
		RequestUnmap = false;
		RequestMap = false;
		RequestFlush = false;
		RequestInvalidate = false;
		Bits = 0;
		Offset = 0;
		Length = 0;
		Mapped = nullptr;
		Helper = false;
	}
};

class GLBuffer : public GraphicsMemoryBuffer, public GLInitable
{
public:
	GLBuffer(GraphicsContext *context, uint64_t byteSize, GraphicsMemoryTypeBit mem);
	virtual ~GLBuffer() override;
	virtual void MapBuffer(uint64_t offset, int32_t length, bool coherent, bool writeOnly) override;
	virtual void UnmapBuffer() override;
	virtual void FlushMapped(uint64_t offset, int32_t size) override;
	virtual void InvalidateMapped(uint64_t offset, int32_t size) override;
	virtual void *GetMappedPointer() override;
	GLuint GetBufferId() { return m_id; }
	uint64_t GetSize() { return m_size; }
	GLBufferMapRequest& GetMapRequest() { return m_request; }
private:
	virtual void InitInternal() override;
	GraphicsContext *m_context;
	GLbitfield m_bits;

	GLBufferMapRequest m_request;

	uint64_t m_size;
	GLuint m_id;
	void *m_mapped = nullptr;
};