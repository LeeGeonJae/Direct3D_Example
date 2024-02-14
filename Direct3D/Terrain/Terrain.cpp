#include <fstream>
#include <sstream>
#include <DirectXMath.h>
#include <algorithm>

#include "Terrain.h"
#include "Camera.h"
#include "LightHelper.h"
#include "Effects.h"

#include "MathHelper.h"
#include "D3DUtil.h"

namespace terrain
{
	Terrain::Terrain()
		: mQuadPatchVB(0)
		, mQuadPatchIB(0)
		, mLayerMapArraySRV(0)
		, mBlendMapSRV(0)
		, mHeightMapSRV(0)
		, mNumPatchVertices(0)
		, mNumPatchQuadFaces(0)
		, mNumPatchVertRows(0)
		, mNumPatchVertCols(0)
	{
		mWorld = Matrix::Identity;

		mMat.Ambient = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		mMat.Diffuse = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		mMat.Specular = Vector4(0.0f, 0.0f, 0.0f, 64.0f);
		mMat.Reflect = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	Terrain::~Terrain()
	{
		ReleaseCOM(mQuadPatchVB);
		ReleaseCOM(mQuadPatchIB);
		ReleaseCOM(mLayerMapArraySRV);
		ReleaseCOM(mBlendMapSRV);
		ReleaseCOM(mHeightMapSRV);
	}

	void Terrain::Init(ID3D11Device* device, ID3D11DeviceContext* dc, const InitInfo& initInfo)
	{
		mInfo = initInfo;

		// 정점은 셀보다 하나 많아야 올바르게 생성된다.
		mNumPatchVertRows = ((mInfo.HeightmapHeight - 1) / CellsPerPatch) + 1;
		mNumPatchVertCols = ((mInfo.HeightmapWidth - 1) / CellsPerPatch) + 1;
		mNumPatchVertices = mNumPatchVertRows * mNumPatchVertCols;

		// Cell은 정점보다 하나 적게 생성된다. 위랑 똑같은 말이긴 함
		mNumPatchQuadFaces = (mNumPatchVertRows - 1) * (mNumPatchVertCols - 1);

		buildTerrain(device);

		LoadHeightmap();
		Smooth();
		CalcAllPatchBoundsY();

		BuildQuadPatchVB(device);
		BuildQuadPatchIB(device);
		BuildHeightmapSRV(device);

		std::vector<std::wstring> layerFilenames;
		layerFilenames.push_back(mInfo.LayerMapFilename0);
		layerFilenames.push_back(mInfo.LayerMapFilename1);
		layerFilenames.push_back(mInfo.LayerMapFilename2);
		layerFilenames.push_back(mInfo.LayerMapFilename3);
		layerFilenames.push_back(mInfo.LayerMapFilename4);
		mLayerMapArraySRV = D3DHelper::CreateTexture2DArraySRV(device, dc, layerFilenames);

		HR(DirectX::CreateDDSTextureFromFile(device, mInfo.BlendMapFilename.c_str(), 0, &mBlendMapSRV));
	}

	void Terrain::Draw(ID3D11DeviceContext* dc, const common::Camera& cam, DirectionLight lights[3])
	{
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
		dc->IASetInputLayout(mTerrainIL);

		UINT stride = sizeof(VertexTerrain);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, &mQuadPatchVB, &stride, &offset);
		dc->IASetIndexBuffer(mQuadPatchIB, DXGI_FORMAT_R16_UINT, 0);

		Matrix viewProj = cam.GetViewProj();
		Matrix world = XMLoadFloat4x4(&mWorld);
		Matrix worldInvTranspose = MathHelper::InverseTranspose(world);
		Matrix worldViewProj = world * viewProj;

		Vector4 worldPlanes[6];
		D3DHelper::ExtractFrustumPlanes(worldPlanes, viewProj);

		// Set per frame constants.
		mPerObjectTerrain.ViewProj = viewProj.Transpose();
		mPerObjectTerrain.Material = mMat;

		mPerFrameTerrain.EyePosW = cam.GetPosition();
		memcpy(mPerFrameTerrain.DirLights, lights, sizeof(mPerFrameTerrain.DirLights));
		mPerFrameTerrain.FogColor = Silver;
		mPerFrameTerrain.FogRange = 500.f;
		mPerFrameTerrain.FogStart = 25.f;
		mPerFrameTerrain.MinDist = 20.0f;
		mPerFrameTerrain.MaxDist = 500.0f;
		mPerFrameTerrain.MinTess = 0.0f;
		mPerFrameTerrain.MaxTess = 6.0f;
		mPerFrameTerrain.TexelCellSpaceU = 1.0f / mInfo.HeightmapWidth;
		mPerFrameTerrain.TexelCellSpaceV = 1.0f / mInfo.HeightmapHeight;
		mPerFrameTerrain.WorldCellSpace = mInfo.CellSpacing;
		memcpy(mPerFrameTerrain.WorldFrustumPlanes, worldPlanes, sizeof(mPerFrameTerrain.WorldFrustumPlanes));

		dc->UpdateSubresource(mFrameTerrainCB, 0, 0, &mPerFrameTerrain, 0, 0);
		dc->UpdateSubresource(mObjectTerrainCB, 0, 0, &mPerObjectTerrain, 0, 0);

		dc->VSSetShaderResources(0, 1, &mLayerMapArraySRV);
		dc->VSSetShaderResources(1, 1, &mBlendMapSRV);
		dc->VSSetShaderResources(2, 1, &mHeightMapSRV);
		dc->VSSetSamplers(0, 1, &mSamHeightMap);
		dc->VSSetSamplers(1, 1, &mSamLinear);
		dc->VSSetConstantBuffers(0, 1, &mObjectTerrainCB);
		dc->VSSetConstantBuffers(1, 1, &mFrameTerrainCB);

		dc->HSSetShaderResources(0, 1, &mLayerMapArraySRV);
		dc->HSSetShaderResources(1, 1, &mBlendMapSRV);
		dc->HSSetShaderResources(2, 1, &mHeightMapSRV);
		dc->HSSetSamplers(0, 1, &mSamHeightMap);
		dc->HSSetSamplers(1, 1, &mSamLinear);
		dc->HSSetConstantBuffers(0, 1, &mObjectTerrainCB);
		dc->HSSetConstantBuffers(1, 1, &mFrameTerrainCB);

		dc->DSSetShaderResources(0, 1, &mLayerMapArraySRV);
		dc->DSSetShaderResources(1, 1, &mBlendMapSRV);
		dc->DSSetShaderResources(2, 1, &mHeightMapSRV);
		dc->DSSetSamplers(0, 1, &mSamHeightMap);
		dc->DSSetSamplers(1, 1, &mSamLinear);
		dc->DSSetConstantBuffers(0, 1, &mObjectTerrainCB);
		dc->DSSetConstantBuffers(1, 1, &mFrameTerrainCB);

		dc->PSSetShaderResources(0, 1, &mLayerMapArraySRV);
		dc->PSSetShaderResources(1, 1, &mBlendMapSRV);
		dc->PSSetShaderResources(2, 1, &mHeightMapSRV);
		dc->PSSetSamplers(0, 1, &mSamHeightMap);
		dc->PSSetSamplers(1, 1, &mSamLinear);
		dc->PSSetConstantBuffers(0, 1, &mObjectTerrainCB);
		dc->PSSetConstantBuffers(1, 1, &mFrameTerrainCB);

		dc->VSSetShader(mTerrainVS, 0, 0);
		dc->HSSetShader(mTerrainHS, 0, 0);
		dc->DSSetShader(mTerrainDS, 0, 0);
		dc->PSSetShader(mTerrainPS, 0, 0);

		dc->DrawIndexed(mNumPatchQuadFaces * 4, 0, 0);

		dc->HSSetShader(0, 0, 0);
		dc->DSSetShader(0, 0, 0);
	}

