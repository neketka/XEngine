#include "pch.h"

#include "AssetBundleReader.h"
#include "AssetManager.h"
#include "XEngine.h"

#include <fstream>

AssetBundleHeader& AssetBundleReader::GetOrLoadAssetBundleHeader(std::string path)
{
	AssetBundleHeader& header = m_headers[path];
	if (header.Assets.size() != 0)
		return header;
	header.Path = path;

	int count = 0;
	std::ifstream stream(path.c_str(), std::ifstream::in | std::ifstream::binary);
	stream.read(reinterpret_cast<char *>(&count), sizeof(int));

	for (int i = 0; i < count; ++i)
	{
		int pathLen = 0;
		stream.read(reinterpret_cast<char *>(&pathLen), sizeof(int));
		char *path = new char[pathLen + 1];
		stream.read(path, pathLen);
		path[pathLen] = '\0';

		AssetBundleHeaderEntry& h = header.Assets[path];
		h.VirtualAssetPath = path;
		stream.read(reinterpret_cast<char *>(&h.OffsetFromZero), sizeof(h.OffsetFromZero));
	}

	header.ByteSize = stream.tellg();

	stream.close();

	return header;
}

std::vector<std::string> AssetBundleReader::LoadAssetHeadersFromHeader(AssetBundleHeader& header, std::vector<LoadMemoryPointer>& headersDest,
	std::vector<AssetDescriptorPreHeader>& preHeadersDest)
{
	std::ifstream stream(header.Path.c_str(), std::ifstream::in | std::ifstream::binary);
	headersDest.reserve(header.Assets.size());
	preHeadersDest.resize(header.Assets.size());

	std::vector<std::string> paths;
	paths.reserve(header.Assets.size());

	AssetManager *manager = XEngineInstance->GetAssetManager();
	int index = 0;
	for (auto pair : header.Assets)
	{
		AssetDescriptorPreHeader& preheader = preHeadersDest[index];

		stream.seekg(header.ByteSize + pair.second.OffsetFromZero, std::istream::beg);
		stream.read(reinterpret_cast<char *>(&preheader), sizeof(AssetDescriptorPreHeader));

		headersDest.push_back(manager->GetLoadMemory().RequestSpace(preheader.HeaderSize));
		PinnedLocalMemory<char> header = manager->GetLoadMemory().GetMemory<char>(headersDest.back());
		stream.read(header.GetData(), preheader.HeaderSize);

		paths.push_back(pair.first);

		++index;
	}

	stream.close();

	return paths;
}

void AssetBundleReader::LoadAssetDataFromHeader(AssetBundleHeader& header, std::string path, 
	std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& dest)
{
	AssetManager *manager = XEngineInstance->GetAssetManager();
	std::ifstream stream(header.Path.c_str(), std::ifstream::in | std::ifstream::binary);

	AssetBundleHeaderEntry& entry = header.Assets[path];

	AssetDescriptorPreHeader preheader;
	stream.seekg(header.ByteSize + entry.OffsetFromZero, std::istream::beg);
	stream.read(reinterpret_cast<char *>(&preheader), sizeof(AssetDescriptorPreHeader));

	unsigned long long initialPosition = static_cast<unsigned long long>(stream.tellg()) + preheader.HeaderSize;

	int index = 0;
	for (AssetLoadRange& range : ranges)
	{
		PinnedLocalMemory<char> content = manager->GetLoadMemory().GetMemory<char>(dest[index]);
		stream.seekg(initialPosition + range.ByteOffset, std::istream::beg);
		stream.read(content.GetData(), range.Size);
		++index;
	}

	stream.close();
}

void AssetBundleReader::ExportAssetBundleToDisc(std::string filePath, std::vector<AssetDescriptorPreHeader>& preHeaders, 
	std::vector<std::string>& paths, std::vector<LoadMemoryPointer>& headers, std::vector<LoadMemoryPointer>& contents)
{
	AssetManager *manager = XEngineInstance->GetAssetManager();
	std::ofstream stream(filePath.c_str(), std::ostream::out | std::ostream::binary);
	
	int count = preHeaders.size();
	stream.write(reinterpret_cast<char *>(&count), sizeof(int));

	AssetBundleHeader header;
	header.Path = filePath;

	AssetBundleHeaderEntry entry;

	decltype(AssetBundleHeaderEntry::OffsetFromZero) pointer = 0;
	for (int i = 0; i < preHeaders.size(); ++i)
	{
		std::string& path = paths[i];
		AssetDescriptorPreHeader& preHeader = preHeaders[i];

		count = path.length();
		stream.write(reinterpret_cast<char *>(&count), sizeof(int));
		stream.write(path.c_str(), path.length());
		stream.write(reinterpret_cast<char *>(&pointer), sizeof(pointer));

		entry.OffsetFromZero = pointer;
		entry.VirtualAssetPath = path;
		header.Assets[entry.VirtualAssetPath] = entry;

		pointer += sizeof(AssetDescriptorPreHeader) + preHeader.HeaderSize + preHeader.AssetSize;
	}

	header.ByteSize = pointer;
	m_headers[header.Path] = header;

	for (int i = 0; i < preHeaders.size(); ++i)
	{
		AssetDescriptorPreHeader& preHeader = preHeaders[i];
		PinnedLocalMemory<char> header = manager->GetLoadMemory().GetMemory<char>(headers[i]);
		PinnedLocalMemory<char> content = manager->GetLoadMemory().GetMemory<char>(contents[i]);

		stream.write(reinterpret_cast<char *>(&preHeader), sizeof(AssetDescriptorPreHeader));
		stream.write(header.GetData(), preHeader.HeaderSize);
		stream.write(content.GetData(), preHeader.AssetSize);
	}

	stream.flush();
	stream.close();
}
