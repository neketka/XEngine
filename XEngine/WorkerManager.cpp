#include "pch.h"
#include "WorkerManager.h"

WorkerManager::WorkerManager(int threads)
{
	for (int i = 0; i < threads; ++i)
	{
		std::thread *worker = new std::thread(&WorkerManager::RunThreadTasks, this);
		m_threads.push_back(worker);
	}
}

WorkerManager::~WorkerManager()
{
	m_running = false;
	for (std::thread *worker : m_threads)
	{
		worker->join();
		delete worker;
	}
}

void WorkerManager::CheckForFree()
{
	for (int i = 0; i < m_workerTaskHolders.size(); ++i)
	{
		InternalWorkerTask *task = m_workerTaskHolders[i];
		if (task->IsFree())
		{
			m_workerTaskHolders.erase(m_workerTaskHolders.begin() + i);
			--i;
			delete task;
		}
	}
}

void WorkerManager::RunThreadTasks()
{
	while (m_running)
	{
		InternalWorkerTask *task;
		if (m_tasks.try_pop(task))
		{
			int iteration = task->GetNextIterationNumber();
			if (iteration >= 0)
				m_tasks.push(task);
			else
				iteration = -iteration - 1;
			task->Perform(iteration);
		}
		else std::this_thread::sleep_for(std::chrono::milliseconds(0));
	}
}