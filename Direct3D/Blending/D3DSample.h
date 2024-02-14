#pragma once

#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "LightHelper.h"
#include "Waves.h"

namespace blending
{
	using namespace common;
	using namespace DirectX::SimpleMath;

	enum class RenderOptions
	{
		Lighting = 0,
		Textures = 1,
		TexturesAndFog = 2
	};

	struct Vertex
	{
		Vector3 Pos;
		Vector3 Normal;
		Vector2 Tex;
	};

	struct CBPerObject
	{
		DirectX::SimpleMath::Matrix World;
		DirectX::SimpleMath::Matrix WorldInvTranspose;
		DirectX::SimpleMath::Matrix WorldViewProj;
		DirectX::SimpleMath::Matrix Tex;
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
		bool unused1[3];
		RenderOptions RenderOptions;
	};

	struct Object
	{
		ID3D11Buffer* VertexBuffer;
		ID3D11Buffer* IndexBuffer;
		UINT IndexCount;
		ID3D11ShaderResourceView* SRV;
		Matrix* WorldMat;
		Matrix* TextureMat;
		Material* Material;
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
		void drawObject(const Object& object);

		float getHillHeight(float x, float z)const;
		Vector3 getHillNormal(float x, float z)const;

		void buildLandGeometryBuffers();
		void buildWaveGeometryBuffers();
		void buildGeometryBuffers();
		void buildShader();
		void buildVertexLayout();
		void buildConstantBuffer();

	private:
		ID3D11Buffer* mPerObjectCB;
		ID3D11Buffer* mPerFrameCB;
		CBPerObject mCBPerObject;
		CBPerFrame mCBPerFrame;

		ID3D11Buffer* mLandVB;
		ID3D11Buffer* mLandIB;

		ID3D11Buffer* mWavesVB;
		ID3D11Buffer* mWavesIB;

		ID3D11Buffer* mBoxVB;
		ID3D11Buffer* mBoxIB;

		ID3D11ShaderResourceView* mGrassMapSRV;
		ID3D11ShaderResourceView* mWavesMapSRV;
		ID3D11ShaderResourceView* mBoxMapSRV;
		ID3D11SamplerState* mLinearSampleState;

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
		UINT mBoxIndexCount;

		Vector2 mWaterTexOffset;

		RenderOptions mRenderOptions;

		Vector3 mEyePosW;

		ID3D11VertexShader* mVertexShader;
		ID3DBlob* mVertexShaderBuffer;
		ID3D11PixelShader* mPixelShader;
		ID3D11InputLayout* mInputLayout;

		float mTheta;
		float mPhi;
		float mRadius;

		POINT mLastMousePos;
	};
}