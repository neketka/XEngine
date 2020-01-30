#include "pch.h"
#include "SystemGraphSorter.h"

SystemGraphSorter::SystemGraphSorter(std::vector<ISystem *>& systems)
{
	for (ISystem *system : systems)
		m_systems[system->GetName()] = system;
}

void SystemGraphSorter::SetupGraph()
{
	for (auto sysPair : m_systems)
	{
		DirectedSystemGraphNode node;
		node.System = sysPair.second;
		m_nodes.push_back(node);
		m_systemNodes[node.System->GetName()] = m_nodes.size() - 1;
	}
	int index = 0;
	for (DirectedSystemGraphNode& node : m_nodes) 
	{
		for (std::string name : node.System->GetSystemsBefore())
		{ 
			node.Inputs.push_back(m_systemNodes[name]);
			m_nodes[node.Inputs.back()].Outputs.push_back(index);
		}
		for (std::string name : node.System->GetSystemsAfter())
		{
			node.Outputs.push_back(m_systemNodes[name]);
			m_nodes[node.Outputs.back()].Inputs.push_back(index);
		}
		++index;
	}
	index = 0;
	for (DirectedSystemGraphNode& node : m_nodes)
	{
		if (node.Inputs.size() == 0)
			m_startingNodes.push_back(index);
		++index;
	}
	std::map<std::string, ISystem *>().swap(m_systems);
	std::map<std::string, int>().swap(m_systemNodes);
}

std::vector<std::vector<ISystem *>> SystemGraphSorter::GetAllPaths()
{
	std::vector<std::vector<ISystem *>> paths;
	for (int start : m_startingNodes)
	{
		DirectedSystemGraphNode& node = m_nodes[m_startingNodes[start]];
		std::vector<ISystem *> path;

		//TODO: depth first search

		paths.push_back(path);
	}
}
