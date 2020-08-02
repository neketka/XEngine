#pragma once

#include <string>
#include <vector>
#include <concurrent_unordered_map.h>
#include <thread>

#include "UUID.h"

#include "ListAllocator.h"
#include "GraphicsDefs.h"
#include "AssetBundleReader.h"
#include "LocalMemoryAllocator.h"

class IAsset
{
public:
	virtual UniqueId GetId() = 0;
	virtual std::string& GetTypeName() = 0;
	virtual void AddRef() = 0;
	virtual void RemoveRef() = 0;
};

template<class T>
class RefCountedAsset
{
public:
	RefCountedAsset() : m_held(nullptr) {}
	RefCountedAsset(IAsset *held) : m_held(held) { if (held) held->AddRef(); }
	RefCountedAsset(const RefCountedAsset& asset) : m_held(asset.m_held) { asset.m_held->AddRef(); }
	~RefCountedAsset() { if (m_held) m_held->RemoveRef(); }
	RefCountedAsset& operator=(const RefCountedAsset& asset) 
	{ 
		asset.m_held->AddRef();
		if (m_held)
			m_held->RemoveRef();
		m_held = asset.m_held;
		return *this; 
	}
	T *operator->() { return static_cast<T *>(m_held); }
	T& operator* () { return static_cast<T&>(*m_held); }
	T& Get() { return *static_cast<T *>(m_held); }
	void RemoveThisReference() { if (m_held) m_held->RemoveRef(); m_held = nullptr; }
private:
	IAsset *m_held;
};

class IAssetLoader
{
public:
	virtual ~IAssetLoader() = 0;

	virtual void CleanupUnusedMemory() = 0;
	virtual std::string& GetAssetType() = 0;

	virtual IAsset *CreateEmpty(UniqueId id) = 0; 

	virtual void Preload(IAsset *asset, LoadMemoryPointer header) = 0;
	virtual bool CanLoad(IAsset *asset, LoadMemoryPointer loadData) = 0;
	virtual std::vector<AssetLoadRange> Load(IAsset *asset, LoadMemoryPointer loadData) = 0;
	virtual void FinishLoad(IAsset *asset, std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& content, LoadMemoryPointer loadData) = 0;

	virtual void Copy(IAsset *src, IAsset *dest) = 0;

	virtual void Unload(IAsset *asset) = 0;

	virtual void Dispose(IAsset *asset) = 0;
	virtual void Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content,
		std::vector<AssetLoadRange>& ranges) = 0;
};

class IFormatImporter
{
public:
	virtual ~IFormatImporter() = 0;
	virtual std::vector<std::string>& GetFileExtensions() = 0;
	virtual void Import(std::string virtualPath, std::string filePath, std::string ext, void *settings) = 0;
};

class StoredAssetPtr
{
public:
	IAsset *Asset = nullptr;
	std::string BundlePath;
	std::string VirtualPath;
};

using LoadMemoryPointer = ListMemoryPointer *;
using AssetMemoryPointer = ListMemoryPointer *;

class AssetLoadRequest 
{
public:
	AssetLoadRequest(IAssetLoader *loader, IAsset *asset, LoadMemoryPointer ptr) : Loader(loader), Asset(asset), LoadData(ptr) { }
	IAssetLoader *Loader;
	IAsset *Asset;
	LoadMemoryPointer LoadData;
};

class AssetUnloadRequest
{
public:
	AssetUnloadRequest(IAssetLoader *loader, IAsset *asset) : Loader(loader), Asset(asset) { }
	IAssetLoader *Loader;
	IAsset *Asset;
};

class AssetExportRequest 
{
public:
	AssetExportRequest(std::string path, std::vector<UniqueId> assets) : Path(path), Assets(assets) {}
	std::string Path;
	std::vector<UniqueId> Assets;
};

class AssetManager
{
public:
	XENGINEAPI AssetManager(int32_t loadMemSize, int32_t assetMemSize);
	XENGINEAPI ~AssetManager();

	XENGINEAPI void AddAsset(std::string path, IAsset *asset);
	XENGINEAPI void DeleteAsset(UniqueId asset);

	template<class T>
	RefCountedAsset<T> CreateAsset(std::string path, std::string type) { return RefCountedAsset<T>(CreateAssetPtr(path, type)); }
	XENGINEAPI IAsset *CreateAssetPtr(std::string path, std::string type);

	XENGINEAPI void PreloadAssetBundle(std::string filePath);
	XENGINEAPI void ExportAssetBundleToDisc(std::string filePath, std::vector<UniqueId>& assets, bool nonBlocking);
	XENGINEAPI void DeleteBundleAssetsFromMemory(std::string filePath);

	XENGINEAPI LocalMemoryAllocator& GetAssetMemory();
	XENGINEAPI LocalMemoryAllocator& GetLoadMemory();

	XENGINEAPI void Copy(IAsset *src, IAsset *dest);
	XENGINEAPI UniqueId Duplicate(IAsset *src, std::string newPath);

	XENGINEAPI void CleanUnusedMemory();

	XENGINEAPI void RegisterImporter(IFormatImporter *importer);
	XENGINEAPI void RegisterLoader(IAssetLoader *loader);
	XENGINEAPI IAssetLoader *GetLoader(std::string type);

	XENGINEAPI UniqueId GetIdByPath(std::string path);
	XENGINEAPI std::string& GetPathById(UniqueId id);
	XENGINEAPI void ReassignPath(UniqueId id, std::string path);

	XENGINEAPI void PushLoadRequest(IAssetLoader *loader, IAsset *asset, LoadMemoryPointer loadData);
	XENGINEAPI void PushUnloadRequest(IAssetLoader *loader, IAsset *asset);

	XENGINEAPI void ImportAsAssets(std::string path, std::string filePath, void *settings);
	XENGINEAPI std::vector<std::string> MergeAssetBundles(std::vector<std::string>& bundles, uint64_t maxSize);

	template<class T>
	RefCountedAsset<T> GetAsset(UniqueId id) { return RefCountedAsset<T>(GetAssetPtr(id)); }
	XENGINEAPI IAsset *GetAssetPtr(UniqueId id);

private:
	AssetBundleReader m_bundleReader;

	std::atomic_bool m_running;
	std::thread *m_assetLoadingThread;
	void PerformThreadTasks();

	void ExportAssetBundleToDisc(std::string filePath, std::vector<UniqueId>& assets);

	LocalMemoryAllocator m_loadMemory;
	LocalMemoryAllocator m_assetMemory;

	std::unordered_map<std::string, IAssetLoader *> m_loaders;
	std::unordered_map<std::string, IFormatImporter *> m_importers;

	concurrency::concurrent_queue<AssetLoadRequest> m_loadRequests;
	concurrency::concurrent_queue<AssetUnloadRequest> m_unloadRequests;
	concurrency::concurrent_queue<AssetExportRequest> m_exportRequests;

	concurrency::concurrent_unordered_map<std::string, UniqueId> m_pathToId;
	concurrency::concurrent_unordered_map<UniqueId, StoredAssetPtr> m_assets;
};
