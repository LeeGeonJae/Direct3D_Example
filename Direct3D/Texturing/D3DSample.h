#pragma once

#include <directxtk/simpleMath.h>

#include "D3dProcessor.h"
#include "LightHelper.h"
#include "Waves.h"

namespace texturing
{
	using namespace common;
	using namespace DirectX::SimpleMath;

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
		float unused2[1];
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

		void buildGeometryBuffers();
		void buildShader();
		void buildVertexLayout();
		void buildConstantBuffer();

		float getHillHeight(float x, float z) const;
		Vector3 getHillNormal(float x, float z) const;
		void buildLandGeometryBuffers();
		void buildWaveGeometryBuffers();

	private:
		ID3D11Buffer* mPerObjectCB;
		ID3D11Buffer* mPerFrameCB;
		CBPerObject mCBPerObject;
		CBPerFrame mCBPerFrame;

		ID3D11Buffer* mBoxVB;
		ID3D11Buffer* mBoxIB;

		ID3D11Buffer* mLandVB;
		ID3D11Buffer* mLandIB;

		ID3D11Buffer* mWavesVB;
		ID3D11Buffer* mWavesIB;

		Waves mWaves;

		ID3D11ShaderResourceView* mGrassMapSRV;
		ID3D11ShaderResourceView* mWavesMapSRV;
		ID3D11ShaderResourceView* mDiffuseMapSRV;
		ID3D11SamplerState* mLinearSampleState;

		DirectionLight mDirLights[3];
		Material mBoxMat;
		Material mWavesMat;
		Material mLandMat;

		Matrix mTexTransform;
		Matrix mBoxWorld;
		Matrix mGrassTexTransform;
		Matrix mWaterTexTransform;
		Matrix mLandWorld;
		Matrix mWavesWorld;
		Matrix mView;
		Matrix mProj;

		int mBoxVertexOffset;
		UINT mBoxIndexOffset;
		UINT mBoxIndexCount;
		UINT mLandIndexCount;
		Vector2 mWaterTexOffset;

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