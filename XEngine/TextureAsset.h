#pragma once

#include "AssetManager.h"
#include "GraphicsDefs.h"
#include "GPURingQueue.h"
#include "FileSpecBuilder.h"

#include "exports.h"

#include <memory>

class TextureAssetView
{
public:
	std::shared_ptr<GraphicsImageObject> Image;
	int MinLevelRange;
	int MaxLevelRange;
};

class TextureAssetLoader;

class TextureAsset : public IAsset
{
public:
	XENGINEAPI TextureAsset(TextureAssetLoader *loader, UniqueId id);

	virtual UniqueId GetId() override;
	virtual std::string& GetTypeName() override;
	virtual void AddRef() override;
	virtual void RemoveRef() override;

	XENGINEAPI void SetFormat(int width, int height, int depth, int levels, ImageType type, VectorDataFormat format);
	XENGINEAPI void SetData(int x, int y, int z, int width, int height, int depth, int level, VectorDataFormat format, void *data);
	XENGINEAPI void SetMaximumStreamingLevel(int lod);
	XENGINEAPI glm::ivec4 GetMaxSizeAndLevelCount();
	XENGINEAPI ImageType GetType();
	XENGINEAPI VectorDataFormat GetFormat();
	XENGINEAPI int GetBytesPerPixel();
	XENGINEAPI int GetMipLevelByteCount(int level);
	XENGINEAPI glm::ivec3 GetMipLevelSize(int level);
	XENGINEAPI void RetainLowestMipLevelOnUnload(bool state);
	XENGINEAPI void Download();
	XENGINEAPI bool AnyDownloadsInProgress();
	XENGINEAPI void DeleteDuplicatedDataFromGPU();
	XENGINEAPI void DeleteDuplicatedDataFromLocal();
	XENGINEAPI PinnedLocalMemory<char> GetData(int level);
	XENGINEAPI TextureAssetView GetTexture();
private:
	friend class TextureAssetLoader;

	std::shared_ptr<GraphicsSyncObject> m_downloadInProgress;

	void SetData(int x, int y, int z, int width, int height, int depth, int level, VectorDataFormat format, PinnedLocalMemory<char> data);
	void BuildView();

	TextureAssetLoader *m_loader;
	UniqueId m_id;
	std::atomic_int m_refCount;

	bool m_keepLowestLevel;
	glm::ivec4 m_sizeLevels;
	VectorDataFormat m_format;
	VectorDataFormat m_pixelFormat;
	int m_bytesPerPixel;
	ImageType m_type;
	std::vector<int> m_mipBytesSizes;
	
	std::shared_mutex m_availabilityAccess;
	TextureAssetView m_view;
	std::vector<bool> m_available;
	std::vector<LoadMemoryPointer> m_buffered;
};

class TextureAssetLoader : public IAssetLoader
{
public:
	XENGINEAPI TextureAssetLoader();
	virtual void CleanupUnusedMemory() override;
	virtual std::string& GetAssetType() override;
	virtual IAsset *CreateEmpty(UniqueId id) override;
	virtual void Preload(IAsset *asset, LoadMemoryPointer header) override;
	virtual bool CanLoad(IAsset *asset, LoadMemoryPointer loadData) override;
	virtual std::vector<AssetLoadRange> Load(IAsset *asset, LoadMemoryPointer loadData) override;
	virtual void FinishLoad(IAsset *asset, std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& content, LoadMemoryPointer loadData) override;
	virtual void Copy(IAsset *src, IAsset *dest) override;
	virtual void Unload(IAsset *asset) override;
	virtual void Dispose(IAsset *asset) override;
	virtual void Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content) override;

	XENGINEAPI std::shared_ptr<GraphicsSyncObject> UploadTextureData(GraphicsImageObject *obj, int x, int y, int z,
		int width, int height, int depth, VectorDataFormat format, void *data);
	XENGINEAPI std::shared_ptr<GraphicsSyncObject> DownloadTextureData(GraphicsImageObject *obj, 
		int x, int y, int z, int width, int height, int depth, VectorDataFormat format, void *dest);
	XENGINEAPI void FinishDownload(std::shared_ptr<GraphicsSyncObject> sync);
private:
	GraphicsContext *m_context;
	GPUDownloadRingQueue m_tDownloadQueue;
	GPUUploadRingQueue m_tUploadQueue;
	FileSpec m_tSpec;
};