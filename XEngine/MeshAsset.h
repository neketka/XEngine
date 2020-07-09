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
	int FullVertexCount = 0;
	int ReducedVertexCount = 0;
	int FullIndexCount = 0;
	int ReducedIndexCount = 0;
	int ClusterCount = 0;
	int VertexElementCount = 0;

	bool ReducedIsSameAsFull = false;
	bool HasReducedMesh = false;
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
	void SetMeshData(T *vertices, int vCount, int *indices, int iCount, bool reduced)
	{
		SetMeshDataInternal(vertices, sizeof(T), vCount, indices, iCount, reduced);
	}
	XENGINEAPI void SetClusterData(ClusterData *data, int count);

	template<class T>
	PinnedLocalMemory<T> GetVertices(bool reduced)
	{
		if (UseReduced(reduced))
			return m_vAlloc->GetMemory<T>(m_reducedLodElementsCPU);
		else
			return m_vAlloc->GetMemory<T>(m_fullLodElementsCPU);
	}
	XENGINEAPI PinnedLocalMemory<int> GetIndices(bool reduced);
	XENGINEAPI PinnedLocalMemory<ClusterData> GetClusterData();
	XENGINEAPI int GetIndexCount(bool reduced);

	MeshAssetHeader& GetMeshHeader() { return m_meshHeader; }

	XENGINEAPI PinnedGPUMemory GetGPUVertices(bool reduced);
	XENGINEAPI PinnedGPUMemory GetGPUIndices(bool reduced);

	XENGINEAPI void Invalidate(bool reduced);

	XENGINEAPI void UpdateGPUVertices(bool reduced);
	XENGINEAPI void UpdateGPUIndices(bool reduced);

	XENGINEAPI void OptimizeCacheAndBuildClusters();

	XENGINEAPI void SetReducedMeshEnabled(bool state);
	XENGINEAPI void SetUnifiedLOD(bool state);

	bool IsNonPersistentLoaded() { return m_loadingState == MeshAssetLoadingState::LoadedFull; }

	XENGINEAPI bool IsFullMeshAvailable();
	XENGINEAPI bool IsReducedMeshAvailable();
	XENGINEAPI void LoadFullMesh();
private:
	std::atomic<MeshAssetLoadingState> m_loadingState = MeshAssetLoadingState::NotLoaded;
	bool UseReduced(bool reduced) { return (reduced && m_meshHeader.HasReducedMesh) || (m_meshHeader.ReducedIsSameAsFull && 
		m_meshHeader.HasReducedMesh && !reduced); }
	friend class MeshAssetLoader;

	LocalMemoryAllocator *m_vAlloc;

	XENGINEAPI void SetMeshDataInternal(void *vertices, int tSize, int vCount, int *indices, int iCount, bool reduced);
	XENGINEAPI LocalMemoryAllocator& GetAssetAllocator();

	MeshAssetLoader *m_loader;

	MeshAssetHeader m_meshHeader;

	AssetMemoryPointer m_clusterData;

	std::vector<VectorDataFormat> m_vertexFormat;
	
	std::shared_ptr<GraphicsSyncObject> m_uploadFullVerticesSync;
	std::shared_ptr<GraphicsSyncObject> m_uploadFullIndicesSync;

	std::shared_ptr<GraphicsSyncObject> m_uploadReducedVerticesSync;
	std::shared_ptr<GraphicsSyncObject> m_uploadReducedIndicesSync;

	AssetMemoryPointer m_fullLodElementsCPU;
	AssetMemoryPointer m_reducedLodElementsCPU;
	AssetMemoryPointer m_fullLodIndicesCPU;
	AssetMemoryPointer m_reducedLodIndicesCPU;

	VertexMemoryPointer m_fullLodElementsGPU;
	VertexMemoryPointer m_reducedLodElementsGPU;
	IndexMemoryPointer m_fullLodIndicesGPU;
	IndexMemoryPointer m_reducedLodIndicesGPU;

	UniqueId m_vertexTypeId;

	UniqueId m_id;
	std::atomic_int m_refCounter;
};

class MeshVertexAllocationPoint
{
public:
	GPUMemoryAllocator *MemoryAllocator;
	std::vector<VectorDataFormat> VertexVectorElements;
	std::vector<int> BytesVertexSizes;
	int BytesPerVertex;
};

class MeshAssetLoader : public IAssetLoader
{
public:
	XENGINEAPI MeshAssetLoader(int minVertexCount, int minIndexCount);
	virtual ~MeshAssetLoader() override;

	XENGINEAPI UniqueId GetVertexType(std::vector<VectorDataFormat> descs);
	XENGINEAPI VertexMemoryPointer AllocateVertices(UniqueId vertexId, int count);
	XENGINEAPI std::shared_ptr<GraphicsSyncObject> UploadVertices(VertexMemoryPointer ptr, UniqueId vertexId, int offset, void *data, int count);
	XENGINEAPI void DeallocateVertices(VertexMemoryPointer ptr, UniqueId vertexId);

	XENGINEAPI IndexMemoryPointer AllocateIndices(int count);
	XENGINEAPI std::shared_ptr<GraphicsSyncObject> UploadIndices(IndexMemoryPointer ptr, int offset, int *data, int count);
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
	virtual void Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content) override;
private:
	FileSpec m_meshSpec;

	int m_minVertexCount, m_minIndexCount;

	GraphicsContext *m_context;

	GPUUploadRingQueue m_uploadQueue;
	GPUMemoryAllocator m_indexAllocator;

	std::shared_mutex m_vertexTypeMutex;
	std::map<std::vector<VectorDataFormat>, UniqueId> m_elementsToType;
	std::unordered_map<UniqueId, MeshVertexAllocationPoint> m_vertexTypes;
};
