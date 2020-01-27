#include "pch.h"
#include "XEngine.h"

XEngine *XEngine::m_engineInstance;

void XEngine::InitializeEngine(bool defaultSystems, std::string rootPath)
{
}

XEngine& XEngine::GetInstance()
{
	return *XEngine::m_engineInstance;
}

XEngine::XEngine()
{
}

XEngine::~XEngine()
{
}

void XEngine::Run()
{
}

void XEngine::AddPropertySet(std::map<std::string, int> properties)
{
	for (auto kp : properties)
	{
		m_intProps[kp.first] = kp.second;
	}
}

void XEngine::AddPropertySet(std::map<std::string, std::string> properties)
{
	for (auto kp : properties)
	{
		m_stringProps[kp.first] = kp.second;
	}
}

void XEngine::SetProperty(std::string key, int value)
{
	m_intProps[key] = value;
}

void XEngine::SetProperty(std::string key, std::string value)
{
	m_stringProps[key] = value;
}

std::string *XEngine::GetStringProperty(std::string key)
{
	auto itr = m_stringProps.find(key);
	return (itr == m_stringProps.end() ? nullptr : &itr->second);
}

int *XEngine::GetIntProperty(std::string key)
{
	auto itr = m_intProps.find(key);
	return (itr == m_intProps.end() ? nullptr : &itr->second);
}

void XEngine::SaveProperties()
{
}

void XEngine::LoadProperties()
{
}

float XEngine::GetTime()
{
	return 0.0f;
}

void XEngine::AddBeginMarker(std::string label)
{
}

void XEngine::AddEndMarker(std::string label)
{
}

void XEngine::SetRootPath(std::string rootPath)
{
	m_rootPath = rootPath;
}

void XEngine::SetScene(Scene *scene)
{
	if (m_scene)
		m_scene->DisableScene();
	m_scene = scene;
	scene->EnableScene();
}

Scene *XEngine::GetScene()
{
	return m_scene;
}

SubsystemManager *XEngine::GetSubsystemManager()
{
	return m_sysManager;
}

ECSRegistrar *XEngine::GetECSRegistrar()
{
	return nullptr;
}
