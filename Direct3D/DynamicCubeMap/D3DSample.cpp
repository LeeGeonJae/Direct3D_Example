#include <cassert>
#include <fstream>
#include <directxtk/DDSTextureLoader.h>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "Basic32.h"
#include "MathHelper.h"
#include "Sky.h"
#include "GeometryGenerator.h"
#include "RenderStates.h"

namespace dynamicCubeMap
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mBasic32(nullptr)
		, mLightCount(3)
	{
		mTitle = L"Dynamic CubeMap Demo";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		buildCubeFaceCamera(0.0f, 2, 0.0f);

		for (int i = 0; i < 6; ++i)
		{
			mDynamicCubeMapRTV[i] = 0;
		}

		Matrix I = XMMatrixIdentity();
		XMStoreFloat4x4(&mGridWorld, I);

		Matrix boxScale = XMMatrixScaling(3.0f, 1.0f, 3.0f);
		Matrix boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
		XMStoreFloat4x4(&mBoxWorld, XMMatrixMultiply(boxScale, boxOffset));

		Matrix centerSphereScale = XMMatrixScaling(2, 2, 2);
		Matrix centerSphereOffset = XMMatrixTranslation(0.0f, 2, 0.0f);
		XMStoreFloat4x4(&mCenterSphereWorld, XMMatrixMultiply(centerSphereScale, centerSphereOffset));

		for (int i = 0; i < 5; ++i)
		{
			XMStoreFloat4x4(&mCylWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
			XMStoreFloat4x4(&mCylWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));

			XMStoreFloat4x4(&mSphereWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
			XMStoreFloat4x4(&mSphereWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
		}

		mDirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Direction = Vector3(0.57735f, -0.57735f, 0.57735f);

		mDirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
		mDirLights[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
		mDirLights[1].Direction = Vector3(-0.57735f, -0.57735f, 0.57735f);

		mDirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Direction = Vector3(0.0f, -0.707f, -0.707f);

		mGridMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mGridMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mCylinderMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mCylinderMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mSphereMat.Ambient = XMFLOAT4(0.6f, 0.8f, 1.0f, 1.0f);
		mSphereMat.Diffuse = XMFLOAT4(0.6f, 0.8f, 1.0f, 1.0f);
		mSphereMat.Specular = XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);
		mSphereMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mBoxMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mBoxMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mSkullMat.Ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		mSkullMat.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mSkullMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mSkullMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mCenterSphereMat.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mCenterSphereMat.Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mCenterSphereMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mCenterSphereMat.Reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	}
	D3DSample::~D3DSample()
	{
		ReleaseCOM(mPerFrameCBPos);
		ReleaseCOM(mVertexShaderPos);
		ReleaseCOM(mPixelShaderPos);
		ReleaseCOM(mPosLayout);
		ReleaseCOM(mLinearSampler);

		SafeDelete(mBasic32);

		SafeDelete(mSky);

		ReleaseCOM(mShapesVB);
		ReleaseCOM(mShapesIB);
		ReleaseCOM(mSkullVB);
		ReleaseCOM(mSkullIB);
		ReleaseCOM(mFloorTexSRV);
		ReleaseCOM(mStoneTexSRV);
		ReleaseCOM(mBrickTexSRV);
		ReleaseCOM(mDynamicCubeMapDSV);
		ReleaseCOM(mDynamicCubeMapSRV);
		for (int i = 0; i < 6; ++i)
			ReleaseCOM(mDynamicCubeMapRTV[i]);

		RenderStates::Destroy();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		mBasic32 = new Basic32(md3dDevice, L"Basic.hlsl");
		mSky = new common::Sky(md3dDevice, L"../Resource/Textures/sunsetcube1024.dds", 5000.0f);

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/floor.dds", NULL, &mFloorTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/stone.dds", NULL, &mStoneTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/bricks.dds", NULL, &mBrickTexSRV));

		buildSky();
		buildDynamicCubeMapViews();

		buildShapeGeometryBuffers();
		buildSkullGeometryBuffers();

		RenderStates::Init(md3dDevice);

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
			mCam.TranslateLook(10.0f * deltaTime);

		if (GetAsyncKeyState('S') & 0x8000)
			mCam.TranslateLook(-10.0f * deltaTime);

		if (GetAsyncKeyState('A') & 0x8000)
			mCam.TranslateRight(-10.0f * deltaTime);

		if (GetAsyncKeyState('D') & 0x8000)
			mCam.TranslateRight(10.0f * deltaTime);


		//
		// Switch the number of lights based on key presses.
		//
		if (GetAsyncKeyState('0') & 0x8000)
			mLightCount = 0;

		if (GetAsyncKeyState('1') & 0x8000)
			mLightCount = 1;

		if (GetAsyncKeyState('2') & 0x8000)
			mLightCount = 2;

		if (GetAsyncKeyState('3') & 0x8000)
			mLightCount = 3;

		//
		// Animate the skull around the center sphere.
		//

		Matrix skullScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
		Matrix skullOffset = XMMatrixTranslation(3.0f, 2.0f, 0.0f);
		Matrix skullLocalRotate = XMMatrixRotationY(2.0f * mTimer.GetTotalTime());
		Matrix skullGlobalRotate = XMMatrixRotationY(0.5f * mTimer.GetTotalTime());
		XMStoreFloat4x4(&mSkullWorld, skullScale * skullLocalRotate * skullOffset * skullGlobalRotate);

		mCam.UpdateViewMatrix();
	}
	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		ID3D11RenderTargetView* renderTargets[1] = {};

		md3dContext->RSSetViewports(1, &mCubeMapViewport);

		for (int i = 0; i < 6; ++i)
		{
			md3dContext->ClearRenderTargetView(mDynamicCubeMapRTV[i], reinterpret_cast<const float*>(&Silver));
			md3dContext->ClearDepthStencilView(mDynamicCubeMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			renderTargets[0] = mDynamicCubeMapRTV[i];
			md3dContext->OMSetRenderTargets(1, renderTargets, mDynamicCubeMapDSV);

			drawScene(mCubeMapCamera[i], false); // 마지막 인수는 가운데 물체를 그릴지 말지
		}

		// 기존 뷰포트와 렌더 대상 바인딩
		md3dContext->RSSetViewports(1, &mScreenViewport);
		renderTargets[0] = mRenderTargetView;
		md3dContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView);
		md3dContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Silver));
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		// 하위 밉맵 수준 생성
		md3dContext->GenerateMips(mDynamicCubeMapSRV);

		drawScene(mCam, true);

		// 입방체 맵 바인딩 해제
		ID3D11ShaderResourceView* SRV = nullptr;
		md3dContext->PSSetShaderResources(1, 1, &SRV);

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

	void D3DSample::buildSky()
	{
		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		static_assert(sizeof(CBPerFramePos) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(CBPerFramePos);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerFrameCBPos));

		ID3DBlob* vertexShaderBufferPos = nullptr;
		HR(common::D3DHelper::CompileShaderFromFile(L"Sky.hlsl", "VS", "vs_5_0", &vertexShaderBufferPos));

		HR(md3dDevice->CreateVertexShader(
			vertexShaderBufferPos->GetBufferPointer(),
			vertexShaderBufferPos->GetBufferSize(),
			NULL,
			&mVertexShaderPos));

		ID3DBlob* pixelShaderBufferPos = nullptr;
		HR(common::D3DHelper::CompileShaderFromFile(L"Sky.hlsl", "PS", "ps_5_0", &pixelShaderBufferPos));

		HR(md3dDevice->CreatePixelShader(
			pixelShaderBufferPos->GetBufferPointer(),
			pixelShaderBufferPos->GetBufferSize(),
			NULL,
			&mPixelShaderPos));

		ReleaseCOM(pixelShaderBufferPos);

		const D3D11_INPUT_ELEMENT_DESC posDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		HR(md3dDevice->CreateInputLayout(
			posDesc,
			ARRAYSIZE(posDesc),
			vertexShaderBufferPos->GetBufferPointer(),
			vertexShaderBufferPos->GetBufferSize(),
			&mPosLayout
		));

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 비교를 통과하지 않는다?
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		md3dDevice->CreateSamplerState(&samplerDesc, &mLinearSampler);
	}

	void D3DSample::drawScene(const Camera& camera, bool bDrawCenterSphere)
	{
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT stride = sizeof(Basic32::Vertex);
		UINT offset = 0;

		Matrix view = camera.GetView();
		Matrix proj = camera.GetProj();
		Matrix viewProj = camera.GetViewProj();

		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		Basic32::PerFrame& perFrame = mBasic32->GetPerFrame();
		memcpy(perFrame.DirLights, mDirLights, sizeof(perFrame.DirLights));
		perFrame.EyePosW = camera.GetPosition();
		perFrame.LigthCount = mLightCount;
		perFrame.FogColor = Silver;
		perFrame.FogStart = 25.f;
		perFrame.FogRange = 200.f;
		perFrame.bUseFog = true;
		perFrame.bUseTexutre = true;
		perFrame.bUseLight = true;

		Matrix world;
		Matrix worldInvTranspose;
		Matrix worldViewProj;

		md3dContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);
		world = mSkullWorld;
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		perFrame.bUseTexutre = false;
		Basic32::PerObject& perObject = mBasic32->GetPerObject();
		perObject.World = world.Transpose();
		perObject.WorldInvTranspose = worldInvTranspose.Transpose();
		perObject.WorldViewProj = worldViewProj.Transpose();
		perObject.Tex = Matrix::Identity;
		perObject.Material = mSkullMat;

		mBasic32->UpdateSubresource(md3dContext);
		mBasic32->Bind(md3dContext);
		md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);

		md3dContext->IASetVertexBuffers(0, 1, &mShapesVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mShapesIB, DXGI_FORMAT_R32_UINT, 0);
		world = mGridWorld;
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;
		perFrame.bUseTexutre = true;
		perObject.World = world.Transpose();
		perObject.WorldInvTranspose = worldInvTranspose.Transpose();
		perObject.WorldViewProj = worldViewProj.Transpose();
		perObject.Tex = Matrix::CreateScale(6.0f, 8.0f, 1.0f);
		perObject.Material = mGridMat;
		mBasic32->UpdateSubresource(md3dContext);
		mBasic32->Bind(md3dContext, mFloorTexSRV);
		md3dContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		world = mBoxWorld;
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;
		perObject.World = world.Transpose();
		perObject.WorldInvTranspose = worldInvTranspose.Transpose();
		perObject.WorldViewProj = worldViewProj.Transpose();
		perObject.Tex = Matrix::Identity;
		perObject.Material = mBoxMat;

		mBasic32->UpdateSubresource(md3dContext);
		mBasic32->Bind(md3dContext, mStoneTexSRV);
		md3dContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			world = mCylWorld[i];
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;
			perObject.World = world.Transpose();
			perObject.WorldInvTranspose = worldInvTranspose.Transpose();
			perObject.WorldViewProj = worldViewProj.Transpose();
			perObject.Tex = Matrix::Identity;
			perObject.Material = mCylinderMat;

			mBasic32->UpdateSubresource(md3dContext);
			mBasic32->Bind(md3dContext, mBrickTexSRV);
			md3dContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		// Draw the spheres.
		for (int i = 0; i < 10; ++i)
		{
			world = mSphereWorld[i];
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;
			perObject.World = world.Transpose();
			perObject.WorldInvTranspose = worldInvTranspose.Transpose();
			perObject.WorldViewProj = worldViewProj.Transpose();
			perObject.Tex = Matrix::Identity;
			perObject.Material = mSphereMat;

			mBasic32->UpdateSubresource(md3dContext);
			mBasic32->Bind(md3dContext, mStoneTexSRV);
			md3dContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

		if (bDrawCenterSphere)
		{
			world = mCenterSphereWorld;
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			perObject.World = world.Transpose();
			perObject.WorldInvTranspose = worldInvTranspose.Transpose();
			perObject.WorldViewProj = worldViewProj.Transpose();
			perObject.Tex = Matrix::Identity;
			perObject.Material = mCenterSphereMat;

			perFrame.bUseReflection = true;

			md3dContext->PSSetShaderResources(1, 1, &mDynamicCubeMapSRV);
			mBasic32->UpdateSubresource(md3dContext);
			mBasic32->Bind(md3dContext, mStoneTexSRV);
			md3dContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);

			perFrame.bUseReflection = false;
		}

		// 바인딩
		md3dContext->VSSetShader(mVertexShaderPos, 0, 0);
		md3dContext->VSSetConstantBuffers(0, 1, &mPerFrameCBPos);

		md3dContext->PSSetShader(mPixelShaderPos, 0, 0);
		md3dContext->PSSetConstantBuffers(0, 1, &mPerFrameCBPos);
		md3dContext->PSSetSamplers(0, 1, &mLinearSampler);

		// 버퍼 갱신
		Vector3 eyePos = camera.GetPosition();
		Matrix T = Matrix::CreateTranslation(eyePos);
		Matrix WVP = T * camera.GetViewProj();
		mCBPerFramePos.WorldViewProj = WVP.Transpose();
		md3dContext->UpdateSubresource(mPerFrameCBPos, 0, 0, &mCBPerFramePos, 0, 0);

		md3dContext->RSSetState(RenderStates::NoCullRS);
		md3dContext->OMSetDepthStencilState(RenderStates::LessEqualDSS, 0);
		mSky->Draw(md3dContext, camera);
		md3dContext->RSSetState(0);
		md3dContext->OMSetDepthStencilState(0, 0);
	}

	void D3DSample::buildCubeFaceCamera(float x, float y, float z)
	{
		// 카메라 정보를 생성한다
		Vector3 center(x, y, z);

		Vector3 targets[6] =
		{
			Vector3(x + 1.0f, y, z), // +X
			Vector3(x - 1.0f, y, z), // -X
			Vector3(x, y + 1.0f, z), // +Y
			Vector3(x, y - 1.0f, z), // -Y
			Vector3(x, y, z + 1.0f), // +Z
			Vector3(x, y, z - 1.0f)  // -Z
		};

		Vector3 ups[6] =
		{
			Vector3(0.0f, 1.0f, 0.0f),  // +X
			Vector3(0.0f, 1.0f, 0.0f),  // -X
			Vector3(0.0f, 0.0f, -1.0f), // +Y
			Vector3(0.0f, 0.0f, +1.0f), // -Y
			Vector3(0.0f, 1.0f, 0.0f),	 // +Z
			Vector3(0.0f, 1.0f, 0.0f)	 // -Z
		};

		for (int i = 0; i < 6; ++i)
		{
			mCubeMapCamera[i].LookAt(center, targets[i], ups[i]);
			mCubeMapCamera[i].SetLens(0.5f * XM_PI, 1.0f, 0.1f, 1000.0f);
			mCubeMapCamera[i].UpdateViewMatrix();
		}
	}

	void D3DSample::buildDynamicCubeMapViews()
	{
		//
		// Cubemap is a special texture array with 6 elements.
		//

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = CubeMapSize; // 256
		texDesc.Height = CubeMapSize; // 256
		texDesc.MipLevels = 0; // 0은 전체 서브 텍스처, 1은 멀티샘플링 텍스처?
		texDesc.ArraySize = 6; // 배열 길이
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE; 
		// D3D11_RESOURCE_MISC_GENERATE_MIPS GenerateMips 메서드가 작동하기 위한 플래그

		ID3D11Texture2D* cubeTex = 0;
		HR(md3dDevice->CreateTexture2D(&texDesc, 0, &cubeTex));

		//
		// Create a render target view to each cube map face 
		// (i.e., each element in the texture array).
		// 

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = texDesc.Format; // 텍스처 포맷과 동일하게
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY; // 2차원 텍스처 배열 렌더타겟뷰
		rtvDesc.Texture2DArray.ArraySize = 1; // 사용할 텍스처의 수
		rtvDesc.Texture2DArray.MipSlice = 0; // 밉맵 수준

		for (int i = 0; i < 6; ++i)
		{
			rtvDesc.Texture2DArray.FirstArraySlice = i; // 텍스처 배열의 인덱스
			HR(md3dDevice->CreateRenderTargetView(cubeTex, &rtvDesc, &mDynamicCubeMapRTV[i]));
		}

		//
		// Create a shader resource view to the cube map.
		//

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0; // 가장 자세한 밉맵 수준
		srvDesc.TextureCube.MipLevels = -1; // 최대 밉맵 수, -1은 모든 밉맵 레벨 표시

		HR(md3dDevice->CreateShaderResourceView(cubeTex, &srvDesc, &mDynamicCubeMapSRV));

		ReleaseCOM(cubeTex);

		//
		// We need a depth texture for rendering the scene into the cubemap
		// that has the same resolution as the cubemap faces.  
		//

		// 입방체맵 해상도로 깊이버퍼 생성
		D3D11_TEXTURE2D_DESC depthTexDesc;
		depthTexDesc.Width = CubeMapSize;
		depthTexDesc.Height = CubeMapSize;
		depthTexDesc.MipLevels = 1; 
		depthTexDesc.ArraySize = 1;
		depthTexDesc.SampleDesc.Count = 1; // 멀티 샘플링 안함
		depthTexDesc.SampleDesc.Quality = 0;
		depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT; // 스텐실을 안씀
		depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
		depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthTexDesc.CPUAccessFlags = 0;
		depthTexDesc.MiscFlags = 0;

		ID3D11Texture2D* depthTex = 0;
		HR(md3dDevice->CreateTexture2D(&depthTexDesc, 0, &depthTex));

		// 어차피 null로 지정하면 동일한데 왜 이렇게 직접만든걸까
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Format = depthTexDesc.Format;
		dsvDesc.Flags = 0;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		HR(md3dDevice->CreateDepthStencilView(depthTex, &dsvDesc, &mDynamicCubeMapDSV)); 

		ReleaseCOM(depthTex);

		// 입방체맵 해상도로 뷰포트 설정
		mCubeMapViewport.TopLeftX = 0.0f;
		mCubeMapViewport.TopLeftY = 0.0f;
		mCubeMapViewport.Width = (float)CubeMapSize;
		mCubeMapViewport.Height = (float)CubeMapSize;
		mCubeMapViewport.MinDepth = 0.0f;
		mCubeMapViewport.MaxDepth = 1.0f;
	}
	void D3DSample::buildShapeGeometryBuffers()
	{
		GeometryGenerator::MeshData box;
		GeometryGenerator::MeshData grid;
		GeometryGenerator::MeshData sphere;
		GeometryGenerator::MeshData cylinder;

		GeometryGenerator::CreateBox(1.0f, 1.0f, 1.0f, &box);
		GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40, &grid);
		GeometryGenerator::CreateSphere(0.5f, 50, 50, &sphere);
		GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, &cylinder);

		// Cache the vertex offsets to each object in the concatenated vertex buffer.
		mBoxVertexOffset = 0;
		mGridVertexOffset = box.Vertices.size();
		mSphereVertexOffset = mGridVertexOffset + grid.Vertices.size();
		mCylinderVertexOffset = mSphereVertexOffset + sphere.Vertices.size();

		// Cache the index count of each object.
		mBoxIndexCount = box.Indices.size();
		mGridIndexCount = grid.Indices.size();
		mSphereIndexCount = sphere.Indices.size();
		mCylinderIndexCount = cylinder.Indices.size();

		// Cache the starting index for each object in the concatenated index buffer.
		mBoxIndexOffset = 0;
		mGridIndexOffset = mBoxIndexCount;
		mSphereIndexOffset = mGridIndexOffset + mGridIndexCount;
		mCylinderIndexOffset = mSphereIndexOffset + mSphereIndexCount;

		UINT totalVertexCount =
			box.Vertices.size() +
			grid.Vertices.size() +
			sphere.Vertices.size() +
			cylinder.Vertices.size();

		UINT totalIndexCount =
			mBoxIndexCount +
			mGridIndexCount +
			mSphereIndexCount +
			mCylinderIndexCount;



		std::vector<Basic32::Vertex> vertices(totalVertexCount);

		UINT k = 0;
		for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = box.Vertices[i].Position;
			vertices[k].Normal = box.Vertices[i].Normal;
			vertices[k].Tex = box.Vertices[i].TexC;
		}

		for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = grid.Vertices[i].Position;
			vertices[k].Normal = grid.Vertices[i].Normal;
			vertices[k].Tex = grid.Vertices[i].TexC;
		}

		for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = sphere.Vertices[i].Position;
			vertices[k].Normal = sphere.Vertices[i].Normal;
			vertices[k].Tex = sphere.Vertices[i].TexC;
		}

		for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = cylinder.Vertices[i].Position;
			vertices[k].Normal = cylinder.Vertices[i].Normal;
			vertices[k].Tex = cylinder.Vertices[i].TexC;
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32::Vertex) * totalVertexCount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapesVB));

		std::vector<UINT> indices;
		indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
		indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
		indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
		indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapesIB));
	}
	void D3DSample::buildSkullGeometryBuffers()
	{
		std::ifstream fin("../Resource/Models/skull.txt");

		if (!fin)
		{
			MessageBox(0, L"Models/skull.txt not found.", 0, 0);
			return;
		}

		UINT vcount = 0;
		UINT tcount = 0;
		std::string ignore;

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		std::vector<Basic32::Vertex> vertices(vcount);
		for (UINT i = 0; i < vcount; ++i)
		{
			fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
			fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
		}

		fin >> ignore;
		fin >> ignore;
		fin >> ignore;

		mSkullIndexCount = 3 * tcount;
		std::vector<UINT> indices(mSkullIndexCount);
		for (UINT i = 0; i < tcount; ++i)
		{
			fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
		}

		fin.close();

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32::Vertex) * vcount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * mSkullIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
	}
}