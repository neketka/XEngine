#pragma once

#include "AssetManager.h"
#include "GraphicsDefs.h"

#include "GPURingQueue.h"

#include <atomic>
#include <shared_mutex>
#include <unordered_map>
#include <glm/glm.hpp>

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
};

class ClusterData
{
public:
};

using VertexMemoryPointer = ListMemoryPointer *;
using IndexMemoryPointer = ListMemoryPointer *;

class MeshAssetLoader;
class MeshAsset : public IAsset
{
public:
	MeshAsset(MeshAssetLoader *loader, UniqueId id);
	virtual UniqueId GetId() override;
	virtual std::string& GetTypeName() override;
	virtual void AddRef() override;
	virtual void RemoveRef() override;

	void SetFormatAsRenderVertex();
	void SetFormatAsSkinnedVertex();

	void SetVertexFormat(std::vector<VectorDataFormat>& format);
	std::vector<VectorDataFormat>& GetVertexFormat();
	UniqueId GetVertexFormatId();

	template<class T>
	void SetMeshData(T *vertices, int vCount, int *indices, int iCount, bool reduced)
	{
		SetMeshDataInternal(vertices, sizeof(T), vCount, indices, iCount, reduced);
	}
	void SetClusterData(ClusterData *data, int count);

	template<class T>
	PinnedLocalMemory<T> GetVertices(bool reduced)
	{

	}
	PinnedLocalMemory<int> GetIndices(bool reduced);
	PinnedLocalMemory<ClusterData> GetClusterData();

	PinnedGPUMemory GetGPUVertices(bool reduced);
	PinnedGPUMemory GetGPUIndices(bool reduced);

	void Invalidate(bool reduced);

	void UpdateGPUVertices();
	void UpdateGPUIndices();

	void SetReducedMeshEnabled(bool state);
	void SetPersistFullMesh(bool state);

	bool IsFullMeshAvailable();
	bool IsReducedMeshAvailable();
	void LoadFullMesh();
private:
	friend class MeshAssetLoader;

	void SetMeshDataInternal(void *vertices, int tSize, int vCount, int *indices, int iCount, bool reduced);

	MeshAssetLoader *m_loader;

	AssetMemoryPointer m_clusterData;
	int m_clusterCount;

	std::vector<VectorDataFormat> m_vertexFormat;
	
	std::shared_ptr<GraphicsSyncObject *> m_uploadSync;

	AssetMemoryPointer m_fullLodElementsCPU;
	AssetMemoryPointer m_reducedLodElementsCPU;
	AssetMemoryPointer m_fullLodIndicesCPU;
	AssetMemoryPointer m_reducedLodIndicesCPU;

	int m_fullVertexCount;
	int m_reducedVertexCount;
	int m_fullIndexCount;
	int m_reducedIndexCount;

	bool m_reducedIsSameAsFull;
	bool m_noReducedMesh;

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
	GPUMemoryAllocator MemoryAllocator;
	std::vector<VectorDataFormat> VertexVectorElements;
	int BytesPerVertex;
};

class MeshAssetLoader : public IAssetLoader
{
public:
	MeshAssetLoader(int minVertexCount, int minIndexCount, int stagingBufferSize);
	virtual ~MeshAssetLoader() override;

	UniqueId GetVertexType(std::vector<VectorDataFormat> descs);
	VertexMemoryPointer AllocateVertices(UniqueId vertexId, int count);
	void UploadVertices(VertexMemoryPointer ptr, UniqueId vertexId, int offset, void *data, int size);
	void DeallocateVertices(VertexMemoryPointer ptr, UniqueId vertexId);

	IndexMemoryPointer AllocateIndices(int count);
	void UploadIndices(IndexMemoryPointer ptr, int offset, int *data, int count);
	void DeallocateIndices(IndexMemoryPointer ptr);

	MeshVertexAllocationPoint& GetMeshMemory(UniqueId vertexId);
	GraphicsMemoryBuffer *GetIndexBuffer();

	virtual void CleanupUnusedMemory() override;
	virtual std::string& GetAssetType() override;
	virtual IAsset *CreateEmpty(UniqueId id) override;
	virtual void Preload(IAsset *asset, LoadMemoryPointer header) override;
	virtual std::vector<AssetLoadRange> Load(IAsset *asset, LoadMemoryPointer loadData) override;
	virtual void FinishLoad(std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& content, LoadMemoryPointer loadData) override;
	virtual void Unload(IAsset *asset) override;
	virtual void Dispose(IAsset *asset) override;
	virtual void Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content) override;
private:
	int m_minVertexCount, m_minIndexCount;
	GPUUploadRingQueue m_uploadQueue;

	std::shared_mutex m_vertexTypeMutex;
	std::unordered_map<std::vector<VectorDataFormat>, UniqueId> m_elementsToType;
	std::unordered_map<UniqueId, MeshVertexAllocationPoint> m_vertexTypes;
};
