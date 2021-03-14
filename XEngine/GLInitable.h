#pragma once
#include <atomic>
#include <thread>
#include <functional>

class GLInitable
{
public:
	void InitializeFromContext()
	{
		/*if (m_finished)
			return;*/
		InitInternal();
		m_finished = true;
	}

	bool IsInitialized()
	{
		return m_finished;
	}

	void WaitInit()
	{
		while (!m_finished)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
protected:
	virtual void InitInternal() = 0;
	std::atomic_bool m_finished = false;
};