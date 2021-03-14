#pragma once
#include "System.h"
#include <concurrent_queue.h>
#include <map>
#include <queue>

class DirectedSystemGraphNode
{
public:
	ISystem *System;
	std::vector<DirectedSystemGraphNode *> Inputs;
	std::vector<DirectedSystemGraphNode *> Outputs;

	std::atomic_int UnfulfilledInputs;
	std::atomic_int QueuedJobs;

	std::vector<bool> *VisitedInputs;

	std::mutex Mutex;
};

class SystemGraphSorter
{
public:
	XENGINEAPI SystemGraphSorter(ComponentManager *manager, std::vector<ISystem *>& systems);
	XENGINEAPI ~SystemGraphSorter();
	XENGINEAPI void SetDeltaTime(float dt);
	XENGINEAPI void QueueLayers();
	XENGINEAPI void RunFromThread(bool isMain);
private:
	void SetupGraph(std::vector<ISystem *>& systems);
	void PropagateUntilFindEnabledOrNonEmptyOrVisitedOrUnfulfilled(std::vector<DirectedSystemGraphNode *>& nodes);

	std::atomic_int m_threadsBusy;
	float m_deltaTime;

	concurrency::concurrent_queue<ComponentDataIterator> m_jobs;

	std::vector<DirectedSystemGraphNode *> m_nodes;
	std::vector<DirectedSystemGraphNode *> m_startingNodes;
	ComponentManager *m_manager;
};