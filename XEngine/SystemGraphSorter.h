#pragma once
#include "System.h"

class DirectedSystemGraphNode
{
public:
	ISystem *System;
	std::vector<int> Inputs;
	std::vector<int> Outputs;
};

class SystemGraphSorter
{
public:
	SystemGraphSorter(std::vector<ISystem *>& systems);
	void SetupGraph();
	std::vector<std::vector<ISystem *>> GetAllPaths();
private:
	std::vector<DirectedSystemGraphNode> m_nodes;
	std::map<std::string, ISystem *> m_systems;
	std::map<std::string, int> m_systemNodes;
	std::vector<int> m_startingNodes;
};