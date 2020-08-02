#pragma once
#include <functional>
#include <concurrent_queue.h>
#include <thread>
#include <ctime>

class WorkerManager;

class InternalWorkerTask
{
public:
	virtual ~InternalWorkerTask() { }
	virtual int32_t GetNextIterationNumber() = 0;
	virtual void Perform(int32_t iteration) = 0;
	virtual bool IsFree() = 0;
};

template<class T>
class WorkerTask : public InternalWorkerTask
{
public:
	WorkerTask(std::function<T(int32_t)> func, int32_t iterCount) : m_func(func), m_freed(false), 
		m_resultsReturned(0), m_resultsExpected(iterCount), m_iterQueued(0)
	{
		m_resultCounter.resize(iterCount);
		m_results.resize(iterCount);
	}
	virtual ~WorkerTask() override { }
	void BlockUntilDone() 
	{ 
		while (m_resultsExpected != m_resultsReturned)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	bool BlockUntilDoneFor(int32_t millitime)
	{
		clock_t t = std::clock();
		while (m_resultsExpected != m_resultsReturned)
			if ((clock() - t) / CLOCKS_PER_SEC * 1000 >= millitime)
				return false;
			else
				std::this_thread::sleep_for(std::milli(1));
		return true;
	}
	void Destroy() { m_freed = true; }
	bool IsDone() { return m_resultsExpected == m_resultsReturned; }
	int32_t GetRemainingIterationsCount() { return m_resultsReturned; }
	int32_t GetExpectedIterationsCount() { return m_resultsExpected; }
	bool IsIterationDone(int32_t iteration) { return m_resultCounter[iteration]; }
	std::vector<T>& GetResults() { BlockUntilDone(); return m_results; }

	virtual void Perform(int32_t iteration) 
	{
		m_results[iteration] = m_func(iteration);
		m_resultCounter[iteration] = true;
		++m_resultsReturned;
	}

	virtual int32_t GetNextIterationNumber()
	{
		int32_t i = m_iterQueued++;
		if (m_iterQueued >= m_resultsExpected)
			return -i - 1;
		else
			return i;
	}

	virtual bool IsFree() override { return m_freed; }
private:
	std::function<T(int32_t)> m_func;
	std::vector<bool> m_resultCounter;
	std::vector<T> m_results;
	std::atomic<bool> m_freed;
	std::atomic<int32_t> m_resultsReturned;
	std::atomic<int32_t> m_resultsExpected;
	std::atomic<int32_t> m_iterQueued;
};

class WorkerManager
{
public:
	XENGINEAPI WorkerManager(int32_t threads);
	XENGINEAPI ~WorkerManager();
	template<class T>
	WorkerTask<T>& EnqueueTask(std::function<T(int32_t)> task, int32_t iterations)
	{
		WorkerTask<T> *taskd = new WorkerTask<T>(task, iterations);
		if (iterations > 0)
		{
			m_tasks.push(taskd);
			m_workerTaskHolders.push_back(taskd);
		}
		return *taskd;
	}

	XENGINEAPI void CheckForFree();
private:
	bool m_running = true;
	void RunThreadTasks();

	std::vector<std::thread *> m_threads;
	std::vector<InternalWorkerTask *> m_workerTaskHolders;
	concurrency::concurrent_queue<InternalWorkerTask *> m_tasks;
};

