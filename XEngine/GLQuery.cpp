#include "pch.h"
#include "GLQuery.h"
#include "GLContext.h"
#include <GL/glew.h>

void GLQuery::InitInternal() 
{
	glGenQueries(1, &m_id);
}

GLQuery::~GLQuery()
{
	GLuint id = m_id;

	dynamic_cast<GLContext *>(m_context)->DeleteInitable([id]() {
		glDeleteQueries(1, &id);
	});
}

GLSync::~GLSync()
{
	GLsync sync = m_sync;
	bool doDel = m_hasSync;

	if (m_hasSync)
	{
		dynamic_cast<GLContext *>(m_context)->DeleteInitable([sync]()
			{
				glDeleteSync(sync);
			});
	}
}

void GLSync::SetSync(GLsync sync)
{ 
	if (m_hasSync)
		glDeleteSync(m_sync);
	m_sync = sync; 
	m_hasSync = true;
}

bool GLSync::Wait(uint64_t nanoSecTimeout)
{
	m_waiting = true;
	m_curWaitTime = nanoSecTimeout;
	while (!m_signaled);
	m_waiting = false;
	return m_signalVal;
}

bool GLSync::GetCurrentStatus()
{
	return m_signaled;
}

void GLSync::Reset()
{
	ResetSignaled();
}
