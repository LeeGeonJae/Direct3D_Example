#pragma once

#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "Camera.h"
#include "LightHelper.h"

namespace shadows
{
	class Basic32;
	class ShadowMap;

	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct PerObject
	{
		Matrix World;
		Matrix WorldInvTranspose;
		Matrix ViewProj;
		Matrix WorldViewProj;
		Matrix Tex;
		Matrix ShadowTransform;
		Material Material;
	};

	struct PerFrame
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
		int unused[3];
	};

	struct PerFrameShadow
	{
		Vector3 EyePosW;
		float unused;
	};

	struct PosNormalTexTan
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 Tex;
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
		void buildInit();

		void drawSceneToShadowMap();
		void drawScreenQuad();
		void buildShadowTransform();
		void buildShapeGeometryBuffers();
		void buildSkullGeometryBuffers();
		void buildScreenQuadGeometryBuffers();

	private:
		PerObject mPerObject;
		PerFrame mPerFrame;
		PerFrameShadow mPerFrameShadow;
		ID3D11Buffer* mObjectCB;
		ID3D11Buffer* mFrameCB;
		ID3D11Buffer* mFrameShadowCB;
		ID3D11InputLayout* mInputLayout;
		ID3D11VertexShader* mVSShader;
		ID3D11PixelShader* mPSShader;
		ID3D11SamplerState* mSamShadow;

		Basic32* mBasic32;
		Camera mCam;
		POINT mLastMousePos;

		ID3D11Buffer* mShapesVB;
		ID3D11Buffer* mShapesIB;

		ID3D11Buffer* mSkullVB;
		ID3D11Buffer* mSkullIB;

		ID3D11Buffer* mSkySphereVB;
		ID3D11Buffer* mSkySphereIB;

		ID3D11Buffer* mScreenQuadVB;
		ID3D11Buffer* mScreenQuadIB;

		ID3D11ShaderResourceView* mStoneTexSRV;
		ID3D11ShaderResourceView* mBrickTexSRV;

		ID3D11ShaderResourceView* mStoneNormalTexSRV;
		ID3D11ShaderResourceView* mBrickNormalTexSRV;

		BoundingSphere mSceneBounds;

		static const int SMapSize = 2048;
		ShadowMap* mSmap;
		XMFLOAT4X4 mLightView;
		XMFLOAT4X4 mLightProj;
		XMFLOAT4X4 mShadowTransform;

		float mLightRotationAngle;
		XMFLOAT3 mOriginalLightDir[3];
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