#include <cassert>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "Basic32.h"
#include "MathHelper.h"
#include "RenderStates.h"

namespace terrain
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mBasic32(nullptr)
		, mWalkCamMode(false)
	{
		mTitle = L"Terrain Demo";
		mEnable4xMsaa = false;

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, 100.0f);

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
		md3dContext->ClearState();

		SafeDelete(mSky);

		RenderStates::Destroy();
		SafeDelete(mBasic32);
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

		return true;
	}
	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mCam.SetLens(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 10000.0f);
	}
	void D3DSample::Update(float deltaTime)
	{
		const float MOVE_SPEED = 100.f;

		if (GetAsyncKeyState('W') & 0x8000)
			mCam.TranslateLook(MOVE_SPEED * deltaTime);

		if (GetAsyncKeyState('S') & 0x8000)
			mCam.TranslateLook(-MOVE_SPEED * deltaTime);

		if (GetAsyncKeyState('A') & 0x8000)
			mCam.TranslateRight(-MOVE_SPEED * deltaTime);

		if (GetAsyncKeyState('D') & 0x8000)
			mCam.TranslateRight(MOVE_SPEED * deltaTime);

		if (GetAsyncKeyState('2') & 0x8000)
			mWalkCamMode = true;
		if (GetAsyncKeyState('3') & 0x8000)
			mWalkCamMode = false;

		{
			Vector3 camPos = mCam.GetPosition();
			float y = mTerrain.GetHeight(camPos.x, camPos.z);
			mCam.SetPosition(camPos.x, y + 20.0f, camPos.z);
		}

		mCam.UpdateViewMatrix();
	}
	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };

		md3dContext->ClearRenderTargetView(mRenderTargetView, color);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		md3dContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Silver));
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		//mBasic32->Bind(md3dContext);
		//md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		if (GetAsyncKeyState('1') & 0x8000)
			md3dContext->RSSetState(RenderStates::WireFrameRS);

		mTerrain.Draw(md3dContext, mCam, mDirLights);

		md3dContext->RSSetState(0);

		// mSky->Draw(md3dContext, mCam);

		// restore default states, as the SkyFX changes them in the effect file.
		md3dContext->RSSetState(0);
		md3dContext->OMSetDepthStencilState(0, 0);

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
			// Make each pixel correspond to a quarter of a degree.
			float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.RotatePitch(dy);
			mCam.RotateAxis({ 0, 1, 0 }, dx);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}
}