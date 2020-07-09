#include "pch.h"
#include "MeshAsset.h"
#include "XEngine.h"
#include "GLImage.h"

std::string t = "Mesh";

MeshAsset::MeshAsset(MeshAssetLoader *loader, UniqueId id) : m_clusterData(nullptr), m_fullLodElementsCPU(nullptr), m_reducedLodElementsCPU(nullptr),
	m_fullLodIndicesCPU(nullptr), m_reducedLodIndicesCPU(nullptr), m_fullLodElementsGPU(nullptr), m_reducedLodElementsGPU(nullptr),
	m_fullLodIndicesGPU(nullptr), m_reducedLodIndicesGPU(nullptr), m_refCounter(0)
{
	m_loader = loader;
	m_vAlloc = &XEngineInstance->GetAssetManager()->GetAssetMemory();
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
	MeshAsset *mAsset = static_cast<MeshAsset *>(asset);

	FileSpecImporter importer(m_meshSpec, header);
	mAsset->m_meshHeader = importer.Get<MeshAssetHeader>("meshHeader");

	importer.CopyArray("vectorDataFormat", mAsset->m_vertexFormat);
	mAsset->m_vertexTypeId = GetVertexType(mAsset->m_vertexFormat);

	if (mAsset->m_meshHeader.HasReducedMesh)
	{
		mAsset->SetMeshData(importer.GetArray<char>("reducedVertices"), mAsset->m_meshHeader.ReducedVertexCount,
			importer.GetArray<int>("reducedIndices"), mAsset->m_meshHeader.ReducedIndexCount, true);
		if (mAsset->m_meshHeader.ReducedIsSameAsFull)
		{
			mAsset->SetClusterData(importer.GetArray<ClusterData>("reducedCluster"), importer.GetArraySize("reducedCluster"));
		}
	}
}

std::vector<AssetLoadRange> MeshAssetLoader::Load(IAsset *asset, LoadMemoryPointer loadData)
{
	MeshAsset *mAsset = static_cast<MeshAsset *>(asset);

	if (mAsset->m_meshHeader.ReducedIsSameAsFull && mAsset->m_meshHeader.HasReducedMesh 
		|| mAsset->m_loadingState == MeshAssetLoadingState::NotLoaded)
	{
		return {};
	}

	std::vector<AssetLoadRange> ranges(3);
	ranges[0].ByteOffset = 0;
	ranges[0].Size = mAsset->m_meshHeader.FullVertexCount * m_vertexTypes[mAsset->GetVertexFormatId()].BytesPerVertex;
	ranges[1].ByteOffset = ranges[0].Size;
	ranges[1].Size = mAsset->m_meshHeader.FullIndexCount * sizeof(int);
	ranges[2].ByteOffset = ranges[1].Size;
	ranges[2].Size = mAsset->m_meshHeader.ClusterCount * sizeof(ClusterData);

	return ranges;
}

void MeshAssetLoader::FinishLoad(IAsset *asset, std::vector<AssetLoadRange>& ranges, std::vector<LoadMemoryPointer>& content, LoadMemoryPointer loadData)
{
	MeshAsset *mAsset = static_cast<MeshAsset *>(asset);

	if (ranges.empty())
		return;

	LocalMemoryAllocator& loadMem = XEngineInstance->GetAssetManager()->GetLoadMemory();

	mAsset->SetMeshData<char>(loadMem.GetMemory<char>(content[0]).GetData(), mAsset->GetMeshHeader().FullVertexCount,
		loadMem.GetMemory<int>(content[1]).GetData(), mAsset->GetMeshHeader().FullIndexCount, false);
	mAsset->SetClusterData(loadMem.GetMemory<ClusterData>(content[2]).GetData(), mAsset->GetMeshHeader().ClusterCount);

	mAsset->m_loadingState = MeshAssetLoadingState::LoadedFull;
}

void MeshAssetLoader::Unload(IAsset *asset)
{
	MeshAsset *mAsset = static_cast<MeshAsset *>(asset);

	if (!mAsset->UseReduced(false))
		mAsset->Invalidate(false);
}

