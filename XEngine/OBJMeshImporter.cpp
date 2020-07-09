#include "pch.h"
#include "OBJMeshImporter.h"
#include "MeshAsset.h"

#include <sstream>
#include <fstream>
#include <iterator>

std::vector<std::string> exts = { "obj" };

std::vector<std::string>& OBJMeshImporter::GetFileExtensions()
{
	return exts;
}

std::vector<std::string> split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

void OBJMeshImporter::Import(std::string virtualPath, std::string filePath)
{
	MeshAsset *asset = static_cast<MeshAsset *>(XEngineInstance->GetAssetManager()->CreateAssetPtr(virtualPath, "Mesh"));

	asset->SetVertexFormat(GetRenderVertexData());
	asset->SetUnifiedLOD(true);
	asset->SetReducedMeshEnabled(false);

	std::vector<RenderVertex> vertices;
	std::vector<int> indices;

	std::ifstream stream(filePath.c_str());
	std::string line;

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	bool hasNormal = false;

	while (std::getline(stream, line))
	{
		if (line.empty())
			continue;

		std::vector<std::string> wSpaceSplit = split(line, ' ');

		if (wSpaceSplit[0] == "v")
		{
			float x = std::atof(wSpaceSplit[1].c_str());
			float y = std::atof(wSpaceSplit[2].c_str());
			float z = std::atof(wSpaceSplit[3].c_str());

			positions.push_back(glm::vec3(x, y, z));
		}
		else if (wSpaceSplit[0] == "vt")
		{
			float x = std::atof(wSpaceSplit[1].c_str());
			float y = std::atof(wSpaceSplit[2].c_str());

			uvs.push_back(glm::vec2(x, y));
		}
		else if (wSpaceSplit[0] == "vn")
		{
			float x = std::atof(wSpaceSplit[1].c_str());
			float y = std::atof(wSpaceSplit[2].c_str());
			float z = std::atof(wSpaceSplit[3].c_str());

			normals.push_back(glm::vec3(x, y, z));
		}
		else if (wSpaceSplit[0] == "f")
		{
			RenderVertex vert;

			vert.PositionColorR = glm::vec4(0, 0, 0, 0);
			vert.NormalColorG = glm::vec4(0, 0, 0, 0);
			vert.TangentColorB = glm::vec4(0, 0, 0, 0);
			vert.Texcoord0ColorA = glm::vec4(0, 0, 0, 1);

			int pIndex = 0;
			int tIndex = 0;
			int nIndex = 0;

			if (line.find('/') != std::string::npos)
			{
				std::vector<std::vector<std::string>> verts = {
					split(wSpaceSplit[1], '/'),
					split(wSpaceSplit[2], '/'),
					split(wSpaceSplit[3], '/')
				};

				for (std::vector<std::string>& v : verts)
				{
					if (v.size() == 3 && v[1] == "")
					{
						hasNormal = true;

						pIndex = std::atoi(v[0].c_str()) - 1;
						nIndex = std::atoi(v[2].c_str()) - 1;

						vert.PositionColorR = glm::vec4(positions[pIndex], 0);
						vert.NormalColorG = glm::vec4(normals[nIndex], 0);

						vertices.push_back(vert);
						indices.push_back(vertices.size() - 1);
					}
					else if (v.size() == 3)
					{
						hasNormal = true;

						pIndex = std::atoi(v[0].c_str()) - 1;
						tIndex = std::atoi(v[1].c_str()) - 1;
						nIndex = std::atoi(v[2].c_str()) - 1;

						vert.PositionColorR = glm::vec4(positions[pIndex], 0);
						vert.Texcoord0ColorA = glm::vec4(uvs[tIndex], 0, 1);
						vert.NormalColorG = glm::vec4(normals[nIndex], 0);

						vertices.push_back(vert);
						indices.push_back(vertices.size() - 1);
					}
					else if (v.size() == 2)
					{
						pIndex = std::atoi(v[0].c_str()) - 1;
						tIndex = std::atoi(v[1].c_str()) - 1;

						vert.PositionColorR = glm::vec4(positions[pIndex], 0);
						vert.Texcoord0ColorA = glm::vec4(uvs[tIndex], 0, 1);

						vertices.push_back(vert);
						indices.push_back(vertices.size() - 1);
					}
				}
			}
			else
			{
				vert.PositionColorR = glm::vec4(positions[std::atoi(wSpaceSplit[1].c_str()) - 1], 0);
				vertices.push_back(vert);
				indices.push_back(vertices.size() - 1);

				vert.PositionColorR = glm::vec4(positions[std::atoi(wSpaceSplit[2].c_str()) - 1], 0);
				vertices.push_back(vert);
				indices.push_back(vertices.size() - 1);

				vert.PositionColorR = glm::vec4(positions[std::atoi(wSpaceSplit[3].c_str()) - 1], 0);
				vertices.push_back(vert);
				indices.push_back(vertices.size() - 1);
			}
		}
	}

	stream.close();

	if (!hasNormal)
	{
		for (int i = 0; i < indices.size(); i += 3)
		{
			glm::vec3 pos0 = glm::vec3(vertices[indices[i]].PositionColorR);
			glm::vec3 pos1 = glm::vec3(vertices[indices[i + 1]].PositionColorR);
			glm::vec3 pos2 = glm::vec3(vertices[indices[i + 2]].PositionColorR);

			glm::vec3 face = glm::normalize(glm::cross(glm::vec3(pos2 - pos1), glm::vec3(pos1 - pos0)));

			vertices[indices[i]].NormalColorG += glm::vec4(face, 0);
			vertices[indices[i + 1]].NormalColorG += glm::vec4(face, 0);
			vertices[indices[i + 2]].NormalColorG += glm::vec4(face, 0);
		}

		for (RenderVertex& v : vertices)
		{
			v.NormalColorG = glm::vec4(glm::normalize(glm::vec3(v.NormalColorG)), v.NormalColorG.a);
		}
	}

	for (int i = 0; i < indices.size(); i += 3)
	{
		RenderVertex& v0 = vertices[indices[i]];
		RenderVertex& v1 = vertices[indices[i + 1]];
		RenderVertex& v2 = vertices[indices[i + 2]];

		glm::vec3 pos0 = glm::vec3(v0.PositionColorR);
		glm::vec3 pos1 = glm::vec3(v1.PositionColorR);
		glm::vec3 pos2 = glm::vec3(v2.PositionColorR);

		glm::vec2 uv0 = glm::vec2(v0.Texcoord0ColorA);
		glm::vec2 uv1 = glm::vec2(v1.Texcoord0ColorA);
		glm::vec2 uv2 = glm::vec2(v2.Texcoord0ColorA);

		glm::vec3 deltaPos1 = pos1 - pos0;
		glm::vec3 deltaPos2 = pos2 - pos0;

		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = glm::normalize((deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r);

		v0.TangentColorB = glm::vec4(tangent, v0.TangentColorB.a);
		v1.TangentColorB = glm::vec4(tangent, v1.TangentColorB.a);
		v2.TangentColorB = glm::vec4(tangent, v2.TangentColorB.a);
	}

	std::vector<ClusterData> dat;
	asset->SetMeshData(vertices.data(), vertices.size(), indices.data(), indices.size(), true);
	asset->SetClusterData(dat.data(), 0);

	asset->OptimizeCacheAndBuildClusters();
}
