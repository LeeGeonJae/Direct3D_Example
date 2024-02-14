#include "Model.h"

#include <functional>
#include <cassert>
#include <filesystem>
#include <string>
#include <set>
#include <directxtk/WICTextureLoader.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "d3dUtil.h"
#include "ResourceManager.h"

namespace resourceManager
{
	DirectX::SimpleMath::Matrix convertMatrix(const aiMatrix4x4& aiMatrix)
	{
		DirectX::SimpleMath::Matrix result;

		for (int i = 0; i < 4; ++i)
		{
			result.m[i][0] = aiMatrix[i][0];
			result.m[i][1] = aiMatrix[i][1];
			result.m[i][2] = aiMatrix[i][2];
			result.m[i][3] = aiMatrix[i][3];
		}

		return result;
	}

	Model::Model(ID3D11Device* d3dDevice, const std::string& fileName)
		: IndexBufferFormat(DXGI_FORMAT_R32_UINT)
		, VertexStride(sizeof(vertex::PosNormalTexTan))
	{
		using namespace DirectX::SimpleMath;

		Assimp::Importer importer;
		importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0);    // $assimp_fbx$ 드 생.성안함
		unsigned int importFlags = aiProcess_Triangulate |
			aiProcess_GenNormals |
			aiProcess_GenUVCoords |
			aiProcess_CalcTangentSpace |
			aiProcess_LimitBoneWeights |
			aiProcess_ConvertToLeftHanded;

		const aiScene* scene = importer.ReadFile(fileName, importFlags);
		UINT id = 0;

		Vertices.reserve(1024);
		Indices.reserve(1024);

