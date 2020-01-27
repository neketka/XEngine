#pragma once

#include <string>
#include <vector>
#include <map>

#include "pch.h"

class XEngine
{
public:
	XENGINEAPI XEngine();
	XENGINEAPI ~XEngine();

	XENGINEAPI void Run();

	XENGINEAPI void AddPropertySet(std::map<std::string, int> properties);
	XENGINEAPI void AddPropertySet(std::map<std::string, std::string> properties);
	XENGINEAPI void SetProperty(std::string key, int value);
	XENGINEAPI void SetProperty(std::string key, std::string value);
	XENGINEAPI std::string *GetStringProperty(std::string key);
	XENGINEAPI int *GetIntProperty(std::string key);

	XENGINEAPI void SaveProperties();
	XENGINEAPI void LoadProperties();

	XENGINEAPI float GetTime();
	XENGINEAPI void AddBeginMarker(std::string label);
	XENGINEAPI void AddEndMarker(std::string label);

	XENGINEAPI void SetRootPath(std::string rootPath);

	XENGINEAPI void SetScene(Scene *scene);
	XENGINEAPI Scene *GetScene();
	XENGINEAPI SubsystemManager *GetSubsystemManager();
	XENGINEAPI ECSRegistrar *GetECSRegistrar();

	XENGINEAPI static void InitializeEngine(bool defaultSystems = true, std::string rootPath = "");
	XENGINEAPI static XEngine& GetInstance();
private:
	static XEngine *m_engineInstance;
	std::string m_rootPath;
	ECSRegistrar *m_ecsRegistrar = nullptr;
	Scene *m_scene;
	SubsystemManager *m_sysManager;
	std::map<std::string, int> m_intProps;
	std::map<std::string, std::string> m_stringProps;
};
