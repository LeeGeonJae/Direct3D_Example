#pragma once

#include <directxtk/SimpleMath.h>

#include "d3dUtil.h"
#include "LightHelper.h"
#include "Camera.h"

namespace terrain
{
	using DirectX::SimpleMath::Matrix;
	using DirectX::SimpleMath::Vector2;
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Vector4;
	using namespace common;

	struct VertexTerrain
	{
		Vector3 Pos;
		Vector2 Tex;
		Vector2 BoundsY;
	};

	struct PerObjectTerrain
	{
		Matrix ViewProj;
		Material Material;
	};

	struct PerFrameTerrain
	{
		common::DirectionLight DirLights[3];
		Vector3 EyePosW;
		float  FogStart;
		Vector4 FogColor;
		float  FogRange;
		float MinDist;
		float MaxDist;
		float MinTess;
		float MaxTess;
		float TexelCellSpaceU;
		float TexelCellSpaceV;
		float WorldCellSpace;
		Vector4 WorldFrustumPlanes[6];
		Vector2 TexScale; // = 50.0f;
		float unuserd[2];
	};

	class Terrain
	{
	public:
		struct InitInfo
		{
			std::wstring HeightMapFilename;
			std::wstring LayerMapFilename0;
			std::wstring LayerMapFilename1;
			std::wstring LayerMapFilename2;
			std::wstring LayerMapFilename3;
			std::wstring LayerMapFilename4;
			std::wstring BlendMapFilename;
			float HeightScale;
			UINT HeightmapWidth;
			UINT HeightmapHeight;
			float CellSpacing;
		};

	public:
		Terrain();
		~Terrain();

		void Init(ID3D11Device* device, ID3D11DeviceContext* dc, const InitInfo& initInfo);
		void Draw(ID3D11DeviceContext* dc, const Camera& cam, DirectionLight lights[3]);

		inline void SetWorld(Matrix M);

		inline float GetWidth() const;
		inline float GetDepth() const;
		inline float GetHeight(float x, float z) const;
		inline Matrix GetWorld()const;

	private:
		void buildTerrain(ID3D11Device* device);

		void LoadHeightmap();
		void Smooth();
		bool InBounds(int i, int j);
		float Average(int i, int j);
		void CalcAllPatchBoundsY();
		void CalcPatchBoundsY(UINT i, UINT j);
		void BuildQuadPatchVB(ID3D11Device* device);
		void BuildQuadPatchIB(ID3D11Device* device);
		void BuildHeightmapSRV(ID3D11Device* device);

	private:
		static const int CellsPerPatch = 64;

		// 상수 버퍼 만드는 게 귀찮았다.
		PerObjectTerrain mPerObjectTerrain;
		PerFrameTerrain mPerFrameTerrain;
		ID3D11Buffer* mFrameTerrainCB;
		ID3D11Buffer* mObjectTerrainCB;

		// 생성한 셰이더를 다 바인딩할 거라 귀찮았다.
		ID3D11VertexShader* mTerrainVS;
		ID3D11HullShader* mTerrainHS;
		ID3D11DomainShader* mTerrainDS;
		ID3D11PixelShader* mTerrainPS;
		ID3D11InputLayout* mTerrainIL;

		// 샘플러 스테이트도 랜더 스테이트로 넣어두자, 참조해서 쓰도록
		ID3D11SamplerState* mSamLinear;
		ID3D11SamplerState* mSamHeightMap;

		ID3D11Buffer* mQuadPatchVB;
		ID3D11Buffer* mQuadPatchIB;

		// 문자열 하나 전달하면 알아서 생성되도록
		ID3D11ShaderResourceView* mLayerMapArraySRV;
		ID3D11ShaderResourceView* mBlendMapSRV;
		ID3D11ShaderResourceView* mHeightMapSRV;

		InitInfo mInfo;

		UINT mNumPatchVertices;
		UINT mNumPatchQuadFaces;

		UINT mNumPatchVertRows;
		UINT mNumPatchVertCols;

		Matrix mWorld;

		Material mMat;

		std::vector<Vector2> mPatchBoundsY;
		std::vector<float> mHeightmap;
	};

	void Terrain::SetWorld(Matrix M)
	{
		mWorld = M;
	}

	float Terrain::GetWidth()const
	{
		return (mInfo.HeightmapWidth - 1) * mInfo.CellSpacing;
	}
	float Terrain::GetDepth()const
	{
		return (mInfo.HeightmapHeight - 1) * mInfo.CellSpacing;
	}
	float Terrain::GetHeight(float x, float z)const
	{
		// 월드 x, y 좌표 정보를 통해 낱칸의 색인을 구한다.
		// 너비의 절반을 x에 곱하고, 셀 크기로 나눠주면 부동 소수점 좌표가 구해진다.
		float c = (x + 0.5f * GetWidth()) / mInfo.CellSpacing; 
		float d = (z - 0.5f * GetDepth()) / -mInfo.CellSpacing;

		// 버림으로 낮은 색인을 구해온다.
		int row = (int)floorf(d);
		int col = (int)floorf(c);

		// A B
		// C D
		float A = mHeightmap[row * mInfo.HeightmapWidth + col];
		float B = mHeightmap[row * mInfo.HeightmapWidth + col + 1];
		float C = mHeightmap[(row + 1) * mInfo.HeightmapWidth + col];
		float D = mHeightmap[(row + 1) * mInfo.HeightmapWidth + col + 1];

		// 포함된 삼각형을 구하기 위해 셀 내의 좌표를 구한다.
		float s = c - (float)col;
		float t = d - (float)row;

		// 이 합이 1보다 작다는 것은 위쪽 삼각형을 의미한다.
		if (s + t <= 1.0f)
		{
			// A -> B
			// C
			float uy = B - A;
			float vy = C - A;

			return A + s * uy + t * vy;
		}
		else
		{
			//      B
			// C <- D
			float uy = C - D;
			float vy = B - D;

			return D + (1.0f - s) * uy + (1.0f - t) * vy;
		}
	}
	Matrix Terrain::GetWorld()const
	{
		return mWorld;
	}
}