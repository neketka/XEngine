#pragma once

#include "AssetManager.h"
#include "GLShaderCompilation.h"
#include "FileSpecBuilder.h"

#include <atomic>

enum class ShaderAssetCode
{
	GlSpirv, VkSpirv
};

class ShaderAssetLoader;

class ShaderAsset : public IAsset
{
public:
	ShaderAsset(ShaderAssetLoader *loader, UniqueId id);

	virtual UniqueId GetId() override;
	virtual std::string& GetTypeName() override;
	virtual void AddRef() override;
	virtual void RemoveRef() override;

	void SetShaderCodeMode(ShaderAssetCode code);
	void UploadCode(void *code, int32_t dataSize);

	void AddSpecializationInformation(std::string flagName, std::vector<std::pair<uint32_t, uint32_t>> specialization);

	GraphicsSpecificSpecializationData *CreateSpecialization(std::vector<std::string> flags, std::string entryPoint);
	GraphicsShader *GetShader();

	void SetStage(ShaderStageBit stage);
	ShaderStageBit GetStage();
private:
	friend ShaderAssetLoader;

	std::atomic_int m_refCounter;

	ShaderAssetLoader *m_loader;
	UniqueId m_id;

	ShaderAssetCode m_mode;
	ShaderStageBit m_stage;

	GraphicsShader *m_shader;
	GraphicsSpecificShaderCode *m_shaderCode;

	std::unordered_map<std::string, std::vector<std::pair<uint32_t, uint32_t>>> m_flags;

	AssetMemoryPointer m_glCode;
	int32_t m_glCodeLength;

	AssetMemoryPointer m_vkCode;
	int32_t m_vkCodeLength;
};

class ShaderAssetLoader : public IAssetLoader
{
public:
	ShaderAssetLoader();
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
	virtual void Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content,
		std::vector<AssetLoadRange>& ranges) override;

	void AddIncludePath(std::string folder);
	std::string ResolveIncludedPath(std::string path);
private:
	FileSpec m_spec;
};