#include "pch.h"
#include "FileSpecBuilder.h"

LoadMemoryPointer FileSpecExporter::AllocateSpace()
{
	m_totalSize = 0;

	for (auto& pair : m_fileEntries)
	{
		if (pair.second.Array)
		{
			m_totalSize += 4;
			pair.second.FileOffset = m_totalSize;
			m_totalSize += pair.second.Size * pair.second.ElementTypeByteSize;
		}
		else if (pair.second.String)
		{
			m_totalSize += 4;
			pair.second.FileOffset = m_totalSize;
			m_totalSize += pair.second.Size * sizeof(char);
		}
		else
		{
			pair.second.FileOffset = m_totalSize;
			m_totalSize += pair.second.ElementTypeByteSize;
		}
	}

	m_alloc = XEngineInstance->GetAssetManager()->GetLoadMemory().RequestSpace(m_totalSize);
	m_space = XEngineInstance->GetAssetManager()->GetLoadMemory().GetMemory<char>(m_alloc);

	for (auto& pair : m_fileEntries)
	{
		if (pair.second.Array || pair.second.String)
		{
			int& size = *reinterpret_cast<int *>(m_space.GetData() + pair.second.FileOffset - 4);
			size = pair.second.Size;
		}
	}
	
	return m_alloc;
}

FileSpecImporter::FileSpecImporter(FileSpec& spec, LoadMemoryPointer data) : m_fileEntries(spec.GetEntries()),
	m_alloc(data)
{
	m_space = XEngineInstance->GetAssetManager()->GetLoadMemory().GetMemory<char>(data);

	unsigned long long pointer = 0;
	for (auto& pair : m_fileEntries)
	{
		if (pair.second.Array || pair.second.String)
		{
			pair.second.Size = *reinterpret_cast<int *>(m_space.GetData() + pointer);
			pointer += 4;
			pair.second.FileOffset = pointer;
			pointer += pair.second.ElementTypeByteSize * pair.second.Size;
		}
		else
		{
			pair.second.FileOffset = pointer;
			pointer += pair.second.ElementTypeByteSize;
		}
	}
}

int FileSpecImporter::GetArraySize(std::string name)
{
	return m_fileEntries[name].Size;
}

std::string FileSpecImporter::GetString(std::string name)
{
	return std::string(m_space.GetData() + m_fileEntries[name].FileOffset, m_fileEntries[name].Size - 1);
}
