#pragma once

#include <vector>
#include <directxcollision.h>
#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "Camera.h"
#include "LightHelper.h"

namespace instancingAndCulling
{
	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct InstancedData
	{
		Matrix World;
		Color Color;
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
		Matrix ViewProj;
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
		bool bUseTexutre;
		bool unused1[7];
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
		void buildShader();
		void buildInputLayout();

		void buildSkullGeometryBuffers();
		void buildInstancedBuffer();

	private:
		CBPerObject mCBPerObject;
		CBPerFrame mCBPerFrame;
		ID3D11Buffer* mPerFrameCB;
		ID3D11Buffer* mPerObjectCB;

		ID3D11VertexShader* mVertexShader;
		ID3D10Blob* mVertexShaderBuffer;
		ID3D11PixelShader* mPixelShader;
		ID3D11SamplerState* mLinearSampleState;
		ID3D11InputLayout* mBasic32;
		ID3D11InputLayout* mInstancedBasic32;

		ID3D11Buffer* mSkullVB;
		ID3D11Buffer* mSkullIB;
		ID3D11Buffer* mInstancedBuffer;

		// Bounding box of the skull.
		BoundingBox mSkullBox;
		BoundingFrustum mCamFrustum;

		UINT mVisibleObjectCount;

		// Keep a system memory copy of the world matrices for culling.
		std::vector<InstancedData> mInstancedData;

		bool mFrustumCullingEnabled;

		DirectionLight mDirLights[3];
		Material mSkullMat;

		// Define transformations from local spaces to world space.
		Matrix mSkullWorld;

		UINT mSkullIndexCount;

		Camera mCam;

		POINT mLastMousePos;
	};
}