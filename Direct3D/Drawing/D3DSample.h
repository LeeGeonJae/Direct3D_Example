#pragma once

#include <directxtk/SimpleMath.h>
#include <Windows.h>
#include <vector>

#include "D3dProcessor.h"
#include "Waves.h"

namespace drawing
{
	using namespace common;

	struct Vertex
	{
		DirectX::SimpleMath::Vector3 Position;
		DirectX::SimpleMath::Vector4 Color;
	};

	struct CBPerObject
	{
		DirectX::SimpleMath::Matrix WVPMat;
	};

	enum class eShapeType
	{
		Box,
		Grid,
		Sphere,
		Cyliner
	};

	class D3DSample final : public D3DProcessor
	{
	public:
		D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name);
		~D3DSample();

		bool Init() override;
		void OnResize() override;

		void Update(float deltaTime) override;
		void Render() override;

		void OnMouseDown(WPARAM btnState, int x, int y) override;
		void OnMouseUp(WPARAM btnState, int x, int y) override;
		void OnMouseMove(WPARAM btnState, int x, int y) override;

	private:
		void drawObject(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, UINT indexSize, const DirectX::SimpleMath::Matrix& worldMatrix = DirectX::SimpleMath::Matrix::Identity);
		void drawShape(eShapeType shapeType, const DirectX::SimpleMath::Matrix& worldMatrix = DirectX::SimpleMath::Matrix::Identity);

		float getHeight(float x, float z) const;
		void buildGeometryBuffers();
		void buildBox();
		void buildGrid();
		void buildShape();
		void buildSkull();
		void buildWaves();

		void buildShader();
		void buildVertexLayout();

	private:
		ID3D11Buffer* mPerObjectCB;
		CBPerObject mCBPerObject;

		ID3D11Buffer* mBoxVB;
		ID3D11Buffer* mBoxIB;
		UINT mBoxIndexCount;

		ID3D11Buffer* mGridVB;
		ID3D11Buffer* mGridIB;
		UINT mGridIndexCount;

		// 하나의 VB와 IB에 여러 데이터 담기
		std::vector<UINT> mVertexOffsets;
		std::vector<UINT> mIndexOffsets;
		std::vector<UINT> mIndexCounts;
		std::vector<std::pair<eShapeType, DirectX::SimpleMath::Matrix>> mWorldMatrixs;
		ID3D11Buffer* mShapeVB;
		ID3D11Buffer* mShapeIB;

		// 파일로딩
		ID3D11Buffer* mSkullVB;
		ID3D11Buffer* mSkullIB;
		UINT mSkullIndexCount;

		// map/unmap 활용
		ID3D11Buffer* mWavesVB;
		ID3D11Buffer* mWavesIB;
		Waves mWaves;

		ID3D11VertexShader* mVertexShader;
		ID3DBlob* mVertexShaderBuffer;
		ID3D11PixelShader* mPixelShader;
		ID3D11InputLayout* mInputLayout;

		DirectX::SimpleMath::Matrix mWorld;
		DirectX::SimpleMath::Matrix mView;
		DirectX::SimpleMath::Matrix mProj;

		float mTheta;
		float mPhi;
		float mRadius;

		POINT mLastMousePos;
	};
}