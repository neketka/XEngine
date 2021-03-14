#pragma once

#include "AssetManager.h"
#include "GraphicsDefs.h"
#include "GPURingQueue.h"
#include "FileSpecBuilder.h"

#include "exports.h"

#include <memory>

class TextureAssetLoader;

// Cube convention: top-left (0, 0), back, front, left, right, bottom, top

class TextureAsset : public IAsset
{
public:
	XENGINEAPI TextureAsset(TextureAssetLoader *loader, UniqueId id);

	virtual UniqueId GetId() override;
	virtual std::string& GetTypeName() override;
	virtual void AddRef() override;
	virtual void RemoveRef() override;

	XENGINEAPI void SetFormat(int32_t width, int32_t height, int32_t depth, int32_t levels, ImageType type, VectorDataFormat format, VectorDataFormat pixelFormat);
	XENGINEAPI void SetCPUData(int32_t level, void *data);

	XENGINEAPI glm::ivec4 GetMaxSizeAndLevelCount();
	XENGINEAPI ImageType GetType();
	XENGINEAPI VectorDataFormat GetFormat();
	XENGINEAPI int32_t GetBytesPerPixelGPU();
	XENGINEAPI int32_t GetBytesPerPixelCPU();

	XENGINEAPI int32_t GetMipLevelByteCountOnGPU(int32_t level);
	XENGINEAPI int32_t GetMipLevelByteCountOnCPU(int32_t level);
	XENGINEAPI glm::ivec3 GetMipLevelSize(int32_t level);

	XENGINEAPI void LoadAllLevelsFully();

	XENGINEAPI bool StreamLevelToCPU(int32_t level);
	XENGINEAPI bool DeleteCPULevel(int32_t level);
	XENGINEAPI PinnedLocalMemory<char> GetCPULevelMemory(int32_t level);
	XENGINEAPI std::vector<bool>& GetLevelsOnCPU();
	XENGINEAPI std::vector<bool>& GetLevelsStreamingToCPU();

	XENGINEAPI void BuildGPUImage(GraphicsCommandBuffer *cmdBuf, int32_t minLevel, int32_t maxLevel);
	XENGINEAPI bool StreamLevelToGPU(GraphicsCommandBuffer *cmdBuf, int32_t level);
	XENGINEAPI bool InvalidateGPULevel(int32_t level);
	XENGINEAPI std::vector<bool>& GetLevelsOnGPU();
	XENGINEAPI std::vector<bool>& GetLevelsStreamingToGPU();
	XENGINEAPI void SetValidGPUImageBounds(int32_t minLevel, int32_t maxLevel);

	XENGINEAPI std::shared_ptr<GraphicsImageView> GetImage();
	XENGINEAPI glm::ivec2 GetImageLevelRange();
	XENGINEAPI glm::ivec2 GetImageValidLevelRange();
private:
	friend class TextureAssetLoader;

	void CheckGpuStream();

	TextureAssetLoader *m_loader;
	UniqueId m_id;
	std::atomic_int m_refCount;

	GraphicsContext *m_context;

	glm::ivec4 m_sizeLevels;
	ImageType m_type;
	VectorDataFormat m_format;
	int32_t m_bytesPerPixel;
	int32_t m_bytesPerPixelCPU;
	std::vector<int32_t> m_mipBytesSizes;
	std::vector<int32_t> m_mipPixelBytesSizes;

	VectorDataFormat m_pixelFormat;

	bool m_shouldCheckGPUStream = false;

	std::vector<bool> m_levelsOnCpu;
	std::vector<bool> m_levelsOnGpu;

	std::vector<bool> m_levelsStreamingToCpuStatus;
	std::vector<bool> m_levelsStreamingToGpuStatus;
	std::vector<std::shared_ptr<GraphicsSyncObject>> m_levelsStreamingToGpu;

	std::mutex m_streamingMutex;

	glm::ivec2 m_gpuImageBounds;
	glm::ivec2 m_validGpuImageBounds;

	std::vector<AssetMemoryPointer> m_levelsOnCpuData;
	GraphicsImageObject *m_image;
	GraphicsImageObject *m_oldImg;
	GraphicsSyncObject *m_copyOldImg;

	std::shared_ptr<GraphicsImageView> m_view;
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
	virtual void Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content, std::vector<AssetLoadRange>& ranges) override;

	XENGINEAPI std::shared_ptr<GraphicsSyncObject> UploadTextureData(GraphicsCommandBuffer *buf, GraphicsImageObject *obj, int32_t x, int32_t y, int32_t z,
		int32_t width, int32_t height, int32_t depth, int32_t level, VectorDataFormat format, ImageType type, void *data);
	/*
	XENGINEAPI std::shared_ptr<GraphicsSyncObject> DownloadTextureData(GraphicsCommandBuffer *buf, GraphicsImageObject *obj,
		int32_t x, int32_t y, int32_t z, int32_t width, int32_t height, int32_t depth, int32_t level, VectorDataFormat format, void *dest);
	XENGINEAPI void FinishDownload(std::shared_ptr<GraphicsSyncObject> sync);*/
	XENGINEAPI uint32_t CalcMemUsage(int32_t width, int32_t height, int32_t depth, int32_t level, VectorDataFormat format);
private:

	GraphicsContext *m_context;
	GPUDownloadRingQueue m_tDownloadQueue;
	GPUUploadRingQueue m_tUploadQueue;
	FileSpec m_tSpec;
};