		std::function<void(aiNode*, Matrix)> nodeRecursive = [this, scene, &nodeRecursive, &id](aiNode* node, Matrix parentToWorldMatrix)
			{
				Matrix toParentMatrix = convertMatrix(node->mTransformation).Transpose();
				Matrix toWorld = toParentMatrix * parentToWorldMatrix;

				for (UINT i = 0; i < node->mNumMeshes; ++i)
				{
					unsigned int meshIndex = node->mMeshes[i];
					aiMesh* mesh = scene->mMeshes[meshIndex];

					Subset subset;
					subset.Id = id++;
					subset.VertexCount = mesh->mNumVertices;
					subset.FaceCount = mesh->mNumFaces;

					if (subset.Id > 0)
					{
						const Subset& prev = SubsetTable[subset.Id - 1];
						subset.VertexStart = prev.VertexStart + prev.VertexCount;
						subset.FaceStart = prev.FaceStart + prev.FaceCount;
					}

					SubsetTable.push_back(subset);

					for (UINT j = 0; j < mesh->mNumVertices; ++j)
					{
						vertex::PosNormalTexTan vertex;

						if (mesh->HasPositions())
						{
							vertex.Pos.x = mesh->mVertices[j].x;
							vertex.Pos.y = mesh->mVertices[j].y;
							vertex.Pos.z = mesh->mVertices[j].z;

							vertex.Pos = Vector3::Transform(vertex.Pos, toWorld);
						}

						if (mesh->HasNormals())
						{
							vertex.Normal.x = mesh->mNormals[j].x;
							vertex.Normal.y = mesh->mNormals[j].y;
							vertex.Normal.z = mesh->mNormals[j].z;

							vertex.Normal = Vector3::TransformNormal(vertex.Normal, toWorld);
						}

						if (mesh->HasTangentsAndBitangents())
						{
							vertex.TangentU.x = mesh->mTangents[j].x;
							vertex.TangentU.y = mesh->mTangents[j].y;
							vertex.TangentU.z = mesh->mTangents[j].z;

							vertex.TangentU = Vector3::TransformNormal(vertex.TangentU, toWorld);
						}

						if (mesh->HasTextureCoords(0))
						{
							vertex.Tex.x = (float)mesh->mTextureCoords[0][j].x;
							vertex.Tex.y = (float)mesh->mTextureCoords[0][j].y;
						}

						Vertices.push_back(vertex);
					}

					for (UINT j = 0; j < mesh->mNumFaces; ++j) {
						aiFace face = mesh->mFaces[j];

						for (UINT k = 0; k < face.mNumIndices; ++k)
						{
							Indices.push_back(face.mIndices[k]);
						}
					}

					aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

					aiString texturePath;
					std::filesystem::path basePath = std::filesystem::current_path() / "textures";

					std::vector<bool> hasTexture(21);
					for (int i = 0; i < 21; ++i)
					{
						if (material->GetTexture((aiTextureType)i, 0, &texturePath) == AI_SUCCESS)
						{
							hasTexture[i] = true;
						}
					}

					if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS)
					{
						std::filesystem::path filePath = texturePath.C_Str();

						auto curPath = basePath / filePath.filename();
						ID3D11ShaderResourceView* srv = ResourceManager::GetInstance()->LoadTexture(curPath.c_str());

						SRVs[static_cast<size_t>(eMaterialTexture::Diffuse)].push_back(srv);
					}
					else
					{
						SRVs[static_cast<size_t>(eMaterialTexture::Diffuse)].push_back(nullptr);
					}
					if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS)
					{
						std::filesystem::path filePath = texturePath.C_Str();

						auto curPath = basePath / filePath.filename();
						ID3D11ShaderResourceView* srv = ResourceManager::GetInstance()->LoadTexture(curPath.c_str());

						SRVs[static_cast<size_t>(eMaterialTexture::Normal)].push_back(srv);
					}
					else
					{
						SRVs[static_cast<size_t>(eMaterialTexture::Normal)].push_back(nullptr);
					}
					if (material->GetTexture(aiTextureType_SPECULAR, 0, &texturePath) == AI_SUCCESS)
					{
						std::filesystem::path filePath = texturePath.C_Str();

						auto curPath = basePath / filePath.filename();
						ID3D11ShaderResourceView* srv = ResourceManager::GetInstance()->LoadTexture(curPath.c_str());

						SRVs[static_cast<size_t>(eMaterialTexture::Specular)].push_back(srv);
					}
					else
					{
						SRVs[static_cast<size_t>(eMaterialTexture::Specular)].push_back(nullptr);

					}
					if (material->GetTexture(aiTextureType_OPACITY, 0, &texturePath) == AI_SUCCESS)
					{
						std::filesystem::path filePath = texturePath.C_Str();

						auto curPath = basePath / filePath.filename();
						ID3D11ShaderResourceView* srv = ResourceManager::GetInstance()->LoadTexture(curPath.c_str());

						SRVs[static_cast<size_t>(eMaterialTexture::Opacity)].push_back(srv);
					}
					else
					{
						SRVs[static_cast<size_t>(eMaterialTexture::Opacity)].push_back(nullptr);
					}
				}

				for (UINT i = 0; i < node->mNumChildren; ++i)
				{
					nodeRecursive(node->mChildren[i], toWorld);
				}
			};
		nodeRecursive(scene->mRootNode, Matrix::Identity);

		importer.FreeScene();

		HRESULT hr;

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = static_cast<size_t>(sizeof(vertex::PosNormalTexTan) * Vertices.size());
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = &Vertices[0];

		hr = d3dDevice->CreateBuffer(&vbd, &initData, &VB);
		if (FAILED(hr)) {
			assert(false);
		}

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = static_cast<size_t>(sizeof(unsigned int) * Indices.size());
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;

		initData.pSysMem = &Indices[0];

		hr = d3dDevice->CreateBuffer(&ibd, &initData, &IB);
		if (FAILED(hr)) {
			assert(false);
		}

		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.ByteWidth = sizeof(int) * static_cast<size_t>(eMaterialTexture::Size);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;

		hr = d3dDevice->CreateBuffer(&cbd, NULL, &CB);
		if (FAILED(hr)) {
			assert(false);
		}
	}

	Model::~Model()
	{
		ReleaseCOM(VB);
		ReleaseCOM(IB);
		ReleaseCOM(CB);
	}

	void Model::Draw(ID3D11DeviceContext* d3dContext)
	{
		UINT offset = 0;
		d3dContext->IASetIndexBuffer(IB, IndexBufferFormat, 0);
		d3dContext->IASetVertexBuffers(0, 1, &VB, &VertexStride, &offset);
		d3dContext->PSSetConstantBuffers(1, 1, &CB);

		for (size_t i = 0; i < SubsetTable.size(); ++i)
		{
			ID3D11ShaderResourceView* srv[static_cast<size_t>(eMaterialTexture::Size)] = { nullptr };
			int bUseTextures[static_cast<size_t>(eMaterialTexture::Size)] = { false, };

			for (size_t j = 0; j < static_cast<size_t>(eMaterialTexture::Size); ++j)
			{
				srv[j] = SRVs[j][i];
				bUseTextures[j] = srv[j] != nullptr;
			}

			d3dContext->UpdateSubresource(CB, 0, 0, bUseTextures, 0, 0);
			d3dContext->PSSetShaderResources(0, 4, srv);
			d3dContext->DrawIndexed(SubsetTable[i].FaceCount * 3, SubsetTable[i].FaceStart * 3, SubsetTable[i].VertexStart);
		}
	}
}