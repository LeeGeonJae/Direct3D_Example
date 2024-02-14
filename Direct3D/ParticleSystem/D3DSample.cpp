#include <cassert>
#include <directxtk/DDSTextureLoader.h>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "Basic32.h"
#include "MathHelper.h"
#include "RenderStates.h"

namespace particleSystem
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mBasic32(nullptr)
	{
		mTitle = L"Particles Demo";
		mEnable4xMsaa = false;

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, -50.0f);

		mDirLights[0].Ambient = Vector4(0.3f, 0.3f, 0.3f, 1.0f);
		mDirLights[0].Diffuse = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		mDirLights[0].Specular = Vector4(0.8f, 0.8f, 0.7f, 1.0f);
		mDirLights[0].Direction = Vector3(0.707f, -0.707f, 0.0f);

		mDirLights[1].Ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[1].Specular = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[1].Direction = Vector3(0.57735f, -0.57735f, 0.57735f);

		mDirLights[2].Ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Direction = Vector3(-0.57735f, -0.57735f, -0.57735f);
	}
	D3DSample::~D3DSample()
	{
		SafeDelete(mBasic32);
		SafeDelete(mSky);

		md3dContext->ClearState();

		ReleaseCOM(mRandomTexSRV);
		ReleaseCOM(mFlareTexSRV);
		ReleaseCOM(mRainTexSRV);

		RenderStates::Destroy();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		mBasic32 = new Basic32(md3dDevice, L"Basic.hlsl");
		RenderStates::Init(md3dDevice);

		mSky = new Sky(md3dDevice, L"../Resource/Textures/grasscube1024.dds", 5000.0f);

		Terrain::InitInfo tii;
		tii.HeightMapFilename = L"../Resource/Textures/terrain.raw";
		tii.LayerMapFilename0 = L"../Resource/Textures/grass.dds";
		tii.LayerMapFilename1 = L"../Resource/Textures/darkdirt.dds";
		tii.LayerMapFilename2 = L"../Resource/Textures/stone.dds";
		tii.LayerMapFilename3 = L"../Resource/Textures/lightdirt.dds";
		tii.LayerMapFilename4 = L"../Resource/Textures/snow.dds";
		tii.BlendMapFilename = L"../Resource/Textures/blend.dds";
		tii.HeightScale = 50.0f;
		tii.HeightmapWidth = 2049;
		tii.HeightmapHeight = 2049;
		tii.CellSpacing = 0.5f;

		mTerrain.Init(md3dDevice, md3dContext, tii);

		mRandomTexSRV = D3DHelper::CreateRandomTexture1DSRV(md3dDevice);

		std::vector<std::wstring> flares;
		flares.push_back(L"..\\Resource\\Textures\\flare0.dds");
		mFlareTexSRV = D3DHelper::CreateTexture2DArraySRV(md3dDevice, md3dContext, flares);
		mFire.SetEmitPos(Vector3(0.0f, 1.0f, 120.0f));
		mFire.SetEmitDir(Vector3(100.f, 0.f, 0.f));

		mFire.Init(md3dDevice, L"Fire.hlsl", mFlareTexSRV, mRandomTexSRV, 500);

		std::vector<std::wstring> raindrops;
		raindrops.push_back(L"..\\Resource\\Textures\\raindrop.dds");
		mRainTexSRV = D3DHelper::CreateTexture2DArraySRV(md3dDevice, md3dContext, raindrops);
		mRain.SetEmitPos(Vector3(0.0f, 50.0f, 120.0f));

		mRain.Init(md3dDevice, L"Rain.hlsl", mRainTexSRV, mRandomTexSRV, 100000);

		return true;
	}
	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mCam.SetLens(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 10000.0f);
	}
	void D3DSample::Update(float deltaTime)
	{
		if (GetAsyncKeyState('W') & 0x8000)
			mCam.TranslateLook(100.0f * deltaTime);

		if (GetAsyncKeyState('S') & 0x8000)
			mCam.TranslateLook(-100.0f * deltaTime);

		if (GetAsyncKeyState('A') & 0x8000)
			mCam.TranslateRight(-100.0f * deltaTime);

		if (GetAsyncKeyState('D') & 0x8000)
			mCam.TranslateRight(100.0f * deltaTime);

		if (GetAsyncKeyState('R') & 0x8000)
		{
			mFire.Reset();
			mRain.Reset();
		}

		static float s = 0.0f;
		float total = mTimer.GetTotalTime();

		mFire.Update(deltaTime, mTimer.GetTotalTime());
		mRain.Update(deltaTime, mTimer.GetTotalTime());

		mCam.UpdateViewMatrix();
	}
	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		float color[] = { 1.f, 0.f, 0.f, 1.0f };

		md3dContext->ClearRenderTargetView(mRenderTargetView, Black);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		mBasic32->Bind(md3dContext);
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		if (GetAsyncKeyState('1') & 0x8000)
			md3dContext->RSSetState(RenderStates::WireFrameRS);

		// mTerrain.Draw(md3dContext, mCam, mDirLights);

		md3dContext->RSSetState(0);

		//mSky->Draw(md3dContext, mCam);

		// Draw particle systems last so it is blended with scene.
		mFire.SetEyePos(mCam.GetPosition());
		mFire.Draw(md3dContext, mCam);
		md3dContext->OMSetBlendState(0, blendFactor, 0xffffffff); // restore default

		mRain.SetEyePos(mCam.GetPosition());
		mRain.Draw(md3dContext, mCam);
		md3dContext->RSSetState(0);
		md3dContext->OMSetDepthStencilState(0, 0);
		md3dContext->OMSetBlendState(0, blendFactor, 0xffffffff);

		HR(mSwapChain->Present(0, 0));
	}
	void D3DSample::OnMouseDown(WPARAM btnState, int x, int y)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		SetCapture(mhWnd);
	}
	void D3DSample::OnMouseUp(WPARAM btnState, int x, int y)
	{
		ReleaseCapture();
	}
	void D3DSample::OnMouseMove(WPARAM btnState, int x, int y)
	{
		if ((btnState & MK_LBUTTON) != 0)
		{
			float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.RotatePitch(dy);
			mCam.RotateAxis({ 0, 1, 0 }, dx);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}
}