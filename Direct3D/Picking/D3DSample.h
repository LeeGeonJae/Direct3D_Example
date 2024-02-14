#pragma once

#include <directxtk/SimpleMath.h>
#include <DirectXCollision.h>
#include <vector>

#include "D3dProcessor.h"
#include "D3DUtil.h"
#include "LightHelper.h"
#include "MathHelper.h"
#include "Camera.h"

namespace picking
{
	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct Basic32
	{
		Vector3 Pos;
		Vector3 Normal;
		Vector2 Tex;
	};

	struct CBPerObject
	{
		Matrix World;
		Matrix WorldInvTranspose;
		Matrix WorldViewProj;
		Matrix Tex;
		Material Material;
	};

	struct CBPerFrame
	{
		DirectionLight DirLights[3];
		Vector3 EyePosW;
		int LigthCount;
		Vector4 FogColor;
		float FogStart;
		float FogRange;
		bool unused1[8];
	};

	class D3DSample final : public D3DProcessor
	{
	public:
		D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name);
		~D3DSample();

		bool Init();
		void OnResize();
		void Update(float deltaTime) override;
		void Render() override;

		void OnMouseDown(WPARAM btnState, int x, int y);
		void OnMouseUp(WPARAM btnState, int x, int y);
		void OnMouseMove(WPARAM btnState, int x, int y);

	private:
		void buildConstantBuffer();
		void buildInputLayout();
		void buildShader();
		void buildBox();

		void buildMeshGeometryBuffers();
		void pick(int sx, int sy);

	private:
		CBPerObject mCBPerObject;
		CBPerFrame mCBPerFrame;
		ID3D11Buffer* mPerFrameCB;
		ID3D11Buffer* mPerObjectCB;

		ID3D11VertexShader* mVertexShader;
		ID3D10Blob* mVertexShaderBuffer;
		ID3D11PixelShader* mPixelShader;
		ID3D11InputLayout* mBasic32;

		ID3D11Buffer* mBoxVB;
		ID3D11Buffer* mBoxIB;
		ID3D11Buffer* mMeshVB;
		ID3D11Buffer* mMeshIB;

		std::vector<Basic32> mMeshVertices;
		std::vector<UINT> mMeshIndices;
		UINT mBoxIndexCount;

		BoundingBox mMeshBox;

		DirectionLight mDirLights[3];
		Material mMeshMat;
		Material mPickedTriangleMat;

		Matrix mMeshWorld;

		UINT mMeshIndexCount;

		UINT mPickedTriangle;

		Camera mCam;

		POINT mLastMousePos;
	};
}