void MeshAssetLoader::Dispose(IAsset *asset)
{
	MeshAsset *mAsset = static_cast<MeshAsset *>(asset);

	mAsset->Invalidate(true);
	mAsset->Invalidate(false);

	delete mAsset;
}

void MeshAssetLoader::Export(IAsset *asset, AssetDescriptorPreHeader& preHeader, LoadMemoryPointer& header, LoadMemoryPointer& content)
{
	MeshAsset *mAsset = static_cast<MeshAsset *>(asset);
	LocalMemoryAllocator& loadMem = XEngineInstance->GetAssetManager()->GetLoadMemory();
	LocalMemoryAllocator& assetMem = XEngineInstance->GetAssetManager()->GetAssetMemory();

	int bPerV = m_vertexTypes[mAsset->GetVertexFormatId()].BytesPerVertex;
	int vSize = mAsset->m_meshHeader.FullVertexCount * bPerV;
	int iSize = mAsset->m_meshHeader.FullIndexCount * sizeof(int);
	int cSize = mAsset->m_meshHeader.ClusterCount * sizeof(ClusterData);

	bool doesAssetExist = !(mAsset->m_meshHeader.HasReducedMesh && mAsset->m_meshHeader.ReducedIsSameAsFull);

	preHeader.Id = asset->GetId();
	strcpy_s(preHeader.AssetType, 64, t.c_str());

	FileSpecExporter exporter(m_meshSpec);

	exporter.SetArraySize("vectorDataFormat", mAsset->m_vertexFormat.size());
	exporter.SetArraySize("reducedVertices", mAsset->m_meshHeader.HasReducedMesh ? mAsset->m_meshHeader.ReducedVertexCount * bPerV : 0);
	exporter.SetArraySize("reducedIndices", mAsset->m_meshHeader.HasReducedMesh ? mAsset->m_meshHeader.ReducedIndexCount : 0);
	exporter.SetArraySize("reducedCluster", mAsset->m_meshHeader.HasReducedMesh && mAsset->m_meshHeader.ReducedIsSameAsFull ?
		mAsset->m_meshHeader.ClusterCount : 0);

	header = exporter.AllocateSpace();
	exporter.Set("meshHeader", mAsset->m_meshHeader);
	exporter.SetArrayData("vectorDataFormat", mAsset->m_vertexFormat.data());
	if (mAsset->m_meshHeader.HasReducedMesh)
	{
		exporter.SetArrayData("reducedVertices", mAsset->GetVertices<char>(true).GetData());
		exporter.SetArrayData("reducedIndices", mAsset->GetVertices<char>(true).GetData());
		if (mAsset->m_meshHeader.ReducedIsSameAsFull)
			exporter.SetArrayData("reducedCluster", mAsset->GetClusterData().GetData());
	}

	preHeader.HeaderSize = exporter.GetTotalByteSize();
	preHeader.AssetSize = doesAssetExist ? (vSize + iSize + cSize) : 0;

	content = loadMem.RequestSpace(preHeader.AssetSize);
	if (doesAssetExist)
	{
		PinnedLocalMemory<char> cMem = loadMem.GetMemory<char>(content);
		std::memcpy(cMem.GetData(), mAsset->GetVertices<char>(false).GetData(), vSize);
		std::memcpy(cMem.GetData() + vSize, mAsset->GetIndices(false).GetData(), iSize);
		std::memcpy(cMem.GetData() + vSize + iSize, mAsset->GetClusterData().GetData(), cSize);
	}
}

bool MeshAssetLoader::CanLoad(IAsset *asset, LoadMemoryPointer loadData)
{
	MeshAsset *mAsset = static_cast<MeshAsset *>(asset);
	MeshVertexAllocationPoint& point = m_vertexTypes[mAsset->m_vertexTypeId];

	int vSize = point.BytesPerVertex * mAsset->m_meshHeader.FullVertexCount;
	int iSize = sizeof(int) * mAsset->m_meshHeader.FullIndexCount;

	return XEngineInstance->GetAssetManager()->GetAssetMemory().WillFit(vSize + iSize);
}

