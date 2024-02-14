#pragma once

#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "Camera.h"
#include "Sky.h"
#include "LightHelper.h"

namespace normalDisplacementMap
{
	class Basic32;

	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct PerObjectDisplacement
	{
		Matrix World;
		Matrix WorldInvTranspose;
		Matrix ViewProj;
		Matrix WorldViewProj;
		Matrix Tex;
		Material Material;
	};
	struct PerFrameDisplacement
	{
		DirectionLight DirLights[3];
		Vector3 EyePosW;
		int LightCount;
		Vector4 FogColor;
		float  FogStart;
		float  FogRange;
		int bUseTexure;
		int bUseLight;
		int bUseFog;

		float HeightScale;
		float MaxTessDistance;
		float MinTessDistance;
		float MaxTessFactor;
		float MinTessFactor;
		float unused[2];
	};

	struct PosNormalTexTan
	{
		Vector3 Pos;
		Vector3 Normal;
		Vector2 Tex;
		Vector3 TangentU;
	};

	struct Pos
	{
		Vector3 Pos;
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
		void buildElement();
		void buildSky();

		void buildShapeGeometryBuffers();
		void buildSkullGeometryBuffers();

	private:
		CBPerFramePos mCBPerFramePos;
		ID3D11Buffer* mPerFrameCBPos;
		ID3D11VertexShader* mVertexShaderPos;
		ID3D11PixelShader* mPixelShaderPos;
		ID3D11InputLayout* mPosLayout;
		ID3D11SamplerState* mLinearSampler;

		ID3D11VertexShader* mNormalVS;
		ID3D11PixelShader* mNormalPS;
		ID3D11VertexShader* mDisplacementVS;
		ID3D11HullShader* mDisplacementHS;
		ID3D11DomainShader* mDisplacementDS;
		ID3D11PixelShader* mDisplacementPS;
		ID3D11SamplerState* mSamLinear;
		ID3D11Buffer* mPerObjectDisplacemnetCB;
		ID3D11Buffer* mPerFrameDisplacemnetCB;
		PerFrameDisplacement mCBPerFrameDisplacement;
		PerObjectDisplacement mCBPerObjectDisplacement;
		ID3D11InputLayout* mDisplacementIL;

		Basic32* mBasic32;
		Camera mCam;
		POINT mLastMousePos;

		Sky* mSky;

		ID3D11Buffer* mShapesVB;
		ID3D11Buffer* mShapesIB;

		ID3D11Buffer* mSkullVB;
		ID3D11Buffer* mSkullIB;

		ID3D11Buffer* mSkySphereVB;
		ID3D11Buffer* mSkySphereIB;

		ID3D11ShaderResourceView* mStoneTexSRV;
		ID3D11ShaderResourceView* mBrickTexSRV;

		ID3D11ShaderResourceView* mStoneNormalTexSRV;
		ID3D11ShaderResourceView* mBrickNormalTexSRV;

		DirectionLight mDirLights[3];
		Material mGridMat;
		Material mBoxMat;
		Material mCylinderMat;
		Material mSphereMat;
		Material mSkullMat;

		// Define transformations from local spaces to world space.
		XMFLOAT4X4 mSphereWorld[10];
		XMFLOAT4X4 mCylWorld[10];
		XMFLOAT4X4 mBoxWorld;
		XMFLOAT4X4 mGridWorld;
		XMFLOAT4X4 mSkullWorld;

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
	};
}