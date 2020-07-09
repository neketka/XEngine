#include "pch.h"
#include "AssetManager.h"

#include <filesystem>

AssetManager::AssetManager(int loadMemSize, int assetMemSize) : m_loadMemory(loadMemSize, 4096, true), m_assetMemory(assetMemSize, 1e6, true)
{
	m_running = true;
	m_assetLoadingThread = new std::thread(&AssetManager::PerformThreadTasks, this);
}

AssetManager::~AssetManager()
{
	m_running = false;
	m_assetLoadingThread->join();
	delete m_assetLoadingThread;
	for (auto loaderPair : m_loaders)
		delete loaderPair.second;
	for (auto importerPair : m_importers)
		delete importerPair.second;
}

void AssetManager::AddAsset(std::string path, IAsset *asset)
{
	StoredAssetPtr& ptr = m_assets[asset->GetId()];
	ptr.Asset = asset;
	ptr.VirtualPath = path;
	if (path != "")
		m_pathToId[path] = asset->GetId();
}

void AssetManager::DeleteAsset(UniqueId asset)
{
	StoredAssetPtr& assetPtr = m_assets[asset];
	IAssetLoader *loader = m_loaders[assetPtr.Asset->GetTypeName()];

	loader->Dispose(assetPtr.Asset);
	m_pathToId[assetPtr.VirtualPath] = 0;

	assetPtr.Asset = nullptr;
	assetPtr.VirtualPath = "";
}

IAsset *AssetManager::CreateAssetPtr(std::string path, std::string type)
{
	IAssetLoader *loader = m_loaders[type];

	UniqueId id = GenerateID();

	StoredAssetPtr& assetPtr = m_assets[id];
	assetPtr.Asset = loader->CreateEmpty(id);
	assetPtr.VirtualPath = path;

	m_pathToId[path] = id;

	return assetPtr.Asset;
}

void AssetManager::PreloadAssetBundle(std::string filePath)
{
	std::string location = XEngineInstance->GetRootPath() + filePath;
	AssetBundleHeader& bundle = m_bundleReader.GetOrLoadAssetBundleHeader(location);

	std::vector<AssetDescriptorPreHeader> preHeaders;
	std::vector<LoadMemoryPointer> headerPtrs;
	
	std::vector<std::string> paths = m_bundleReader.LoadAssetHeadersFromHeader(bundle, headerPtrs, preHeaders);

	for (int i = 0; i < preHeaders.size(); ++i)
	{
		AssetDescriptorPreHeader& preHeader = preHeaders[i];
		LoadMemoryPointer& ptr = headerPtrs[i];
		IAssetLoader *loader = m_loaders[preHeader.AssetType];
		
		IAsset *asset = loader->CreateEmpty(preHeader.Id);
		loader->Preload(asset, ptr);

		m_pathToId[paths[i]] = preHeader.Id;

		StoredAssetPtr& sap = m_assets[preHeader.Id];
		sap.Asset = asset;
		sap.BundlePath = filePath;
		sap.VirtualPath = paths[i];

		m_loadMemory.FreeSpace(ptr);
	}
}

void AssetManager::ExportAssetBundleToDisc(std::string filePath, std::vector<UniqueId>& assets, bool nonBlocking)
{
	if (nonBlocking)
		m_exportRequests.push(AssetExportRequest(filePath, assets));
	else
		ExportAssetBundleToDisc(filePath, assets);
}

void AssetManager::RegisterImporter(IFormatImporter *importer)
{
	for (std::string& ext : importer->GetFileExtensions())
		m_importers[ext] = importer;
}

void AssetManager::RegisterLoader(IAssetLoader *loader)
{
	m_loaders[loader->GetAssetType()] = loader;
}

IAssetLoader *AssetManager::GetLoader(std::string type)
{
	return m_loaders[type];
}

UniqueId AssetManager::GetIdByPath(std::string path)
{
	return m_pathToId[path];
}

std::string& AssetManager::GetPathById(UniqueId id)
{
	return m_assets[id].VirtualPath;
}

void AssetManager::ReassignPath(UniqueId id, std::string path)
{
	StoredAssetPtr& ptr = m_assets[id];

	m_pathToId[ptr.VirtualPath] = 0;
	m_pathToId[path] = id;

	ptr.VirtualPath = path;
}

void AssetManager::PushLoadRequest(IAssetLoader *loader, IAsset *asset, LoadMemoryPointer loadData)
{
	m_loadRequests.push(AssetLoadRequest(loader, asset, loadData));
}