void MeshAssetLoader::Copy(IAsset *src, IAsset *dest)
{
	MeshAsset *mSrc = static_cast<MeshAsset *>(src);
	MeshAsset *mDest = static_cast<MeshAsset *>(dest);

	mDest->SetUnifiedLOD(mSrc->m_meshHeader.ReducedIsSameAsFull);
	mDest->SetReducedMeshEnabled(mSrc->m_meshHeader.HasReducedMesh);
	mDest->SetVertexFormat(mSrc->GetVertexFormat());

	mDest->Invalidate(false);
	mDest->Invalidate(true);

	if (mSrc->m_meshHeader.HasReducedMesh && !mSrc->m_meshHeader.ReducedIsSameAsFull)
	{
		mDest->SetMeshData(mSrc->GetVertices<char>(true).GetData(), mSrc->m_meshHeader.ReducedVertexCount, mSrc->GetIndices(true).GetData(),
			mSrc->m_meshHeader.ReducedIndexCount, true);
		mDest->SetMeshData(mSrc->GetVertices<char>(false).GetData(), mSrc->m_meshHeader.FullVertexCount, mSrc->GetIndices(false).GetData(),
			mSrc->m_meshHeader.FullIndexCount, false);
	}
	else
	{
		mDest->SetMeshData(mSrc->GetVertices<char>(false).GetData(), mSrc->m_meshHeader.FullVertexCount, mSrc->GetIndices(false).GetData(),
			mSrc->m_meshHeader.FullIndexCount, false);
	}

	mDest->SetClusterData(mSrc->GetClusterData().GetData(), mSrc->m_meshHeader.ClusterCount);
}

void MeshAsset::SetVertexFormat(std::vector<VectorDataFormat>& format)
{
	m_vertexFormat = format;
	m_vertexTypeId = m_loader->GetVertexType(format);
	m_meshHeader.VertexElementCount = format.size();
}

std::vector<VectorDataFormat>& MeshAsset::GetVertexFormat()
{
	return m_vertexFormat;
}

UniqueId MeshAsset::GetVertexFormatId()
{
	return m_vertexTypeId;
}

void MeshAsset::SetClusterData(ClusterData *data, int count)
{
	if (m_clusterData)
		XEngineInstance->GetAssetManager()->GetAssetMemory().FreeSpace(m_clusterData);
	m_clusterData = XEngineInstance->GetAssetManager()->GetAssetMemory().RequestSpace(count * sizeof(ClusterData));
	PinnedLocalMemory<ClusterData> d = XEngineInstance->GetAssetManager()->GetAssetMemory().GetMemory<ClusterData>(m_clusterData);
	std::memcpy(d.GetData(), data, count * sizeof(ClusterData));
	m_meshHeader.ClusterCount = count;
}

PinnedLocalMemory<int> MeshAsset::GetIndices(bool reduced)
{
	if (UseReduced(reduced))
		return XEngineInstance->GetAssetManager()->GetAssetMemory().GetMemory<int>(m_reducedLodIndicesCPU);
	else
		return XEngineInstance->GetAssetManager()->GetAssetMemory().GetMemory<int>(m_fullLodIndicesCPU);
}

PinnedLocalMemory<ClusterData> MeshAsset::GetClusterData()
{
	return XEngineInstance->GetAssetManager()->GetAssetMemory().GetMemory<ClusterData>(m_clusterData);
}

int MeshAsset::GetIndexCount(bool reduced)
{
	return UseReduced(reduced) ? m_meshHeader.ReducedIndexCount : m_meshHeader.FullIndexCount;
}

PinnedGPUMemory MeshAsset::GetGPUVertices(bool reduced)
{
	if (UseReduced(reduced))
	{
		//m_uploadReducedVerticesSync->Wait(1e12);
		return m_loader->GetMeshMemory(m_vertexTypeId).MemoryAllocator->GetMemory(m_reducedLodElementsGPU);
	}
	else
	{
		//m_uploadFullVerticesSync->Wait(1e12);
		return m_loader->GetMeshMemory(m_vertexTypeId).MemoryAllocator->GetMemory(m_fullLodElementsGPU);
	}
}

