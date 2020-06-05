#pragma once

#include <string>
#include "UUID.h"
#include <vector>

class AssetDescriptorHeader
{
public:
	const char AssetType[64];
	const char FriendlyName[64];
	UniqueId Id;
	unsigned long long AssetSize;
};

class AssetContent { };

class IAsset
{
public:
	virtual UniqueId GetId() = 0;
	virtual std::string GetTypeName() = 0;
	virtual void SetName(std::string name) = 0;
	virtual std::string GetName() = 0;
	virtual int Load(float lod, bool preload) = 0;
	virtual float GetMaxLoadedLod() = 0;
	virtual void ClearBytes(int bytes) = 0;
	virtual void RemoveRef() = 0;
	virtual void Export(AssetDescriptorHeader *header, AssetContent **content) = 0;
};

class IAssetLoader
{
public:
	virtual std::vector<std::string>& GetAssetTypes() = 0;
};

class IFormatImporter
{
public:
	virtual IAsset *Import(std::string name, std::string path) = 0;
};

class AssetManager
{
public:
	void RegisterImporter();
	void RegisterLoader();
	UniqueId GetIdByPath(std::string path);
	IAsset *GetAsset(UniqueId id);
	void ReassignPath(IAsset *asset, std::string path);
	IAsset *ImportAsAsset(std::string name, std::string filePath);
};