	void Terrain::buildTerrain(ID3D11Device* device)
	{
		using namespace common;

		// 입력 레이아웃
		const D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		// 쉐이더 컴파일
		ID3D10Blob* blob;
		HR(D3DHelper::CompileShaderFromFile(L"Terrain.hlsl", "VS", "vs_5_0", &blob));
		HR(device->CreateVertexShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mTerrainVS
		));
		HR(device->CreateInputLayout(
			inputLayoutDesc,
			ARRAYSIZE(inputLayoutDesc),
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			&mTerrainIL
		));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(L"Terrain.hlsl", "HS", "hs_5_0", &blob));
		HR(device->CreateHullShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mTerrainHS
		));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(L"Terrain.hlsl", "DS", "ds_5_0", &blob));
		HR(device->CreateDomainShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mTerrainDS
		));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(L"Terrain.hlsl", "PS", "ps_5_0", &blob));
		HR(device->CreatePixelShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mTerrainPS
		));
		ReleaseCOM(blob);

		// 샘플 스테이트
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 비교를 통과하지 않는다?
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		device->CreateSamplerState(&samplerDesc, &mSamLinear);

		samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 비교를 통과하지 않는다?
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		device->CreateSamplerState(&samplerDesc, &mSamHeightMap);

		// 상수 버퍼
		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		static_assert(sizeof(PerFrameTerrain) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerFrameTerrain);
		HR(device->CreateBuffer(&cbd, NULL, &mFrameTerrainCB));

		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		static_assert(sizeof(PerObjectTerrain) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerObjectTerrain);
		HR(device->CreateBuffer(&cbd, NULL, &mObjectTerrainCB));
	}

	void Terrain::LoadHeightmap()
	{
		std::vector<unsigned char> in(mInfo.HeightmapWidth * mInfo.HeightmapHeight);

		std::ifstream inFile;
		inFile.open(mInfo.HeightMapFilename.c_str(), std::ios_base::binary);

		if (inFile)
		{
			inFile.read((char*)&in[0], (std::streamsize)in.size());

			inFile.close();
		}

		mHeightmap.resize(mInfo.HeightmapHeight * mInfo.HeightmapWidth, 0);
		for (UINT i = 0; i < mInfo.HeightmapHeight * mInfo.HeightmapWidth; ++i)
		{
			// 데이터를 정규화 한 후 scale로 조정한다.
			mHeightmap[i] = (in[i] / 255.0f) * mInfo.HeightScale;
		}
	}

	void Terrain::Smooth()
	{
		std::vector<float> dest(mHeightmap.size());

		for (UINT i = 0; i < mInfo.HeightmapHeight; ++i)
		{
			for (UINT j = 0; j < mInfo.HeightmapWidth; ++j)
			{
				// average는 (현재 픽셀 + sum(인접한 픽셀 값)) / 개수
				dest[i * mInfo.HeightmapWidth + j] = Average(i, j);
			}
		}

		mHeightmap = dest;
	}

	bool Terrain::InBounds(int i, int j)
	{
		// True if ij are valid indices; false otherwise.
		return
			i >= 0 && i < (int)mInfo.HeightmapHeight &&
			j >= 0 && j < (int)mInfo.HeightmapWidth;
	}

	float Terrain::Average(int i, int j)
	{
		// Function computes the average height of the ij element.
		// It averages itself with its eight neighbor pixels.  Note
		// that if a pixel is missing neighbor, we just don't include it
		// in the average--that is, edge pixels don't have a neighbor pixel.
		//
		// ----------
		// | 1| 2| 3|
		// ----------
		// |4 |ij| 6|
		// ----------
		// | 7| 8| 9|
		// ----------

		float avg = 0.0f;
		float num = 0.0f;

		// Use int to allow negatives.  If we use UINT, @ i=0, m=i-1=UINT_MAX
		// and no iterations of the outer for loop occur.
		for (int m = i - 1; m <= i + 1; ++m)
		{
			for (int n = j - 1; n <= j + 1; ++n)
			{
				if (InBounds(m, n))
				{
					avg += mHeightmap[m * mInfo.HeightmapWidth + n];
					num += 1.0f;
				}
			}
		}

		return avg / num;
	}

	void Terrain::CalcAllPatchBoundsY()
	{
		mPatchBoundsY.resize(mNumPatchQuadFaces);

		// 모든 패치(셀 단위)를 순회하여 Bound Y를 구한다.
		for (UINT i = 0; i < mNumPatchVertRows - 1; ++i)
		{
			for (UINT j = 0; j < mNumPatchVertCols - 1; ++j)
			{
				CalcPatchBoundsY(i, j);
			}
		}
	}

	void Terrain::CalcPatchBoundsY(UINT i, UINT j)
	{
		// 셀 안에 들어간 패치만큼 순회하여 min, map를 찾는다.
		UINT x0 = j * CellsPerPatch;
		UINT x1 = (j + 1) * CellsPerPatch;

		UINT y0 = i * CellsPerPatch;
		UINT y1 = (i + 1) * CellsPerPatch;

		float minY = +MathHelper::Infinity;
		float maxY = -MathHelper::Infinity;

		for (UINT y = y0; y <= y1; ++y)
		{
			for (UINT x = x0; x <= x1; ++x)
			{
				UINT k = y * mInfo.HeightmapWidth + x;
				minY = MathHelper::Min(minY, mHeightmap[k]);
				maxY = MathHelper::Max(maxY, mHeightmap[k]);
			}
		}

		UINT patchID = i * (mNumPatchVertCols - 1) + j;
		mPatchBoundsY[patchID] = Vector2(minY, maxY);
	}

	void Terrain::BuildQuadPatchVB(ID3D11Device* device)
	{
		// 패치 제어점 만큼 정점 생성
		std::vector<VertexTerrain> patchVertices(mNumPatchVertRows * mNumPatchVertCols);

		float halfWidth = 0.5f * GetWidth();
		float halfDepth = 0.5f * GetDepth();

		// 셀의 너비는 (전체 셀 너비 / 샐 개수)
		float patchWidth = GetWidth() / (mNumPatchVertCols - 1);
		float patchDepth = GetDepth() / (mNumPatchVertRows - 1);
		float du = 1.0f / (mNumPatchVertCols - 1);
		float dv = 1.0f / (mNumPatchVertRows - 1);

		for (UINT i = 0; i < mNumPatchVertRows; ++i)
		{
			float z = halfDepth - i * patchDepth; // 화면 깊은쪽 부터

			for (UINT j = 0; j < mNumPatchVertCols; ++j)
			{
				float x = -halfWidth + j * patchWidth; // 왼쪽 부터

				patchVertices[i * mNumPatchVertCols + j].Pos = Vector3(x, 0.0f, z);

				patchVertices[i * mNumPatchVertCols + j].Tex.x = j * du;
				patchVertices[i * mNumPatchVertCols + j].Tex.y = i * dv;
			}
		}

		for (UINT i = 0; i < mNumPatchVertRows - 1; ++i)
		{
			for (UINT j = 0; j < mNumPatchVertCols - 1; ++j)
			{
				// 패치를 감싸는 높이의 최소/최대를 미리 계산한 것을 적용해준다.
				UINT patchID = i * (mNumPatchVertCols - 1) + j;
				patchVertices[i * mNumPatchVertCols + j].BoundsY = mPatchBoundsY[patchID];
			}
		}

		// d3d vertex buffer 생성

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(VertexTerrain) * patchVertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &patchVertices[0];
		HR(device->CreateBuffer(&vbd, &vinitData, &mQuadPatchVB));
	}

	void Terrain::BuildQuadPatchIB(ID3D11Device* device)
	{
		// 제어점 패치 형태라서 Face당 4개의 색인을 갖는다. 
		std::vector<USHORT> indices(mNumPatchQuadFaces * 4);

		int k = 0;
		for (UINT i = 0; i < mNumPatchVertRows - 1; ++i)
		{
			for (UINT j = 0; j < mNumPatchVertCols - 1; ++j)
			{
				// 윗 정점 두개
				indices[k] = i * mNumPatchVertCols + j;
				indices[k + 1] = i * mNumPatchVertCols + j + 1;

				// 아랫 정점 두개
				indices[k + 2] = (i + 1) * mNumPatchVertCols + j;
				indices[k + 3] = (i + 1) * mNumPatchVertCols + j + 1;

				k += 4;
			}
		}

		// d3d index buffer 생성

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(USHORT) * indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(device->CreateBuffer(&ibd, &iinitData, &mQuadPatchIB));
	}

	void Terrain::BuildHeightmapSRV(ID3D11Device* device)
	{
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = mInfo.HeightmapWidth;
		texDesc.Height = mInfo.HeightmapHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R32_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		// 16비트 float 만들기 로직

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = &mHeightmap[0];
		data.SysMemPitch = mInfo.HeightmapWidth * sizeof(float);
		data.SysMemSlicePitch = 0;

		ID3D11Texture2D* hmapTex = 0;
		HR(device->CreateTexture2D(&texDesc, &data, &hmapTex));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1; // 최대 밉맵 수, -1은 모든 밉맵 레벨 표시
		HR(device->CreateShaderResourceView(hmapTex, &srvDesc, &mHeightMapSRV));

		ReleaseCOM(hmapTex);
	}
}