PinnedGPUMemory MeshAsset::GetGPUIndices(bool reduced)
{
	if (UseReduced(reduced))
	{
		//m_uploadReducedIndicesSync->Wait(1e12);
		return m_loader->GetIndexBuffer().GetMemory(m_reducedLodIndicesGPU);
	}
	else
	{
		//m_uploadFullIndicesSync->Wait(1e12);
		return m_loader->GetIndexBuffer().GetMemory(m_fullLodIndicesGPU);
	}
}

void MeshAsset::UpdateGPUVertices(bool reduced)
{
	if (UseReduced(reduced))
	{
		if (m_uploadReducedVerticesSync)
			m_uploadReducedVerticesSync->Wait(1e12);
		m_uploadReducedVerticesSync = m_loader->UploadVertices(m_reducedLodElementsGPU, m_vertexTypeId, 0, 
			GetVertices<char>(reduced).GetData(), m_meshHeader.ReducedVertexCount);
	}
	else
	{
		if (m_uploadFullVerticesSync)
			m_uploadFullVerticesSync->Wait(1e12);
		m_uploadFullVerticesSync = m_loader->UploadVertices(m_fullLodElementsGPU, m_vertexTypeId, 0,
			GetVertices<char>(reduced).GetData(), m_meshHeader.FullVertexCount);
	}
}

void MeshAsset::UpdateGPUIndices(bool reduced)
{
	if (UseReduced(reduced))
	{
		if (m_uploadReducedIndicesSync)
			m_uploadReducedIndicesSync->Wait(1e12);
		m_uploadReducedIndicesSync = m_loader->UploadIndices(m_reducedLodIndicesGPU, 0,
			GetVertices<int>(reduced).GetData(), m_meshHeader.ReducedIndexCount);
	}
	else
	{
		if (m_uploadFullIndicesSync)
			m_uploadFullIndicesSync->Wait(1e12);
		m_uploadFullIndicesSync = m_loader->UploadIndices(m_fullLodIndicesGPU, 0,
			GetVertices<int>(reduced).GetData(), m_meshHeader.FullIndexCount);
	}
}

void MeshAsset::SetReducedMeshEnabled(bool state)
{
	m_meshHeader.HasReducedMesh = state;
}

void MeshAsset::SetUnifiedLOD(bool state)
{
	m_meshHeader.ReducedIsSameAsFull = state;
}

bool MeshAsset::IsFullMeshAvailable()
{
	return UseReduced(false) ? IsReducedMeshAvailable() : (m_uploadFullVerticesSync && m_uploadFullIndicesSync ? (m_uploadFullVerticesSync->GetCurrentStatus() &&
		m_uploadFullIndicesSync->GetCurrentStatus()) : false);
}

bool MeshAsset::IsReducedMeshAvailable()
{
	return UseReduced(true) ? IsFullMeshAvailable() : (m_uploadReducedVerticesSync && m_uploadReducedIndicesSync) ? (m_uploadReducedVerticesSync->GetCurrentStatus() &&
		m_uploadReducedIndicesSync->GetCurrentStatus()) : false;
}

void MeshAsset::LoadFullMesh()
{
	m_loadingState =  MeshAssetLoadingState::LoadingFull;
	XEngineInstance->GetAssetManager()->PushLoadRequest(m_loader, this, nullptr);
}

