#include "pch.h"
#include "SystemGraphSorter.h"
#include <concurrent_vector.h>

SystemGraphSorter::SystemGraphSorter(ComponentManager *manager, std::vector<ISystem *>& systems) : m_manager(manager)
{
	SetupGraph(systems);
}

SystemGraphSorter::~SystemGraphSorter()
{
	for (DirectedSystemGraphNode *node : m_nodes)
	{
		delete node;
	}
}

void SystemGraphSorter::SetDeltaTime(float dt)
{
	m_deltaTime = dt;
}

void SystemGraphSorter::QueueLayers()
{
	PropagateUntilFindEnabledOrNonEmptyOrVisitedOrUnfulfilled(m_startingNodes); // Add the starting nodes' jobs

	m_currVisitedFlag = !m_currVisitedFlag; // Invert flag for this pass to differentiate from last pass
}

void SystemGraphSorter::RunFromThread(bool isMain)
{
	bool markedBusy = false;
	ComponentDataIterator job({}, {}, 0, 0);
	do // Basic multithreaded breadth-first traversal
	{
		if (m_jobs.try_pop(job))
		{
			if (!markedBusy) // Mark this thread as busy if it has not been already
				++m_threadsBusy; 

			DirectedSystemGraphNode *node = reinterpret_cast<DirectedSystemGraphNode *>(job.UserPointer);
			bool isDisposed = job.UserFlag;

			if (isDisposed)
				node->System->Dispose(job);
			else
				node->System->Update(m_deltaTime, job);

			if (--node->QueuedJobs == 0 && node->Mutex.try_lock())
			{
				PropagateUntilFindEnabledOrNonEmptyOrVisitedOrUnfulfilled(node->Outputs); // Add jobs to the queue from real systems 
				node->Mutex.unlock();
			}

			markedBusy = true;
		}
		else
		{
			if (markedBusy)
				--m_threadsBusy;
			markedBusy = false;
		}
	} while (m_threadsBusy != 0); // Make sure that there are threads still working to add more jobs
}

void SystemGraphSorter::SetupGraph(std::vector<ISystem *>& systems)
{
	int index = 0; // Variable is reused
	std::map<std::string, ISystem *> systemsMap;
	std::map<std::string, DirectedSystemGraphNode *> systemNodes;

	for (ISystem *system : systems)
		systemsMap[system->GetName()] = system;

	for (auto sysPair : systemsMap)
	{
		DirectedSystemGraphNode *node = new DirectedSystemGraphNode;
		node->System = sysPair.second;
		node->VisitedFlag = m_currVisitedFlag;

		m_nodes.push_back(node);
		systemNodes[node->System->GetName()] = node;
	}

	for (DirectedSystemGraphNode *node : m_nodes) 
	{
		for (std::string name : node->System->GetSystemsBefore())
		{ 
			if (systemNodes.find(name) == systemNodes.end())
				continue;
			node->Inputs.push_back(systemNodes[name]); // Add requirement systems as inputs
			node->Inputs.back()->Outputs.push_back(m_nodes[index]);
		}
		for (std::string name : node->System->GetSystemsAfter())
		{
			if (systemNodes.find(name) == systemNodes.end())
				continue;
			node->Outputs.push_back(systemNodes[name]); // Add dependent systems as outputs
			node->Outputs.back()->Inputs.push_back(m_nodes[index]);
		}
		++index;
	}

	index = 0;
	for (DirectedSystemGraphNode *node : m_nodes) // Find the amount of starting nodes
	{
		if (node->Inputs.size() == 0)
			m_startingNodes.push_back(m_nodes[index]);
		++index;
	}

	index = 0;
	for (DirectedSystemGraphNode *node : m_nodes) // Add all the
	{
		for (int innerIndex = 0; innerIndex < m_nodes.size(); ++innerIndex)
		{
			if (index == innerIndex) // Can be removed by splitting into two loops; saves a compare and jump
				continue;

			DirectedSystemGraphNode *innerNode = m_nodes[innerIndex];

			std::vector<std::string> innerTypes = innerNode->System->GetComponentTypes();
			std::vector<std::string> outerTypes = node->System->GetComponentTypes();

			bool found = false;
			for (std::string type : innerTypes)
			{ // See if they contain ANY common components
				found |= std::find(outerTypes.begin(), outerTypes.end(), type) != outerTypes.end();
			}

			if (found)
			{ // Link together if found; loops aren't a problem because of "visited" flag
				node->Outputs.push_back(m_nodes[innerIndex]);
				innerNode->Inputs.push_back(m_nodes[index]);
			}
		}
		++index;
	}

	for (DirectedSystemGraphNode *node : m_nodes)
	{
		node->UnfulfilledInputs = node->Inputs.size(); // Set the amount of inputs unfulfilled as the number of inputs
	}
}

void SystemGraphSorter::PropagateUntilFindEnabledOrNonEmptyOrVisitedOrUnfulfilled(std::vector<DirectedSystemGraphNode *> nodes)
{
	for (DirectedSystemGraphNode *output : nodes)
	{
		if (output->VisitedFlag == m_currVisitedFlag) // If already visited
			continue;
		else if (--output->UnfulfilledInputs > 0) // If all input graphs are finished
			continue;
		else if (output->System->IsEnabled())
		{
			if (!output->Mutex.try_lock()) // Try to hold lock for this system
				continue;
			output->VisitedFlag = m_currVisitedFlag;

			std::vector<ComponentDataIterator> *jobs = m_manager->GetFilteringGroup(output->System->__filteringGroup, false);
			std::vector<ComponentDataIterator> *disposedJobs = m_manager->GetFilteringGroup(output->System->__filteringGroup, true);

			if (jobs->empty() && disposedJobs->empty())
			{
				PropagateUntilFindEnabledOrNonEmptyOrVisitedOrUnfulfilled(output->Outputs); // Add the outputs as jobs as much as possible

				delete jobs;
				delete disposedJobs;

				output->Mutex.unlock();

				continue;
			}

			output->QueuedJobs = jobs->size() + disposedJobs->size();

			for (ComponentDataIterator iter : *jobs) // Dump jobs into queue
			{
				iter.UserPointer = output;
				iter.UserFlag = false;
				m_jobs.push(iter);
			}

			for (ComponentDataIterator iter : *disposedJobs) // Dump disposed jobs into queue
			{
				iter.UserPointer = output;
				iter.UserFlag = true;
				m_jobs.push(iter);
			}

			delete jobs;
			delete disposedJobs;

			output->Mutex.unlock();
		}
		else // If disabled
		{
			if (output->Mutex.try_lock())
			{
				PropagateUntilFindEnabledOrNonEmptyOrVisitedOrUnfulfilled(output->Outputs);
				output->Mutex.unlock();
			}
		}
	}
}
