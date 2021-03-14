#pragma once

#include "AssetManager.h"
#include "GraphicsDefs.h"
#include "exports.h"

#include "GPURingQueue.h"

#include "FileSpecBuilder.h"

#include <atomic>
#include <shared_mutex>
#include <unordered_map>
#include <map>
#include <glm/glm.hpp>

enum class MeshAssetLoadingState
{
	NotLoaded, LoadingFull, LoadedFull
};

class RenderVertex
{
public:
	glm::vec4 PositionColorR;
	glm::vec4 NormalColorG;
	glm::vec4 TangentColorB;
	glm::vec4 Texcoord0ColorA;
};

class SkinnedVertex
{
public:
	glm::vec4 PositionColorR;
	glm::vec4 NormalColorG;
	glm::vec4 TangentColorB;
	glm::vec4 Texcoord0ColorA;

	glm::vec4 BoneWeights;
	glm::ivec4 BoneIndices;
};

class ClusterData
{
public:
};

using VertexMemoryPointer = ListMemoryPointer *;
using IndexMemoryPointer = ListMemoryPointer *;

XENGINEAPI std::vector<VectorDataFormat>& GetRenderVertexData();
XENGINEAPI std::vector<VectorDataFormat>& GetSkinnedVertexData();

class MeshAssetHeader
{
public:
	int32_t FullVertexCount = 0;
	int32_t FullIndexCount = 0;
	int32_t ClusterCount = 0;
	int32_t VertexElementCount = 0;
};

class MeshAssetLoader;
class MeshAsset : public IAsset
{
public:
	XENGINEAPI MeshAsset(MeshAssetLoader *loader, UniqueId id);
	virtual UniqueId GetId() override;
	virtual std::string& GetTypeName() override;
	virtual void AddRef() override;
	virtual void RemoveRef() override;

	XENGINEAPI void SetVertexFormat(std::vector<VectorDataFormat>& format);
	XENGINEAPI std::vector<VectorDataFormat>& GetVertexFormat();
	XENGINEAPI UniqueId GetVertexFormatId();

	template<class T>
	void SetMeshData(T *vertices, int32_t vCount, int32_t *indices, int32_t iCount)
	{
		SetMeshDataInternal(vertices, sizeof(T), vCount, indices, iCount);
	}
	XENGINEAPI void SetClusterData(ClusterData *data, int32_t count);

	template<class T>
	PinnedLocalMemory<T> GetVertices()
	{
		return m_vAlloc->GetMemory<T>(m_fullLodElementsCPU);
	}
	XENGINEAPI PinnedLocalMemory<int32_t> GetIndices();
	XENGINEAPI PinnedLocalMemory<ClusterData> GetClusterData();
	XENGINEAPI int32_t GetIndexCount();

	MeshAssetHeader& GetMeshHeader() { return m_meshHeader; }

	XENGINEAPI PinnedGPUMemory GetGPUVertices();
	XENGINEAPI PinnedGPUMemory GetGPUIndices();

	XENGINEAPI void Invalidate();

	XENGINEAPI void UpdateGPUVertices();
	XENGINEAPI void UpdateGPUIndices();

	XENGINEAPI void OptimizeCacheAndBuildClusters();

	bool IsLoaded() { return m_loadingState == MeshAssetLoadingState::LoadedFull; }

	XENGINEAPI bool IsFullMeshAvailable();
	XENGINEAPI void LoadFullMesh();
private:
	std::atomic<MeshAssetLoadingState> m_loadingState = MeshAssetLoadingState::NotLoaded;
	friend class MeshAssetLoader;

	LocalMemoryAllocator *m_vAlloc;

	XENGINEAPI void SetMeshDataInternal(void *vertices, int32_t tSize, int32_t vCount, int32_t *indices, int32_t iCount);
	XENGINEAPI LocalMemoryAllocator& GetAssetAllocator();

	MeshAssetLoader *m_loader;

	MeshAssetHeader m_meshHeader;

	AssetMemoryPointer m_clusterData;

	std::vector<VectorDataFormat> m_vertexFormat;
	
	std::shared_ptr<GraphicsSyncObject> m_uploadFullVerticesSync;
	std::shared_ptr<GraphicsSyncObject> m_uploadFullIndicesSync;

	AssetMemoryPointer m_fullLodElementsCPU;
	AssetMemoryPointer m_fullLodIndicesCPU;

	VertexMemoryPointer m_fullLodElementsGPU;
	IndexMemoryPointer m_fullLodIndicesGPU;

	UniqueId m_vertexTypeId;

	UniqueId m_id;
	std::atomic_int m_refCounter;
};

class MeshVertexAllocationPoint
{
public:
	GPUMemoryAllocator *MemoryAllocator;
	std::vector<VectorDataFormat> VertexVectorElements;
	std::vector<int32_t> BytesVertexSizes;
	int32_t BytesPerVertex;
};

class MeshAssetLoader : public IAssetLoader
{
public:
	XENGINEAPI MeshAssetLoader(int32_t minVertexCount, int32_t minIndexCount);
	virtual ~MeshAssetLoader() override;

	XENGINEAPI UniqueId GetVertexType(std::vector<VectorDataFormat> descs);
	XENGINEAPI VertexMemoryPointer AllocateVertices(UniqueId vertexId, int32_t count);
	XENGINEAPI std::shared_ptr<GraphicsSyncObject> UploadVertices(VertexMemoryPointer ptr, UniqueId vertexId, int32_t offset, void *data, int32_t count);
	XENGINEAPI void DeallocateVertices(VertexMemoryPointer ptr, UniqueId vertexId);

	XENGINEAPI IndexMemoryPointer AllocateIndices(int32_t count);
	XENGINEAPI std::shared_ptr<GraphicsSyncObject> UploadIndices(IndexMemoryPointer ptr, int32_t offset, int32_t *data, int32_t count);
	XENGINEAPI void DeallocateIndices(IndexMemoryPointer ptr);

	XENGINEAPI MeshVertexAllocationPoint& GetMeshMemory(UniqueId vertexId);
	XENGINEAPI GPUMemoryAllocator& GetIndexBuffer();

	XENGINEAPI void SetVertexInputState(GraphicsRenderVertexInputState& state, UniqueId vertexId);

	virtual void Copy(IAsset *src, IAsset *dest) override;
	virtual void CleanupUnusedMemory() override;
	virtual std::string& GetAssetType() override;
	virtual IAsset *CreateEmpty(UniqueId id) override;
	virtual void Preload(IAsset *asset, LoadMemoryPointer header) override;
	virtual bool CanLoad(IAsset *asset, LoadMemoryPointer loadData) override;
	virtual std::vector<AssetLoadRange> Load(IAsset *asset, LoadMemoryPointer loadData) override;
	virtual void FinishLoad(IAsset *asset, std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& content, LoadMemoryPointer loadData) override;
	virtual void Unload(IAsset *asset) override;
	virtual void Dispose(IAsset *asset) override;
	virtual void Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content,
		std::vector<AssetLoadRange>& ranges) override;
private:
	FileSpec m_meshSpec;

	int32_t m_minVertexCount, m_minIndexCount;

	GraphicsContext *m_context;

	GPUUploadRingQueue m_uploadQueue;
	GPUMemoryAllocator m_indexAllocator;

	std::shared_mutex m_vertexTypeMutex;
	std::map<std::vector<VectorDataFormat>, UniqueId> m_elementsToType;
	std::unordered_map<UniqueId, MeshVertexAllocationPoint> m_vertexTypes;
};
