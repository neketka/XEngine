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

	unsigned long long Offset;
	int Length;

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
	GLBuffer(GraphicsContext *context, unsigned long long byteSize, GraphicsMemoryTypeBit mem);
	virtual ~GLBuffer() override;
	virtual void MapBuffer(unsigned long long offset, int length, bool coherent, bool writeOnly) override;
	virtual void UnmapBuffer() override;
	virtual void FlushMapped(unsigned long long offset, int size) override;
	virtual void InvalidateMapped(unsigned long long offset, int size) override;
	virtual void *GetMappedPointer() override;
	GLuint GetBufferId() { return m_id; }
	unsigned long long GetSize() { return m_size; }
	GLBufferMapRequest& GetMapRequest() { return m_request; }
private:
	virtual void InitInternal() override;
	GraphicsContext *m_context;
	GLbitfield m_bits;

	GLBufferMapRequest m_request;

	unsigned long long m_size;
	GLuint m_id;
	void *m_mapped = nullptr;
};