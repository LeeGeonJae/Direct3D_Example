#include "SkinnedModel.h"

#include <functional>
#include <cassert>
#include <filesystem>
#include <string>
#include <array>
#include <set>
#include <directxtk/WICTextureLoader.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "d3dUtil.h"
#include "ResourceManager.h"
#include "MathHelper.h"

namespace resourceManager
{
	extern DirectX::SimpleMath::Matrix convertMatrix(const aiMatrix4x4& aiMatrix);

	SkinnedModel::SkinnedModel(ID3D11Device* d3dDevice, const std::string& fileName)
		: IndexBufferFormat(DXGI_FORMAT_R32_UINT)
		, VertexStride(sizeof(vertex::PosNormalTexTanSkinned))
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

		NodeInorderTraversal.reserve(128);

		std::function<void(aiNode*, int)> nodeRecursive = [this, scene, &nodeRecursive, &id](aiNode* node, size_t parentIndex)
			{
				// 노드 계층 구조 저장
				SkinnedNode skinnedNode;
				skinnedNode.ToParentMatrix = convertMatrix(node->mTransformation).Transpose();
				skinnedNode.Name = node->mName.C_Str();
				skinnedNode.ParentIndex = parentIndex;
				skinnedNode.CurrentIndex = NodeInorderTraversal.size();
				NodeInorderTraversal.push_back(skinnedNode);

				// 메쉬(정점 데이터) 저장
				for (UINT i = 0; i < node->mNumMeshes; ++i)
				{
					unsigned int meshIndex = node->mMeshes[i];
					aiMesh* mesh = scene->mMeshes[meshIndex];

					SkinnedSubset subset;
					subset.Id = id++;
					subset.VertexCount = mesh->mNumVertices;
					subset.FaceCount = mesh->mNumFaces;

					skinnedNode.SubsetIndex.push_back(subset.Id);

					if (subset.Id > 0)
					{
						const SkinnedSubset& prev = SubsetTable[subset.Id - 1];
						subset.VertexStart = prev.VertexStart + prev.VertexCount;
						subset.FaceStart = prev.FaceStart + prev.FaceCount;
					}

					for (UINT j = 0; j < mesh->mNumVertices; ++j)
					{
						vertex::PosNormalTexTanSkinned vertex;

						if (mesh->HasPositions())
						{
							vertex.Pos.x = mesh->mVertices[j].x;
							vertex.Pos.y = mesh->mVertices[j].y;
							vertex.Pos.z = mesh->mVertices[j].z;
						}

						if (mesh->HasNormals())
						{
							vertex.Normal.x = mesh->mNormals[j].x;
							vertex.Normal.y = mesh->mNormals[j].y;
							vertex.Normal.z = mesh->mNormals[j].z;
						}

						if (mesh->HasTangentsAndBitangents())
						{
							vertex.TangentU.x = mesh->mTangents[j].x;
							vertex.TangentU.y = mesh->mTangents[j].y;
							vertex.TangentU.z = mesh->mTangents[j].z;
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

					// 메쉬에 관여한 본 저장
					subset.Bones.reserve(mesh->mNumBones);
					for (UINT j = 0; j < mesh->mNumBones; ++j)
					{
						SkinnedBone skinnedBone;
						skinnedBone.Name = mesh->mBones[j]->mName.C_Str();
						skinnedBone.OffsetMatrix = convertMatrix(mesh->mBones[j]->mOffsetMatrix).Transpose();

						subset.Bones.push_back(skinnedBone);

						for (UINT k = 0; k < mesh->mBones[j]->mNumWeights; ++k)
						{
							unsigned int vertexIndex = mesh->mBones[j]->mWeights[k].mVertexId + subset.VertexStart;
							float vertexWeight = mesh->mBones[j]->mWeights[k].mWeight;

							for (UINT l = 0; l < 4; ++l)
							{
								if (Vertices[vertexIndex].Indices[l] == vertex::PosNormalTexTanSkinned::INVALID_INDEX)
								{
									Vertices[vertexIndex].Indices[l] = j;
									Vertices[vertexIndex].Weights[l] = vertexWeight;
									break;
								}
							}
						}
					}

					SubsetTable.push_back(subset);

					// 재질(겉면, 텍스처) 저장
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
					nodeRecursive(node->mChildren[i], skinnedNode.CurrentIndex);
				}
			};

		nodeRecursive(scene->mRootNode, (size_t)SkinnedNode::INVALID_INDEX);

		// 본에 노드 인덱스 연결
		std::map<std::string, size_t> nodeNameIndexMap;

		for (const auto& skinnedNode : NodeInorderTraversal)
		{
			nodeNameIndexMap.insert({ skinnedNode.Name, skinnedNode.CurrentIndex });
		}

		for (auto& subset : SubsetTable)
		{
			for (auto& skinnedBone : subset.Bones)
			{
				auto find = nodeNameIndexMap.find(skinnedBone.Name);
				assert(find != nodeNameIndexMap.end());

				skinnedBone.NodeIndex = find->second;
			}
		}

		// 애니메이션 로드
		for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
		{
			auto& currentAnimation = scene->mAnimations[i];

			AnimationClip animClip;
			auto totalFrame = currentAnimation->mDuration;
			auto framePerSeconds = 1 / (currentAnimation->mTicksPerSecond);
			animClip.Duration = totalFrame * framePerSeconds;
			animClip.Name = currentAnimation->mName.C_Str();

			for (unsigned int j = 0; j < currentAnimation->mNumChannels; ++j)
			{
				auto& currentChennel = currentAnimation->mChannels[j];

				AnimationNode animationNode;
				animationNode.Name = currentChennel->mNodeName.C_Str();

				for (unsigned int k = 0; k < currentChennel->mNumPositionKeys; ++k)
				{
					KeyAnimation keyAnimation;

					keyAnimation.Time = currentChennel->mPositionKeys[k].mTime * framePerSeconds;

					keyAnimation.Position.x = currentChennel->mPositionKeys[k].mValue.x;
					keyAnimation.Position.y = currentChennel->mPositionKeys[k].mValue.y;
					keyAnimation.Position.z = currentChennel->mPositionKeys[k].mValue.z;

					keyAnimation.Rotation.x = currentChennel->mRotationKeys[k].mValue.x;
					keyAnimation.Rotation.y = currentChennel->mRotationKeys[k].mValue.y;
					keyAnimation.Rotation.z = currentChennel->mRotationKeys[k].mValue.z;
					keyAnimation.Rotation.w = currentChennel->mRotationKeys[k].mValue.w;

					keyAnimation.Scaling.x = currentChennel->mScalingKeys[k].mValue.x;
					keyAnimation.Scaling.y = currentChennel->mScalingKeys[k].mValue.y;
					keyAnimation.Scaling.z = currentChennel->mScalingKeys[k].mValue.z;

					animationNode.KeyAnimations.push_back(keyAnimation);
				}

				animClip.AnimationNodes.insert({ animationNode.Name, animationNode });
			}

			Animations.insert({ animClip.Name, animClip });
		}

		importer.FreeScene();

		HRESULT hr;

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = static_cast<size_t>(sizeof(vertex::PosNormalTexTanSkinned) * Vertices.size());
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

		hr = d3dDevice->CreateBuffer(&cbd, NULL, &MaterialCB);
		if (FAILED(hr)) {
			assert(false);
		}

		cbd = {};
		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.ByteWidth = sizeof(Matrix) * MAX_BONE_COUNT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;

		hr = d3dDevice->CreateBuffer(&cbd, NULL, &BoneCB);
		if (FAILED(hr)) {
			assert(false);
		}
	}

	SkinnedModel::~SkinnedModel()
	{
		ReleaseCOM(VB);
		ReleaseCOM(IB);
		ReleaseCOM(BoneCB);
		ReleaseCOM(MaterialCB);
	}

	void SkinnedModel::Draw(ID3D11DeviceContext* d3dContext, const std::string& clipName, float timePos)
	{
		using namespace DirectX::SimpleMath;
		// 바인딩
		UINT offset = 0;
		d3dContext->IASetIndexBuffer(IB, IndexBufferFormat, 0);
		d3dContext->IASetVertexBuffers(0, 1, &VB, &VertexStride, &offset);
		d3dContext->VSSetConstantBuffers(1, 1, &BoneCB);
		d3dContext->PSSetConstantBuffers(1, 1, &MaterialCB);

		auto findedAnim = Animations.find(clipName);
		assert(findedAnim != Animations.end());

		const AnimationClip& animClip = findedAnim->second;

		for (size_t i = 0; i < SubsetTable.size(); ++i)
		{
			// 본으로 로컬 매트릭스 갱신
			for (const SkinnedBone& bone : SubsetTable[i].Bones)
			{
				auto find = animClip.AnimationNodes.find(bone.Name);

				if (find != animClip.AnimationNodes.end())
				{
					const AnimationNode& animNode = find->second;

					NodeInorderTraversal[bone.NodeIndex].ToParentMatrix = animNode.Evaluate(fmod(timePos, animClip.Duration));
				}
			}

			// 노드 월드 매트릭스 갱신
			NodeInorderTraversal[0].ToRootMatrix = NodeInorderTraversal[0].ToParentMatrix;
			for (size_t j = 1; j < NodeInorderTraversal.size(); ++j)
			{
				SkinnedNode& curNode = NodeInorderTraversal[j];
				const SkinnedNode& parentNode = NodeInorderTraversal[curNode.ParentIndex];

				curNode.ToRootMatrix = curNode.ToParentMatrix * parentNode.ToRootMatrix;
			}

			// 본 팔레트 행렬 만들기, 상수 버퍼 반영

			// 추후에 매번 벡터를 동적할당하는 
			std::array<Matrix, MAX_BONE_COUNT> matrixPalette;
			auto paletteIter = matrixPalette.begin();

			for (const SkinnedBone& bone : SubsetTable[i].Bones)
			{
				const Matrix& boneToRoot = NodeInorderTraversal[bone.NodeIndex].ToRootMatrix;

				// 오프셋 매트릭스는 정점을 뼈대 공간에서 다룰 수 있게 하고,
				// 본의 로컬(애니메이션) + 본 부모의 루트(월드에 재배치)하는 흐름을 갖는다.
				*paletteIter++ = (bone.OffsetMatrix * boneToRoot).Transpose();
			}

			if (!SubsetTable[i].Bones.empty())
			{
				d3dContext->UpdateSubresource(BoneCB, 0, 0, &matrixPalette[0], 0, 0);
			}

			// 텍스처 적용 그리기 호출
			constexpr const size_t TEXTURE_SIZE = static_cast<size_t>(eMaterialTexture::Size);
			ID3D11ShaderResourceView* srv[TEXTURE_SIZE];
			int bUseTextures[TEXTURE_SIZE];

			for (size_t j = 0; j < static_cast<size_t>(eMaterialTexture::Size); ++j)
			{
				srv[j] = SRVs[j][i];
				bUseTextures[j] = srv[j] != nullptr;
			}

			d3dContext->UpdateSubresource(MaterialCB, 0, 0, &bUseTextures[0], 0, 0);
			d3dContext->PSSetShaderResources(0, TEXTURE_SIZE, &srv[0]);
			d3dContext->DrawIndexed(SubsetTable[i].FaceCount * 3, SubsetTable[i].FaceStart * 3, SubsetTable[i].VertexStart);
		}
	}
}