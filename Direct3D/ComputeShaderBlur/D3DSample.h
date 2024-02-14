#pragma once

#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "BlurFilter.h"
#include "Waves.h"
#include "LightHelper.h"

namespace computeShaderBlur
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
		void updateCBPerObject(const Matrix& worldMat, const Matrix& TexMat, const Material& material);

		void buildShader();
		void buildInputLayout();
		void buildConstantBuffer();

		void updateWaves();
		void drawWrapper();
		void drawScreenQuad();
		float getHillHeight(float x, float z)const;
		Vector3 getHillNormal(float x, float z)const;
		void buildLandGeometryBuffers();
		void buildWaveGeometryBuffers();
		void buildCrateGeometryBuffers();
		void buildScreenQuadGeometryBuffers();
		void buildOffscreenViews();

	private:
		CBPerObject mCBPerObject;
		CBPerFrame mCBPerFrame;
		ID3D11Buffer* mPerObjectCB;
		ID3D11Buffer* mPerFrameCB;

		ID3D11VertexShader* mVertexShader;
		ID3DBlob* mVertexShaderBlob;
		ID3D11PixelShader* mPixelShader;
		ID3D11InputLayout* mInputLayout;
		ID3D11SamplerState* mLinearSampler;
		ID3D11ComputeShader* mComputeShaderH;
		ID3D11ComputeShader* mComputeShaderV;

		ID3D11Buffer* mLandVB;
		ID3D11Buffer* mLandIB;

		ID3D11Buffer* mWavesVB;
		ID3D11Buffer* mWavesIB;

		ID3D11Buffer* mBoxVB;
		ID3D11Buffer* mBoxIB;

		ID3D11Buffer* mScreenQuadVB;
		ID3D11Buffer* mScreenQuadIB;

		ID3D11ShaderResourceView* mGrassMapSRV;
		ID3D11ShaderResourceView* mWavesMapSRV;
		ID3D11ShaderResourceView* mCrateSRV;

		ID3D11ShaderResourceView* mOffscreenSRV;
		ID3D11UnorderedAccessView* mOffscreenUAV;
		ID3D11RenderTargetView* mOffscreenRTV;

		BlurFilter mBlur;
		Waves mWaves;

		DirectionLight mDirLights[3];
		Material mLandMat;
		Material mWavesMat;
		Material mBoxMat;

		Matrix mGrassTexTransform;
		Matrix mWaterTexTransform;
		Matrix mLandWorld;
		Matrix mWavesWorld;
		Matrix mBoxWorld;

		Matrix mView;
		Matrix mProj;

		UINT mLandIndexCount;
		UINT mWaveIndexCount;

		Vector2 mWaterTexOffset;

		Vector3 mEyePosW;

		float mTheta;
		float mPhi;
		float mRadius;

		POINT mLastMousePos;
	};
}