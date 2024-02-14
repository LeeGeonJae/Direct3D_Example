#pragma once

#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "D3DUtil.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Waves.h"

namespace vertex
{
	using namespace DirectX::SimpleMath;

	struct Basic32
	{
		Basic32() = default;
		Basic32(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty)
			: Pos(px, py, pz)
			, Normal(nx, ny, nz)
			, Tex(tx, ty)
		{
		}

		Vector3 Pos;
		Vector3 Normal;
		Vector2 Tex;
	};

	struct Sprite
	{
		Vector3 Pos;
		Vector2 Size;
	};
}

namespace geometryShader
{
	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct CBPerObjectBasic32
	{
		Matrix World;
		Matrix WorldInvTranspose;
		Matrix WorldViewProj;
		Matrix Tex;
		Material Material;
	};

	struct CBPerObjectSprite
	{
		Matrix ViewProj;
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

	public:
		void setShader(ID3D11VertexShader* VS, ID3D11GeometryShader* GS, ID3D11PixelShader* PS);

		void buildConstantBuffer();
		void buildShader();
		void buildInputLayout();

		float getHillHeight(float x, float z) const;
		Vector3 getHillNormal(float x, float z) const;
		void buildLandGeometryBuffers();
		void buildWaveGeometryBuffers();
		void buildCrateGeometryBuffers();
		void buildTreeSpritesBuffer();
		void drawTreeSprites(Matrix viewProj);

	private:
		CBPerObjectBasic32 mCBPerObjectBasic32;
		CBPerObjectSprite mCBPerObjectSprite;
		CBPerFrame mCBPerFrame;
		ID3D11Buffer* mPerFrameCB;

		ID3D11Buffer* mPerObjectCBBasic32;
		ID3D11InputLayout* mInputLayoutBasic32;
		ID3D11VertexShader* mVertexShaderBasic32;
		ID3DBlob* mVertexShaderBlobBasic32;
		ID3D11PixelShader* mPixelShaderBasic32;

		ID3D11Buffer* mPerObjectCBSprite;
		ID3D11InputLayout* mInputLayoutSprite;
		ID3D11VertexShader* mVertexShaderSprite;
		ID3DBlob* mVertexShaderBlobSprite;
		ID3D11GeometryShader* mGeometryShader;
		ID3D11PixelShader* mPixelShaderSprite;

		ID3D11SamplerState* mLinearSamplerWrap;
		ID3D11SamplerState* mLinearSamplerClamp;

		ID3D11Buffer* mLandVB;
		ID3D11Buffer* mLandIB;

		ID3D11Buffer* mWavesVB;
		ID3D11Buffer* mWavesIB;

		ID3D11Buffer* mBoxVB;
		ID3D11Buffer* mBoxIB;

		ID3D11Buffer* mTreeSpritesVB;

		ID3D11ShaderResourceView* mGrassMapSRV;
		ID3D11ShaderResourceView* mWavesMapSRV;
		ID3D11ShaderResourceView* mBoxMapSRV;
		ID3D11ShaderResourceView* mTreeTextureMapArraySRV;

		Waves mWaves;

		DirectionLight mDirLights[3];
		Material mLandMat;
		Material mWavesMat;
		Material mBoxMat;
		Material mTreeMat;

		Matrix mGrassTexTransform;
		Matrix mWaterTexTransform;
		Matrix mLandWorld;
		Matrix mWavesWorld;
		Matrix mBoxWorld;
		Matrix mView;
		Matrix mProj;

		UINT mLandIndexCount;

		static const UINT TreeCount = 32;

		bool mAlphaToCoverageOn;

		Vector2 mWaterTexOffset;

		Vector3 mEyePosW;

		float mTheta;
		float mPhi;
		float mRadius;

		POINT mLastMousePos;
	};
}