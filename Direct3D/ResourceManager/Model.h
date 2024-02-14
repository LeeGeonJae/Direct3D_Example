#pragma once

#include <array>
#include <string>
#include <vector>
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <DirectXCollision.h>

#include "LightHelper.h"
#include "Vertex.h"
#include "Subset.h"
#include "eMaterialTexture.h"

namespace resourceManager
{
	class Model
	{
	public:
		Model(ID3D11Device* d3dDevice, const std::string& fileName);
		~Model();

		void Draw(ID3D11DeviceContext* d3dContext);

	public:
		// material
		std::vector<common::Material> Materials;
		std::array<std::vector<ID3D11ShaderResourceView*>, static_cast<size_t>(eMaterialTexture::Size)> SRVs;
		ID3D11Buffer* CB;

		// mesh
		std::vector<vertex::PosNormalTexTan> Vertices;
		std::vector<UINT> Indices;
		ID3D11Buffer* VB;
		ID3D11Buffer* IB;
		DXGI_FORMAT IndexBufferFormat; // 항상 32비트 가정함 
		UINT VertexStride;
		std::vector<Subset> SubsetTable;

		// scene data
		DirectX::BoundingBox BoundingBox;
		DirectX::BoundingSphere BoundingSphere;
	};

	struct ModelInstance
	{
		Model* Model;
		DirectX::SimpleMath::Matrix WorldMatrix;
	};
}