void MeshAsset::SetMeshDataInternal(void *vertices, int tSize, int vCount, int *indices, int iCount, bool reduced)
{
	LocalMemoryAllocator& assetMem = XEngineInstance->GetAssetManager()->GetAssetMemory();

	if (UseReduced(reduced))
	{
		Invalidate(true);

		m_reducedLodElementsGPU = m_loader->AllocateVertices(m_vertexTypeId, vCount);
		m_reducedLodIndicesGPU = m_loader->AllocateIndices(iCount);
		m_reducedLodElementsCPU = assetMem.RequestSpace(m_loader->GetMeshMemory(m_vertexTypeId).BytesPerVertex * vCount);
		m_reducedLodIndicesCPU = assetMem.RequestSpace(sizeof(int) * iCount);

		if (!vertices || !indices)
			return;

		m_uploadReducedVerticesSync = m_loader->UploadVertices(m_reducedLodElementsGPU, m_vertexTypeId, 0, vertices, vCount);
		m_uploadReducedIndicesSync = m_loader->UploadIndices(m_reducedLodIndicesGPU, 0, indices, iCount);

		PinnedLocalMemory<char> vMem = assetMem.GetMemory<char>(m_reducedLodElementsCPU);
		PinnedLocalMemory<char> iMem = assetMem.GetMemory<char>(m_reducedLodIndicesCPU);

		std::memcpy(vMem.GetData(), vertices, vCount * m_loader->GetMeshMemory(m_vertexTypeId).BytesPerVertex);
		std::memcpy(iMem.GetData(), indices, iCount * sizeof(int));

		m_meshHeader.ReducedVertexCount = vCount;
		m_meshHeader.ReducedIndexCount = iCount;
	}
	else
	{
		m_loadingState = MeshAssetLoadingState::NotLoaded;
		Invalidate(false);
		m_loadingState = MeshAssetLoadingState::LoadingFull;

		m_fullLodElementsGPU = m_loader->AllocateVertices(m_vertexTypeId, vCount);
		m_fullLodIndicesGPU = m_loader->AllocateIndices(iCount);
		m_fullLodElementsCPU = assetMem.RequestSpace(m_loader->GetMeshMemory(m_vertexTypeId).BytesPerVertex * vCount);
		m_fullLodIndicesCPU = assetMem.RequestSpace(sizeof(int) * iCount);

		if (!vertices || !indices)
			return;

		m_uploadFullVerticesSync = m_loader->UploadVertices(m_fullLodElementsGPU, m_vertexTypeId, 0, vertices, vCount);
		m_uploadFullIndicesSync = m_loader->UploadIndices(m_fullLodIndicesGPU, 0, indices, iCount);

		PinnedLocalMemory<char> vMem = assetMem.GetMemory<char>(m_fullLodElementsCPU);
		PinnedLocalMemory<char> iMem = assetMem.GetMemory<char>(m_fullLodIndicesCPU);

		std::memcpy(vMem.GetData(), vertices, vCount * m_loader->GetMeshMemory(m_vertexTypeId).BytesPerVertex);
		std::memcpy(iMem.GetData(), indices, iCount * sizeof(int));

		m_loadingState = MeshAssetLoadingState::LoadedFull;

		m_meshHeader.FullVertexCount = vCount;
		m_meshHeader.FullIndexCount = iCount;

	}
}

LocalMemoryAllocator& MeshAsset::GetAssetAllocator()
{
	return XEngineInstance->GetAssetManager()->GetAssetMemory();
}

MeshAssetLoader::MeshAssetLoader(int minVertexCount, int minIndexCount)
	: m_uploadQueue(1e5), m_minVertexCount(minVertexCount), m_minIndexCount(minIndexCount),
	m_indexAllocator(minIndexCount, 16384, true)
{
	m_context = XEngineInstance->GetInterface<DisplayInterface>(HardwareInterfaceType::Display)->GetGraphicsContext();
	
	m_meshSpec.Add<MeshAssetHeader>("meshHeader");
	m_meshSpec.AddArray<VectorDataFormat>("vectorDataFormat");
	m_meshSpec.AddArray<char>("reducedVertices");
	m_meshSpec.AddArray<int>("reducedIndices");
	m_meshSpec.AddArray<ClusterData>("reducedCluster");
}

MeshAssetLoader::~MeshAssetLoader()
{
	for (auto pair : m_vertexTypes)
	{
		delete pair.second.MemoryAllocator;
	}
}

