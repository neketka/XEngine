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
	uint64_t HeaderSize;
	uint64_t AssetSize;
};

class AssetLoadRange
{
public:
	int32_t ByteOffset;
	int32_t Size;
	int32_t BulkLoadIndex;
	int32_t InternalId;
};

class AssetBundleHeaderEntry
{
public:
	std::string VirtualAssetPath;
	uint64_t OffsetFromZero;
};

class AssetBundleHeader
{
public:
	uint64_t ByteSize;
	std::string Path;
	std::map<std::string, AssetBundleHeaderEntry> Assets;
};

using LoadMemoryPointer = ListMemoryPointer *;

class AssetBundleReader
{
public:
	AssetBundleReader() {}

	AssetBundleHeader& GetOrLoadAssetBundleHeader(std::string path);
	std::vector<std::string> LoadAssetHeadersFromHeader(AssetBundleHeader& header, std::vector<LoadMemoryPointer>& headersDest,
		std::vector<AssetDescriptorPreHeader>& preHeadersDest);
	void LoadAssetDataFromHeader(AssetBundleHeader& header, std::string path,
		std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& dest);
	void ExportAssetBundleToDisc(std::string filePath, std::vector<AssetDescriptorPreHeader>& preHeaders, 
		std::vector<std::string>& paths, std::vector<LoadMemoryPointer>& headers, std::vector<LoadMemoryPointer>& contents,
		std::vector<std::vector<AssetLoadRange>>& ranges);

private:
	std::map<std::string, AssetBundleHeader> m_headers;
};
