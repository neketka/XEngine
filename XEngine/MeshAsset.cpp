#include "pch.h"
#include "MeshAsset.h"

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
