#pragma once

#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "Camera.h"
#include "Sky.h"
#include "LightHelper.h"
#include "Terrain.h"
#include "ParticleSystem.h"

namespace particleSystem
{
	class Basic32;

	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

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
		Basic32* mBasic32;
		Camera mCam;
		POINT mLastMousePos;

		Sky* mSky;
		Terrain mTerrain;

		ID3D11ShaderResourceView* mFlareTexSRV;
		ID3D11ShaderResourceView* mRainTexSRV;
		ID3D11ShaderResourceView* mRandomTexSRV;

		ParticleSystem mFire;
		ParticleSystem mRain;

		DirectionLight mDirLights[3];
	};
}