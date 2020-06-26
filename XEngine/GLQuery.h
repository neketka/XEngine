#pragma once
#include "GLInitable.h"
#include "GraphicsDefs.h"
#include <GL/glew.h>
#include <atomic>

class GLQuery : public GraphicsQuery, public GLInitable
{
public:
	GLQuery(GraphicsContext *context, GraphicsQueryType type) : m_context(context), m_type(type) {}
	virtual ~GLQuery() override;
private:
	GraphicsQueryType m_type;
	GraphicsContext *m_context;
	GLuint m_id;
	virtual void InitInternal() override;
};

class GLSync : public GraphicsSyncObject
{
public:
	GLSync(GraphicsContext *context) : m_nanoSecTimeout(), m_context(context) {}
	virtual ~GLSync();
	
	void SetSync(GLsync sync);
	GLsync GetSync() { return m_sync; }

	void Signal(bool val) { m_signaled = true; m_signalVal = val; }
	void ResetSignaled() { m_signaled = false; }
	unsigned long long GetWaitTime() { return m_curWaitTime; }
	bool IsWaiting() { return m_waiting; }
	virtual void Reset() override;

	virtual bool Wait(unsigned long long nanoSecTimeout) override;
	virtual bool GetCurrentStatus() override;
private:
	std::atomic_bool m_signalVal;
	std::atomic_bool m_signaled = false;
	bool m_waiting;
	bool m_hasSync = false;
	unsigned long long m_curWaitTime;
	GraphicsContext *m_context;
	unsigned long long m_nanoSecTimeout;
	GLsync m_sync;
};