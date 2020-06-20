#pragma once

#include "exports.h"
#include "UUID.h"

#include <map>
#include <vector>
#include <string>

class AssetDescriptorPreHeader
{
public:
	char AssetType[64];
	UniqueId Id;
	unsigned long long HeaderSize;
	unsigned long long AssetSize;
};

class AssetLoadRange
{
public:
	int ByteOffset;
	int Size;
	int BulkLoadIndex;
	int InternalId;
};

class AssetBundleHeaderEntry
{
public:
	std::string VirtualAssetPath;
	unsigned long long OffsetFromZero;
};

class AssetBundleHeader
{
public:
	unsigned long long ByteSize;
	std::string Path;
	std::map<std::string, AssetBundleHeaderEntry> Assets;
};

using LoadMemoryPointer = ListMemoryPointer *;

class AssetBundleReader
{
public:
	AssetBundleReader() {}

	AssetBundleHeader& GetOrLoadAssetBundleHeader(std::string path);
	void LoadAssetHeadersFromHeader(AssetBundleHeader& header, std::vector<LoadMemoryPointer>& headersDest,
		std::vector<AssetDescriptorPreHeader>& preHeadersDest);
	void LoadAssetDataFromHeader(AssetBundleHeader& header, std::string path,
		std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& dest);
	void ExportAssetBundleToDisc(std::string filePath, std::vector<AssetDescriptorPreHeader>& preHeaders, 
		std::vector<std::string>& paths, std::vector<LoadMemoryPointer>& headers, std::vector<LoadMemoryPointer>& contents);

private:
	std::map<std::string, AssetBundleHeader> m_headers;
};