UniqueId MeshAssetLoader::GetVertexType(std::vector<VectorDataFormat> descs)
{
	m_vertexTypeMutex.lock_shared();
	auto iter = m_elementsToType.find(descs);
	if (iter == m_elementsToType.end())
	{
		m_vertexTypeMutex.unlock_shared();
		m_vertexTypeMutex.lock();
		UniqueId id = m_elementsToType[descs] = GenerateID();

		MeshVertexAllocationPoint& alloc = m_vertexTypes[id];

		alloc.VertexVectorElements = descs;
		alloc.BytesPerVertex = 0;
		for (VectorDataFormat& format : descs)
		{
			VectorFormatDesc desc = GetFormatDesc(format);
			alloc.BytesVertexSizes.push_back((desc.RBits + desc.GBits + desc.BBits + desc.ABits) / 8);
			alloc.BytesPerVertex += alloc.BytesVertexSizes.back();
		}
		alloc.MemoryAllocator = new GPUMemoryAllocator(m_minVertexCount * alloc.BytesPerVertex, 8192, true);

		m_vertexTypeMutex.unlock();
		return id;
	}
	m_vertexTypeMutex.unlock_shared();
	return iter->second;
}

VertexMemoryPointer MeshAssetLoader::AllocateVertices(UniqueId vertexId, int count)
{
	MeshVertexAllocationPoint& alloc = m_vertexTypes[vertexId];
	return alloc.MemoryAllocator->RequestSpace(count * alloc.BytesPerVertex);
}

std::shared_ptr<GraphicsSyncObject> MeshAssetLoader::UploadVertices(VertexMemoryPointer ptr, UniqueId vertexId, int offset, void *data, int count)
{
	MeshVertexAllocationPoint& alloc = m_vertexTypes[vertexId];

	GraphicsCommandBuffer *cmdBuf = m_context->GetTransferBufferFromPool();

	cmdBuf->BeginRecording();
	PinnedGPUMemory mem = alloc.MemoryAllocator->GetMemory(ptr);
	std::shared_ptr<GraphicsSyncObject> up = m_uploadQueue.Upload<char>(cmdBuf, reinterpret_cast<char *>(data), mem, offset, 0, count * alloc.BytesPerVertex);
	cmdBuf->StopRecording();
	m_context->SubmitCommands(cmdBuf, GraphicsQueueType::Transfer);

	return up;
}

void MeshAssetLoader::DeallocateVertices(VertexMemoryPointer ptr, UniqueId vertexId)
{
	MeshVertexAllocationPoint& alloc = m_vertexTypes[vertexId];
	alloc.MemoryAllocator->FreeSpace(ptr);
}

IndexMemoryPointer MeshAssetLoader::AllocateIndices(int count)
{
	return m_indexAllocator.RequestSpace(count * sizeof(int));
}

std::shared_ptr<GraphicsSyncObject> MeshAssetLoader::UploadIndices(IndexMemoryPointer ptr, int offset, int *data, int count)
{
	PinnedGPUMemory mem = m_indexAllocator.GetMemory(ptr);

	GraphicsCommandBuffer *cmdBuf = m_context->GetTransferBufferFromPool();

	cmdBuf->BeginRecording();
	std::shared_ptr<GraphicsSyncObject> up = m_uploadQueue.Upload<int>(cmdBuf, data, mem, offset, 0, count);
	cmdBuf->StopRecording();
	m_context->SubmitCommands(cmdBuf, GraphicsQueueType::Transfer);

	return up;
}

void MeshAssetLoader::DeallocateIndices(IndexMemoryPointer ptr)
{
	m_indexAllocator.FreeSpace(ptr);
}

MeshVertexAllocationPoint& MeshAssetLoader::GetMeshMemory(UniqueId vertexId)
{
	return m_vertexTypes[vertexId];
}

GPUMemoryAllocator& MeshAssetLoader::GetIndexBuffer()
{
	return m_indexAllocator;
}

