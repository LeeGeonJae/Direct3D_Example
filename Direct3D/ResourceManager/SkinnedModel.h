#pragma once

#include <array>
#include <vector>
#include <string>

#include <d3d11.h>
#include <directxtk/SimpleMath.h>

#include "Animation.h"
#include "Subset.h"
#include "eMaterialTexture.h"
#include "LightHelper.h"
#include "Vertex.h"

namespace resourceManager
{
	struct SkinnedNode
	{
		enum { INVALID_INDEX = -1 };

		size_t CurrentIndex;
		size_t ParentIndex; // 중위 순회 결과 인덱스
		std::string Name;
		DirectX::SimpleMath::Matrix ToParentMatrix = DirectX::SimpleMath::Matrix::Identity; // 부모 공간까지
		DirectX::SimpleMath::Matrix ToRootMatrix = DirectX::SimpleMath::Matrix::Identity; // 루트 공간까지
		std::vector<size_t> SubsetIndex;
	};

	struct SkinnedBone
	{
		std::string Name;
		size_t NodeIndex; // 중위 순회 결과 인덱스
		DirectX::SimpleMath::Matrix OffsetMatrix;
	};

	struct SkinnedSubset
	{
		SkinnedSubset() :
			Id(-1),
			VertexStart(0), VertexCount(0),
			FaceStart(0), FaceCount(0)
		{
		}

		unsigned int Id;
		unsigned int VertexStart;
		unsigned int VertexCount;
		unsigned int FaceStart;
		unsigned int FaceCount;
		std::vector<SkinnedBone> Bones;
	};

	// 애니메이션 모델이라고 가정하는 게 나을듯
	// 그러면 노드 애니메이션이든 스키닝 애니메이션이든 일단 처리할 수는 있으니까
	// 노드에 포함된 매쉬 개념을 유지시켜줘야 되네 다시
	// 노드를 업데이트 시킨 후 매쉬 순회하면서 렌더링하도록 수정할 수는 있을 듯
	class SkinnedModel
	{
	public:
		SkinnedModel(ID3D11Device* d3dDevice, const std::string& fileName);
		~SkinnedModel();

		// 올바르게 렌더링하려면 노드 계층구조 업데이트
		// 노드 순회하면서 서브셋 테이블로 메쉬 렌더링
		// 메쉬 렌더링할 때 본은 요청된 시간에 맞춰 행렬을 평가한 후 업데이트 해서 처리한다.
		void Draw(ID3D11DeviceContext* d3dContext, const std::string& clipName, float timePos);

	public:
		enum { MAX_BONE_COUNT = 128 };

		// node
		std::vector<SkinnedNode> NodeInorderTraversal; // 중위 순회 순으로 저장됨

		// material
		std::vector<common::Material> Materials;
		std::array<std::vector<ID3D11ShaderResourceView*>, static_cast<size_t>(eMaterialTexture::Size)> SRVs;
		ID3D11Buffer* MaterialCB;

		// mesh
		std::vector<vertex::PosNormalTexTanSkinned> Vertices;
		std::vector<UINT> Indices;
		ID3D11Buffer* VB;
		ID3D11Buffer* IB;
		DXGI_FORMAT IndexBufferFormat; // 항상 32비트 가정함 
		UINT VertexStride;
		std::vector<SkinnedSubset> SubsetTable;
		ID3D11Buffer* BoneCB;

		// scene data
		DirectX::BoundingBox BoundingBox;
		DirectX::BoundingSphere BoundingSphere;

		// animation 
		std::map<std::string, AnimationClip> Animations;
	};

	struct SkinnedModelInstance
	{
		SkinnedModel* SkinnedModel;
		DirectX::SimpleMath::Matrix WorldMatrix;
		float TimePos;
		std::string AnimationName;
	};
}