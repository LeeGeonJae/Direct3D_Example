#pragma once

#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "LightHelper.h"
#include "Camera.h"
#include "Sky.h"

namespace cubeMap
{
	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct Pos
	{
		Vector3 Pos;
	};

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

	struct CBPerFramePos
	{
		Matrix WorldViewProj;
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

		void buildShapeGeometryBuffers();
		void buildSkullGeometryBuffers();

	private:
		CBPerObject mCBPerObject;
		CBPerFrame mCBPerFrame;
		CBPerFramePos mCBPerFramePos;
		ID3D11Buffer* mPerFrameCB;
		ID3D11Buffer* mPerFrameCBPos;
		ID3D11Buffer* mPerObjectCB;

		ID3D11VertexShader* mVertexShader;
		ID3D11VertexShader* mVertexShaderPos;
		ID3D10Blob* mVertexShaderBuffer;
		ID3D10Blob* mVertexShaderBufferPos;
		ID3D11PixelShader* mPixelShader;
		ID3D11PixelShader* mPixelShaderPos;
		ID3D11InputLayout* mBasic32;
		ID3D11InputLayout* mPosLayout;
		ID3D11SamplerState* mLinearSampleState;

		Sky* mSky;

		ID3D11Buffer* mShapesVB;
		ID3D11Buffer* mShapesIB;

		ID3D11Buffer* mSkullVB;
		ID3D11Buffer* mSkullIB;

		ID3D11Buffer* mSkySphereVB;
		ID3D11Buffer* mSkySphereIB;

		ID3D11ShaderResourceView* mFloorTexSRV;
		ID3D11ShaderResourceView* mStoneTexSRV;
		ID3D11ShaderResourceView* mBrickTexSRV;

		DirectionLight mDirLights[3];
		Material mGridMat;
		Material mBoxMat;
		Material mCylinderMat;
		Material mSphereMat;
		Material mSkullMat;

		Matrix mSphereWorld[10];
		Matrix mCylWorld[10];
		Matrix mBoxWorld;
		Matrix mGridWorld;
		Matrix mSkullWorld;

		int mBoxVertexOffset;
		int mGridVertexOffset;
		int mSphereVertexOffset;
		int mCylinderVertexOffset;

		UINT mBoxIndexOffset;
		UINT mGridIndexOffset;
		UINT mSphereIndexOffset;
		UINT mCylinderIndexOffset;

		UINT mBoxIndexCount;
		UINT mGridIndexCount;
		UINT mSphereIndexCount;
		UINT mCylinderIndexCount;

		UINT mSkullIndexCount;

		UINT mLightCount;

		Camera mCam;

		POINT mLastMousePos;
	};
}