void MeshAssetLoader::SetVertexInputState(GraphicsRenderVertexInputState& state, UniqueId vertexId)
{
	MeshVertexAllocationPoint& alloc = m_vertexTypes[vertexId];
	state.AttributeData.resize(alloc.VertexVectorElements.size());
	state.BufferData.resize(1);
	state.PrimitiveRestart = false;
	state.Topology = GraphicsPrimitiveType::Triangles;

	GraphicsRenderVertexBufferInputData& bData = state.BufferData.back();
	bData.Stride = alloc.BytesPerVertex;
	bData.BufferBinding = 0;
	bData.Instanced = false;

	int i = 0;
	int offset = 0;
	for (VectorDataFormat format : alloc.VertexVectorElements)
	{
		GraphicsRenderVertexAttributeData& aData = state.AttributeData[i];

		aData.BufferBinding = 0;
		aData.ShaderAttributeLocation = i;
		aData.Format = format;
		aData.InElementOffset = offset;

		offset += alloc.BytesVertexSizes[i];
		++i;
	}
}

void MeshAsset::Invalidate(bool reduced)
{
	MeshVertexAllocationPoint& alloc = m_loader->GetMeshMemory(m_vertexTypeId);
	LocalMemoryAllocator& assetAlloc = XEngineInstance->GetAssetManager()->GetAssetMemory();

	if (reduced)
	{
		if (m_uploadReducedVerticesSync)
			m_uploadReducedVerticesSync->Wait(1e12);
		if (m_uploadReducedIndicesSync)
			m_uploadReducedIndicesSync->Wait(1e12);

		if (m_reducedLodElementsCPU)
			assetAlloc.FreeSpace(m_reducedLodElementsCPU);
		if (m_reducedLodIndicesCPU)
			assetAlloc.FreeSpace(m_reducedLodIndicesCPU);
		if (m_reducedLodElementsGPU)
			alloc.MemoryAllocator->FreeSpace(m_reducedLodElementsGPU);
		if (m_reducedLodIndicesGPU)
			alloc.MemoryAllocator->FreeSpace(m_reducedLodIndicesGPU);

		m_uploadReducedVerticesSync = m_uploadReducedIndicesSync = nullptr;
		m_reducedLodElementsCPU = m_reducedLodIndicesCPU = nullptr;
		m_reducedLodElementsGPU = m_reducedLodIndicesGPU = nullptr;
	}
	else
	{
		if (m_uploadFullVerticesSync)
			m_uploadFullVerticesSync->Wait(1e12);
		if (m_uploadFullIndicesSync)
			m_uploadFullIndicesSync->Wait(1e12);

		while (m_loadingState == MeshAssetLoadingState::LoadingFull);
		m_loadingState = MeshAssetLoadingState::NotLoaded;

		if (m_fullLodElementsCPU)
			assetAlloc.FreeSpace(m_fullLodElementsCPU);
		if (m_fullLodIndicesCPU)
			assetAlloc.FreeSpace(m_fullLodIndicesCPU);
		if (m_fullLodElementsGPU)
			alloc.MemoryAllocator->FreeSpace(m_fullLodElementsGPU);
		if (m_fullLodIndicesGPU)
			alloc.MemoryAllocator->FreeSpace(m_fullLodIndicesGPU);

		m_uploadFullVerticesSync = m_uploadFullIndicesSync = nullptr;
		m_fullLodElementsCPU = m_fullLodIndicesCPU = nullptr;
		m_fullLodElementsGPU = m_fullLodIndicesGPU = nullptr;
	}
}

void MeshAsset::OptimizeCacheAndBuildClusters()
{
}

std::vector<VectorDataFormat> renderV = 
{
	VectorDataFormat::R32G32B32A32Float,
	VectorDataFormat::R32G32B32A32Float,
	VectorDataFormat::R32G32B32A32Float,
	VectorDataFormat::R32G32B32A32Float
};

std::vector<VectorDataFormat> skinnedV = 
{
	VectorDataFormat::R32G32B32A32Float,
	VectorDataFormat::R32G32B32A32Float,
	VectorDataFormat::R32G32B32A32Float,
	VectorDataFormat::R32G32B32A32Float,
	VectorDataFormat::R32G32B32A32Float,
	VectorDataFormat::R32G32B32A32Int,
};

std::vector<VectorDataFormat>& GetRenderVertexData()
{
	return renderV;
}

std::vector<VectorDataFormat>& GetSkinnedVertexData()
{
	return skinnedV;
}