void AssetManager::ImportAsAssets(std::string path, std::string filePath)
{
	std::string location = XEngineInstance->GetRootPath() + filePath;

	std::string ext = std::filesystem::path(filePath).extension().string().substr(1);
	m_importers[ext]->Import(path, filePath);
}

IAsset *AssetManager::GetAssetPtr(UniqueId id)
{
	return m_assets[id].Asset;
}

void AssetManager::PerformThreadTasks()
{
	AssetLoadRequest request(nullptr, nullptr, nullptr);
	AssetExportRequest eRequest("", {});
	while (m_running)
	{
		while (m_loadRequests.try_pop(request))
		{
			std::vector<AssetLoadRange> loadRanges = request.Loader->Load(request.Asset, request.LoadData);

			int totalSize = 0;
			for (AssetLoadRange& range : loadRanges)
				totalSize += range.Size;

			if (!request.Loader->CanLoad(request.Asset, request.LoadData) && !m_loadMemory.WillFit(totalSize))
			{
				m_loadRequests.push(request);
				continue;
			}

			std::vector<LoadMemoryPointer> loadMemories(loadRanges.size());
			for (int i = 0; i < loadRanges.size(); ++i)
			{
				loadMemories[i] = m_loadMemory.RequestSpace(loadRanges[i].Size);
			}

			StoredAssetPtr& stored = m_assets[request.Asset->GetId()];

			m_bundleReader.LoadAssetDataFromHeader(m_bundleReader.GetOrLoadAssetBundleHeader(stored.BundlePath), stored.VirtualPath, 
				loadRanges, loadMemories);

			request.Loader->FinishLoad(request.Asset, loadRanges, loadMemories, request.LoadData);
		}
		while (m_exportRequests.try_pop(eRequest))
		{
			ExportAssetBundleToDisc(eRequest.Path, eRequest.Assets);
		}
		XEngineInstance->DoIdleWork();
	}
}

void AssetManager::ExportAssetBundleToDisc(std::string filePath, std::vector<UniqueId>& assets)
{
	std::string location = XEngineInstance->GetRootPath() + filePath;

	std::vector<AssetDescriptorPreHeader> preHeaders(assets.size());
	std::vector<std::string> paths(assets.size());
	std::vector<LoadMemoryPointer> headers(assets.size());
	std::vector<LoadMemoryPointer> contents(assets.size());

	for (int i = 0; i < assets.size(); ++i)
	{
		UniqueId& id = assets[i];
		StoredAssetPtr& asset = m_assets[id];
		asset.BundlePath = filePath;

		IAssetLoader *loader = m_loaders[asset.Asset->GetTypeName()];

		paths[i] = asset.VirtualPath;

		AssetDescriptorPreHeader& preHeader = preHeaders[i];
		LoadMemoryPointer& header = headers[i];
		LoadMemoryPointer& content = contents[i];

		loader->Export(asset.Asset, preHeader, header, content);
	}

	m_bundleReader.ExportAssetBundleToDisc(location, preHeaders, paths, headers, contents);

	for (int i = 0; i < assets.size(); ++i)
	{
		m_loadMemory.FreeSpace(headers[i]);
		m_loadMemory.FreeSpace(contents[i]);
	}
}

std::vector<std::string> AssetManager::MergeAssetBundles(std::vector<std::string>& bundles, unsigned long long maxSize)
{
	// TODO: IMPLEMENT
	return std::vector<std::string>();
}

LocalMemoryAllocator& AssetManager::GetAssetMemory()
{
	return m_assetMemory;
}

LocalMemoryAllocator& AssetManager::GetLoadMemory()
{
	return m_loadMemory;
}

void AssetManager::Copy(IAsset *src, IAsset *dest)
{
	if (src->GetTypeName() != dest->GetTypeName())
		return;

	m_loaders[src->GetTypeName()]->Copy(src, dest);
}

UniqueId AssetManager::Duplicate(IAsset *src, std::string newPath)
{
	IAsset *dest = CreateAssetPtr(newPath, src->GetTypeName());
	m_loaders[src->GetTypeName()]->Copy(src, dest);

	return dest->GetId();
}

void AssetManager::DeleteBundleAssetsFromMemory(std::string filePath)
{
	// TODO: IMPLEMENT
}

void AssetManager::CleanUnusedMemory()
{
	// TODO: IMPLEMENT
}

IAssetLoader::~IAssetLoader()
{
}

IFormatImporter::~IFormatImporter()
{
}
