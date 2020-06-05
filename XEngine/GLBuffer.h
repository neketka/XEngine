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

	int Offset;
	int Length;

	void *Mapped;

	std::mutex Lock;
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
	GLBuffer(GraphicsContext *context, int byteSize, GraphicsMemoryTypeBit mem);
	virtual ~GLBuffer() override;
	virtual void MapBuffer(bool coherent) override;
	virtual void MapBuffer(int offset, int length, bool coherent) override;
	virtual void UnmapBuffer() override;
	virtual void FlushMapped(int offset, int size) override;
	virtual void InvalidateMapped(int offset, int size) override;
	virtual void *GetMappedPointer() override;
	GLuint GetBufferId() { return m_id; }
	int GetSize() { return m_size; }
	GLBufferMapRequest& GetMapRequest() { return m_request; }
private:
	virtual void InitInternal() override;
	GraphicsContext *m_context;
	GLbitfield m_bits;

	GLBufferMapRequest m_request;

	int m_size;
	GLuint m_id;
	void *m_mapped = nullptr;
};