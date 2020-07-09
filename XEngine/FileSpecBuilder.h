#pragma once

#include "AssetManager.h"

class FileSpecEntry
{
public:
	int ElementTypeByteSize;
	bool String;
	bool Array;

	int FileOffset;
	int Size;
};

class FileSpec
{
public:
	template<class T>
	void AddArray(std::string name)
	{
		FileSpecEntry& e = m_fileEntries[name];
		e.ElementTypeByteSize = sizeof(T);
		e.Size = 0;
		e.FileOffset = 0;
		e.String = false;
		e.Array = true;
	}
	void AddString(std::string name)
	{
		FileSpecEntry& e = m_fileEntries[name];
		e.ElementTypeByteSize = sizeof(char);
		e.Size = 0;
		e.FileOffset = 0;
		e.String = true;
		e.Array = false;
	}
	template<class T>
	void Add(std::string name)
	{
		FileSpecEntry& e = m_fileEntries[name];
		e.ElementTypeByteSize = sizeof(T);
		e.Size = 0;
		e.FileOffset = 0;
		e.String = false;
		e.Array = false;
	}
	std::map<std::string, FileSpecEntry>& GetEntries()
	{
		return m_fileEntries;
	}
private:
	std::map<std::string, FileSpecEntry> m_fileEntries;
};

class FileSpecExporter
{
public:
	FileSpecExporter(FileSpec& spec) : m_fileEntries(spec.GetEntries()), m_totalSize(0)
	{
	}
	void SetArraySize(std::string name, int size)
	{
		m_fileEntries[name].Size = size;
	}
	void SetStringSize(std::string name, std::string& str)
	{
		m_fileEntries[name].Size = str.length() + 1;
	}
	template<class T>
	void SetArrayData(std::string name, T *data)
	{
		FileSpecEntry& entry = m_fileEntries[name];
		std::memcpy(m_space.GetData() + entry.FileOffset, data, sizeof(T) * entry.Size);
	}
	void SetString(std::string name, std::string& str)
	{
		FileSpecEntry& entry = m_fileEntries[name];
		std::memcpy(m_space.GetData() + entry.FileOffset, str.c_str(), sizeof(char) * entry.Size);
	}
	template<class T>
	void Set(std::string name, T& data)
	{
		FileSpecEntry& entry = m_fileEntries[name];
		std::memcpy(m_space.GetData() + entry.FileOffset, &data, sizeof(T));
	}

	int GetTotalByteSize() { return m_totalSize; }
	LoadMemoryPointer AllocateSpace();
private:
	int m_totalSize;
	LoadMemoryPointer m_alloc;
	PinnedLocalMemory<char> m_space;
	std::map<std::string, FileSpecEntry> m_fileEntries;
};

class FileSpecImporter
{
public:
	FileSpecImporter(FileSpec& spec, LoadMemoryPointer data);

	template<class T>
	T *GetArray(std::string name)
	{
		return reinterpret_cast<T *>(m_space.GetData() + m_fileEntries[name].FileOffset);
	}

	template<class T>
	void CopyArray(std::string name, T *dest)
	{
		std::memcpy(dest, m_space.GetData() + m_fileEntries[name].FileOffset, m_fileEntries[name].Size * m_fileEntries[name].ElementTypeByteSize);
	}

	template<class T>
	void CopyArray(std::string name, std::vector<T>& dest)
	{
		dest.resize(GetArraySize(name));
		CopyArray(name, dest.data());
	}
	int GetArraySize(std::string name);

	std::string GetString(std::string name);

	template<class T>
	T& Get(std::string name)
	{
		return *reinterpret_cast<T *>(m_space.GetData() + m_fileEntries[name].FileOffset);
	}
private:
	int m_totalSize;
	LoadMemoryPointer m_alloc;
	PinnedLocalMemory<char> m_space;
	std::map<std::string, FileSpecEntry> m_fileEntries;
};