#pragma once

#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "Camera.h"
#include "LightHelper.h"

namespace common
{
	class Sky;
}

namespace dynamicCubeMap
{
	class Basic32;

	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

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
		void buildSky();

		void drawScene(const Camera& camera, bool bDrawCenterSphere);
		void buildCubeFaceCamera(float x, float y, float z);
		void buildDynamicCubeMapViews();
		void buildShapeGeometryBuffers();
		void buildSkullGeometryBuffers();

	private:
		CBPerFramePos mCBPerFramePos;
		ID3D11Buffer* mPerFrameCBPos;
		ID3D11VertexShader* mVertexShaderPos;
		ID3D11PixelShader* mPixelShaderPos;
		ID3D11InputLayout* mPosLayout;
		ID3D11SamplerState* mLinearSampler;

		Basic32* mBasic32;
		Camera mCam;
		POINT mLastMousePos;

		common::Sky* mSky;

		ID3D11Buffer* mShapesVB;
		ID3D11Buffer* mShapesIB;

		ID3D11Buffer* mSkullVB;
		ID3D11Buffer* mSkullIB;

		ID3D11ShaderResourceView* mFloorTexSRV;
		ID3D11ShaderResourceView* mStoneTexSRV;
		ID3D11ShaderResourceView* mBrickTexSRV;

		ID3D11DepthStencilView* mDynamicCubeMapDSV;
		ID3D11RenderTargetView* mDynamicCubeMapRTV[6];
		ID3D11ShaderResourceView* mDynamicCubeMapSRV;
		D3D11_VIEWPORT mCubeMapViewport;

		static const int CubeMapSize = 256;

		DirectionLight mDirLights[3];
		Material mGridMat;
		Material mBoxMat;
		Material mCylinderMat;
		Material mSphereMat;
		Material mSkullMat;
		Material mCenterSphereMat;

		// Define transformations from local spaces to world space.
		Matrix mSphereWorld[10];
		Matrix mCylWorld[10];
		Matrix mBoxWorld;
		Matrix mGridWorld;
		Matrix mSkullWorld;
		Matrix mCenterSphereWorld;

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

		Camera mCubeMapCamera[6];
	};
}