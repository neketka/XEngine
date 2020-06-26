#include "pch.h"
#include "MeshAsset.h"
#include "XEngine.h"

std::string t = "Mesh";

MeshAsset::MeshAsset(MeshAssetLoader *loader, UniqueId id)
{
	m_loader = loader;
	m_id = id;
}

UniqueId MeshAsset::GetId()
{
	return m_id;
}

std::string& MeshAsset::GetTypeName()
{
	return t;
}

void MeshAsset::AddRef()
{
	++m_refCounter;
}

void MeshAsset::RemoveRef()
{
	int newVal = --m_refCounter;
	if (newVal == 0)
	{
		m_loader->Unload(this);
	}
}

void MeshAssetLoader::CleanupUnusedMemory()
{
}

std::string& MeshAssetLoader::GetAssetType()
{
	return t;
}

IAsset *MeshAssetLoader::CreateEmpty(UniqueId id)
{
	return new MeshAsset(this, id);
}

void MeshAssetLoader::Preload(IAsset *asset, LoadMemoryPointer header)
{
}

std::vector<AssetLoadRange> MeshAssetLoader::Load(IAsset *asset, LoadMemoryPointer loadData)
{
	return std::vector<AssetLoadRange>();
}

void MeshAssetLoader::FinishLoad(std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& content, LoadMemoryPointer loadData)
{
}

void MeshAssetLoader::Unload(IAsset *asset)
{
}

void MeshAssetLoader::Dispose(IAsset *asset)
{
}

void MeshAssetLoader::Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content)
{
}

void MeshAsset::SetFormatAsRenderVertex()
{
}

void MeshAsset::SetFormatAsSkinnedVertex()
{
}

void MeshAsset::SetVertexFormat(std::vector<VectorDataFormat>& format)
{
}

std::vector<VectorDataFormat>& MeshAsset::GetVertexFormat()
{
	return m_vertexFormat;
}

UniqueId MeshAsset::GetVertexFormatId()
{
	return UniqueId();
}

void MeshAsset::SetClusterData(ClusterData *data, int count)
{
}

PinnedLocalMemory<int> MeshAsset::GetIndices(bool reduced)
{
	if (reduced)
		return XEngineInstance->GetAssetManager()->GetAssetMemory().GetMemory<int>(m_reducedLodElementsCPU);
	else
		return XEngineInstance->GetAssetManager()->GetAssetMemory().GetMemory<int>(m_fullLodElementsCPU);
}

PinnedLocalMemory<ClusterData> MeshAsset::GetClusterData()
{
	return XEngineInstance->GetAssetManager()->GetAssetMemory().GetMemory<ClusterData>(m_clusterData);
}

PinnedGPUMemory MeshAsset::GetGPUVertices(bool reduced)
{
	return PinnedGPUMemory();
}

PinnedGPUMemory MeshAsset::GetGPUIndices(bool reduced)
{
	return PinnedGPUMemory();
}

void MeshAsset::Invalidate(bool reduced)
{
}

void MeshAsset::UpdateGPUVertices()
{
}

void MeshAsset::UpdateGPUIndices()
{
}

void MeshAsset::SetReducedMeshEnabled(bool state)
{
}

void MeshAsset::SetPersistFullMesh(bool state)
{
}

bool MeshAsset::IsFullMeshAvailable()
{
	return false;
}

bool MeshAsset::IsReducedMeshAvailable()
{
	return false;
}

void MeshAsset::LoadFullMesh()
{
}

void MeshAsset::SetMeshDataInternal(void * vertices, int tSize, int vCount, int * indices, int iCount, bool reduced)
{
}
