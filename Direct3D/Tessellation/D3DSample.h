#pragma once

#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "LightHelper.h"

namespace tessellation
{
	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct Pos
	{
		Vector3 Pos;
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
		DirectionLight DirLight[3];
		Vector3 EyePosW;
		int LigthCount;
		Vector4 FogColor;
		float FogStart;
		float FogRange;
		UINT bUseTexutre;
		UINT bUseLight;
		UINT bUseFog;
		UINT unused1[3];
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

		void buildQuadPatchBuffer();

	private:
		CBPerObject mCBPerObject;
		CBPerFrame mCBPerFrame;
		ID3D11Buffer* mPerObjectCB;
		ID3D11Buffer* mPerFrameCB;

		ID3D11InputLayout* mInputLayout;
		ID3D11VertexShader* mVertexShader;
		ID3DBlob* mVertexShaderBlob;
		ID3D11HullShader* mHullShader;
		ID3D11DomainShader* mDomainShader;
		ID3D11PixelShader* mPixelShader;

		ID3D11VertexShader* mVertexShaderBazier;
		ID3DBlob* mVertexShaderBlobBazier;
		ID3D11HullShader* mHullShaderBazier;
		ID3D11DomainShader* mDomainShaderBazier;
		ID3D11PixelShader* mPixelShaderBazier;

		ID3D11Buffer* mQuadPatchVB;
		ID3D11Buffer* mQuadPatchBazierVB;

		Matrix mView;
		Matrix mProj;

		Vector3 mEyePosW;

		float mTheta;
		float mPhi;
		float mRadius;

		POINT mLastMousePos;
	};
}