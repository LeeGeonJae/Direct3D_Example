#include <cassert>
#include <fstream>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "Basic32.h"
#include "MathHelper.h"
#include "GeometryGenerator.h"
#include "RenderStates.h"
#include "ShadowMap.h"

namespace shadows
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mBasic32(nullptr)
		, mLightRotationAngle(0.f)
	{
		mTitle = L"Shadows Demo";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
		mSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);

		XMMATRIX I = XMMatrixIdentity();
		XMStoreFloat4x4(&mGridWorld, I);

		XMMATRIX boxScale = XMMatrixScaling(3.0f, 1.0f, 3.0f);
		XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
		XMStoreFloat4x4(&mBoxWorld, XMMatrixMultiply(boxScale, boxOffset));

		XMMATRIX skullScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
		XMMATRIX skullOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
		XMStoreFloat4x4(&mSkullWorld, XMMatrixMultiply(skullScale, skullOffset));

		for (int i = 0; i < 5; ++i)
		{
			XMStoreFloat4x4(&mCylWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
			XMStoreFloat4x4(&mCylWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));

			XMStoreFloat4x4(&mSphereWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
			XMStoreFloat4x4(&mSphereWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
		}

		mDirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[0].Diffuse = XMFLOAT4(0.7f, 0.7f, 0.6f, 1.0f);
		mDirLights[0].Specular = XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f);
		mDirLights[0].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

		// Shadow acne gets worse as we increase the slope of the polygon (from the
		// perspective of the light).
		//mDirLights[0].Direction = XMFLOAT3(5.0f/sqrtf(50.0f), -5.0f/sqrtf(50.0f), 0.0f);
		//mDirLights[0].Direction = XMFLOAT3(10.0f/sqrtf(125.0f), -5.0f/sqrtf(125.0f), 0.0f);
		//mDirLights[0].Direction = XMFLOAT3(10.0f/sqrtf(116.0f), -4.0f/sqrtf(116.0f), 0.0f);
		//mDirLights[0].Direction = XMFLOAT3(10.0f/sqrtf(109.0f), -3.0f/sqrtf(109.0f), 0.0f);

		mDirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = XMFLOAT4(0.40f, 0.40f, 0.40f, 1.0f);
		mDirLights[1].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[1].Direction = XMFLOAT3(0.707f, -0.707f, 0.0f);

		mDirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Direction = XMFLOAT3(0.0f, 0.0, -1.0f);

		mOriginalLightDir[0] = mDirLights[0].Direction;
		mOriginalLightDir[1] = mDirLights[1].Direction;
		mOriginalLightDir[2] = mDirLights[2].Direction;

		mGridMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mGridMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
		mGridMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mCylinderMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 32.0f);
		mCylinderMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mSphereMat.Ambient = XMFLOAT4(0.2f, 0.3f, 0.4f, 1.0f);
		mSphereMat.Diffuse = XMFLOAT4(0.2f, 0.3f, 0.4f, 1.0f);
		mSphereMat.Specular = XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);
		mSphereMat.Reflect = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);

		mBoxMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mBoxMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mSkullMat.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mSkullMat.Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mSkullMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mSkullMat.Reflect = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	}
	D3DSample::~D3DSample()
	{
		ReleaseCOM(mObjectCB);
		ReleaseCOM(mFrameCB);
		ReleaseCOM(mFrameShadowCB);
		ReleaseCOM(mInputLayout);
		ReleaseCOM(mVSShader);
		ReleaseCOM(mPSShader);
		ReleaseCOM(mSamShadow);

		SafeDelete(mBasic32);
		SafeDelete(mSmap);

		ReleaseCOM(mShapesVB);
		ReleaseCOM(mShapesIB);
		ReleaseCOM(mSkullVB);
		ReleaseCOM(mSkullIB);
		ReleaseCOM(mScreenQuadVB);
		ReleaseCOM(mScreenQuadIB);
		ReleaseCOM(mStoneTexSRV);
		ReleaseCOM(mBrickTexSRV);
		ReleaseCOM(mStoneNormalTexSRV);
		ReleaseCOM(mBrickNormalTexSRV);

		RenderStates::Destroy();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		mBasic32 = new Basic32(md3dDevice, L"Basic.hlsl");
		// Must init Effects first since InputLayouts depend on shader signatures.
		RenderStates::Init(md3dDevice);

		mSmap = new ShadowMap(md3dDevice, SMapSize, SMapSize);

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice,
			L"../Resource/Textures/floor.dds", 0, &mStoneTexSRV));

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice,
			L"../Resource/Textures/bricks.dds", 0, &mBrickTexSRV));

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice,
			L"../Resource/Textures/floor_nmap.dds", 0, &mStoneNormalTexSRV));

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice,
			L"../Resource/Textures/bricks_nmap.dds", 0, &mBrickNormalTexSRV));

		buildInit();
		buildShapeGeometryBuffers();
		buildSkullGeometryBuffers();
		buildScreenQuadGeometryBuffers();

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

		// Animate the lights (and hence shadows).

		static float s_time = 0.f;
		s_time += 0.001f;
		mLightRotationAngle += deltaTime; // 0.1f * deltaTime;

		XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
		for (int i = 0; i < 3; ++i)
		{
			XMVECTOR lightDir = XMLoadFloat3(&mOriginalLightDir[i]);
			lightDir = XMVector3TransformNormal(lightDir, R);
			XMStoreFloat3(&mDirLights[i].Direction, lightDir);
		}

		buildShadowTransform();

		mCam.UpdateViewMatrix();
	}
	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);
		ID3D11ShaderResourceView* nullSRV[16] = { 0 };

		// 같이 쓰는 거 바인딩
		auto* linear = mBasic32->GetSamplerState();
		md3dContext->VSSetSamplers(0, 1, &linear);
		md3dContext->VSSetSamplers(1, 1, &mSamShadow);

		md3dContext->PSSetSamplers(0, 1, &linear);
		md3dContext->PSSetSamplers(1, 1, &mSamShadow);

		md3dContext->VSSetConstantBuffers(0, 1, &mObjectCB);
		md3dContext->PSSetConstantBuffers(0, 1, &mObjectCB);

		// 그림자 그리기
		md3dContext->PSSetShaderResources(1, 1, nullSRV);
		mSmap->BindDsvAndSetNullRenderTarget(md3dContext);
		drawSceneToShadowMap();
		md3dContext->RSSetState(0);


		// 그리기 자원 바인딩
		mPerFrame.bUseFog = true;
		mPerFrame.bUseLight = true;
		mPerFrame.bUseTexure = true;
		mPerFrame.LightCount = 3;
		mPerFrame.FogColor = Silver;
		mPerFrame.FogRange = 175.f;
		mPerFrame.FogStart = 25.f;

		md3dContext->VSSetConstantBuffers(1, 1, &mFrameCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mFrameCB);

		md3dContext->VSSetShader(mBasic32->GetVS(), 0, 0);
		md3dContext->PSSetShader(mBasic32->GetPS(), 0, 0);

		// Restore the back and depth buffer to the OM stage.
		ID3D11RenderTargetView* renderTargets[1] = { mRenderTargetView };
		md3dContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView);
		md3dContext->RSSetViewports(1, &mScreenViewport);

		md3dContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Silver));
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		UINT stride = sizeof(PosNormalTexTan);
		UINT offset = 0;

		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dContext->IASetInputLayout(mInputLayout);
		md3dContext->IASetVertexBuffers(0, 1, &mShapesVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mShapesIB, DXGI_FORMAT_R32_UINT, 0);

		auto* shadowSRV = mSmap->DepthMapSRV();
		md3dContext->PSSetShaderResources(1, 1, &shadowSRV);

		XMMATRIX view = mCam.GetView();
		XMMATRIX proj = mCam.GetProj();
		XMMATRIX viewProj = mCam.GetViewProj();

		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		// Set per frame constants.
		memcpy(mPerFrame.DirLights, mDirLights, sizeof(mPerFrame.DirLights));
		mPerFrame.EyePosW = mCam.GetPosition();

		Matrix world;
		Matrix worldInvTranspose;
		Matrix worldViewProj;
		Matrix shadowTransform = XMLoadFloat4x4(&mShadowTransform);


		if (GetAsyncKeyState('1') & 0x8000)
			md3dContext->RSSetState(RenderStates::WireFrameRS);

		// Draw the grid.
		world = XMLoadFloat4x4(&mGridWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		mPerObject.World = world.Transpose();
		mPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
		mPerObject.WorldViewProj = worldViewProj.Transpose();
		mPerObject.ShadowTransform = (world * shadowTransform).Transpose();
		mPerObject.Tex = Matrix::CreateScale(8.0f, 10.0f, 1.0f).Transpose();
		mPerObject.Material = mGridMat;
		md3dContext->PSSetShaderResources(0, 1, &mStoneTexSRV);

		md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
		md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);
		md3dContext->PSSetSamplers(1, 1, &mSamShadow);

		md3dContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		// Draw the box.
		world = XMLoadFloat4x4(&mBoxWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		mPerObject.World = world.Transpose();
		mPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
		mPerObject.WorldViewProj = worldViewProj.Transpose();
		mPerObject.ShadowTransform = (world * shadowTransform).Transpose();
		mPerObject.Tex = Matrix::CreateScale(2.0f, 1.0f, 1.0f).Transpose();
		mPerObject.Material = mBoxMat;
		md3dContext->PSSetShaderResources(0, 1, &mBrickTexSRV);

		md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
		md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);
		md3dContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&mCylWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			mPerObject.World = world.Transpose();
			mPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
			mPerObject.WorldViewProj = worldViewProj.Transpose();
			mPerObject.ShadowTransform = (world * shadowTransform).Transpose();
			mPerObject.Tex = Matrix::CreateScale(1.0f, 2.0f, 1.0f).Transpose();
			mPerObject.Material = mCylinderMat;
			md3dContext->PSSetShaderResources(0, 1, &mBrickTexSRV);

			md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
			md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);
			md3dContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//
		// Draw the spheres with cubemap reflection.
		//

		// Draw the spheres.

		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&mSphereWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			mPerObject.World = world.Transpose();
			mPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
			mPerObject.WorldViewProj = worldViewProj.Transpose();
			mPerObject.ShadowTransform = (world * shadowTransform).Transpose();
			mPerObject.Tex = Matrix::Identity;
			mPerObject.Material = mSphereMat;

			mPerFrame.bUseTexure = false;
			md3dContext->PSSetShaderResources(0, 1, nullSRV);

			md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
			md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);
			md3dContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

		stride = sizeof(Basic32::Vertex);
		offset = 0;

		md3dContext->RSSetState(0);

		md3dContext->IASetInputLayout(mBasic32->GetInputLayout());
		md3dContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

		// Draw the skull.
		world = XMLoadFloat4x4(&mSkullWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		mPerObject.World = world.Transpose();
		mPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
		mPerObject.WorldViewProj = worldViewProj.Transpose();
		mPerObject.ShadowTransform = (world * shadowTransform).Transpose();
		mPerObject.Tex = Matrix::Identity;
		mPerObject.Material = mSkullMat;
		md3dContext->PSSetShaderResources(0, 1, nullSRV);

		md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
		md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);
		md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);

		// Debug view depth buffer.
		if (GetAsyncKeyState('Z') & 0x8000)
		{
			drawScreenQuad();
		}

		// restore default states, as the SkyFX changes them in the effect file.
		md3dContext->RSSetState(0);
		md3dContext->OMSetDepthStencilState(0, 0);

		// Unbind shadow map as a shader input because we are going to render to it next frame.
		// The shadow might might be at any slot, so clear all slots.
		md3dContext->PSSetShaderResources(0, 1, nullSRV);

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

	void D3DSample::buildInit()
	{
		// 입력 레이아웃
		const D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		// 쉐이더 컴파일
		ID3D10Blob* blob;
		HR(D3DHelper::CompileShaderFromFile(L"BuildShadowMap.hlsl", "VS", "vs_5_0", &blob));
		HR(md3dDevice->CreateVertexShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mVSShader
		));
		HR(md3dDevice->CreateInputLayout(
			inputLayoutDesc,
			ARRAYSIZE(inputLayoutDesc),
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			&mInputLayout
		));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(L"BuildShadowMap.hlsl", "PS", "ps_5_0", &blob));
		HR(md3dDevice->CreatePixelShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mPSShader
		));
		ReleaseCOM(blob);

		// 샘플 스테이트
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		samplerDesc.MaxAnisotropy = 1u;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		// samplerDesc.BorderColor = { 0, 0, 0, 0 };

		md3dDevice->CreateSamplerState(&samplerDesc, &mSamShadow);
		md3dContext->PSSetSamplers(1, 1, &mSamShadow);

		// 상수 버퍼
		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		static_assert(sizeof(PerObject) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerObject);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mObjectCB));

		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		static_assert(sizeof(PerFrame) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerFrame);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mFrameCB));

		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		static_assert(sizeof(PerFrameShadow) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerFrameShadow);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mFrameShadowCB));
	}

	void D3DSample::drawSceneToShadowMap()
	{
		md3dContext->VSSetConstantBuffers(1, 1, &mFrameShadowCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mFrameShadowCB);
		md3dContext->VSSetShader(mVSShader, 0, 0);
		md3dContext->PSSetShader(NULL, 0, 0);
		md3dContext->RSSetState(RenderStates::DepthRS);

		Matrix view = XMLoadFloat4x4(&mLightView);
		Matrix proj = XMLoadFloat4x4(&mLightProj);
		Matrix viewProj = XMMatrixMultiply(view, proj);

		mPerFrameShadow.EyePosW = mCam.GetPosition();
		mPerObject.ViewProj = viewProj.Transpose();
		md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
		md3dContext->UpdateSubresource(mFrameShadowCB, 0, 0, &mPerFrameShadow, 0, 0);

		Matrix world;
		Matrix worldInvTranspose;
		Matrix worldViewProj;

		//
		// Draw the grid, cylinders, and box without any cubemap reflection.
		// 

		UINT stride = sizeof(Basic32::Vertex);
		UINT offset = 0;

		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dContext->IASetInputLayout(mInputLayout);
		md3dContext->IASetVertexBuffers(0, 1, &mShapesVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mShapesIB, DXGI_FORMAT_R32_UINT, 0);

		if (GetAsyncKeyState('1') & 0x8000)
			md3dContext->RSSetState(RenderStates::WireFrameRS);

		// Draw the grid.
		world = XMLoadFloat4x4(&mGridWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		mPerObject.World = world.Transpose();
		mPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
		mPerObject.WorldViewProj = worldViewProj.Transpose();
		mPerObject.Tex = Matrix::CreateScale(8, 10, 1).Transpose();

		md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
		md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);
		md3dContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		// Draw the box.
		world = XMLoadFloat4x4(&mBoxWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		mPerObject.World = world.Transpose();
		mPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
		mPerObject.WorldViewProj = worldViewProj.Transpose();
		mPerObject.Tex = Matrix::CreateScale(2, 1, 1).Transpose();

		md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
		md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);
		md3dContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&mCylWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			mPerObject.World = world.Transpose();
			mPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
			mPerObject.WorldViewProj = worldViewProj.Transpose();
			mPerObject.Tex = Matrix::CreateScale(1, 2, 1).Transpose();

			md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
			md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);
			md3dContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw the spheres.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&mSphereWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			mPerObject.World = world.Transpose();
			mPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
			mPerObject.WorldViewProj = worldViewProj.Transpose();
			mPerObject.Tex = Matrix::Identity;

			md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
			md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);
			md3dContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

		stride = sizeof(Basic32::Vertex);
		offset = 0;

		md3dContext->RSSetState(0);

		md3dContext->IASetInputLayout(mBasic32->GetInputLayout());
		md3dContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

		// Draw the skull.
		world = XMLoadFloat4x4(&mSkullWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		mPerObject.World = world.Transpose();
		mPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
		mPerObject.WorldViewProj = worldViewProj.Transpose();
		mPerObject.Tex = Matrix::Identity;

		md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
		md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);
		md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);

		md3dContext->VSSetConstantBuffers(1, 1, &mFrameCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mFrameCB);
	}

	void D3DSample::drawScreenQuad()
	{
		UINT stride = sizeof(Basic32::Vertex);
		UINT offset = 0;

		md3dContext->IASetInputLayout(mBasic32->GetInputLayout());
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dContext->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R32_UINT, 0);
		// Scale and shift quad to lower-right corner.
		Matrix world(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, -0.5f, 0.0f, 1.0f);

		mPerObject.WorldViewProj = world.Transpose();
		mPerFrame.bUseTexure = true;
		mPerFrame.bUseFog = false;
		mPerFrame.bUseLight = false;
		md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
		md3dContext->UpdateSubresource(mFrameCB, 0, 0, &mPerFrame, 0, 0);

		auto* shadowSRV = mSmap->DepthMapSRV();
		md3dContext->PSSetShaderResources(0, 1, &shadowSRV);

		md3dContext->DrawIndexed(6, 0, 0);
	}

	void D3DSample::buildShadowTransform()
	{
		// 경계 구의 가운데를 가리키도록 한 후 뷰 행렬을 정의한다.
		XMVECTOR lightDir = XMLoadFloat3(&mDirLights[0].Direction);
		XMVECTOR lightPos = -2.0f * mSceneBounds.Radius * lightDir;
		XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		XMMATRIX V = XMMatrixLookAtLH(lightPos, targetPos, up);

		XMFLOAT3 sphereCenterLS;
		XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, V));

		// 시야 입체를 통해 투영 행렬 구성
		float l = sphereCenterLS.x - mSceneBounds.Radius;
		float b = sphereCenterLS.y - mSceneBounds.Radius;
		float n = sphereCenterLS.z - mSceneBounds.Radius;
		float r = sphereCenterLS.x + mSceneBounds.Radius;
		float t = sphereCenterLS.y + mSceneBounds.Radius;
		float f = sphereCenterLS.z + mSceneBounds.Radius;
		XMMATRIX P = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

		// NDC -> 텍스처 변환 행렬
		XMMATRIX T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);

		XMMATRIX S = V * P * T;

		XMStoreFloat4x4(&mLightView, V);
		XMStoreFloat4x4(&mLightProj, P);
		XMStoreFloat4x4(&mShadowTransform, S);
	}

	void D3DSample::buildShapeGeometryBuffers()
	{
		GeometryGenerator::MeshData box;
		GeometryGenerator::MeshData grid;
		GeometryGenerator::MeshData sphere;
		GeometryGenerator::MeshData cylinder;

		GeometryGenerator geoGen;
		GeometryGenerator::CreateBox(1.0f, 1.0f, 1.0f, &box);
		GeometryGenerator::CreateGrid(20.0f, 30.0f, 50, 40, &grid);
		GeometryGenerator::CreateSphere(0.5f, 20, 20, &sphere);
		GeometryGenerator::CreateCylinder(0.5f, 0.5f, 3.0f, 15, 15, &cylinder);

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

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//

		std::vector<PosNormalTexTan> vertices(totalVertexCount);

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
		vbd.ByteWidth = sizeof(PosNormalTexTan) * totalVertexCount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapesVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//

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

	void D3DSample::buildScreenQuadGeometryBuffers()
	{
		GeometryGenerator::MeshData quad;

		GeometryGenerator geoGen;
		geoGen.CreateFullscreenQuad(&quad);

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//

		std::vector<Basic32::Vertex> vertices(quad.Vertices.size());

		for (UINT i = 0; i < quad.Vertices.size(); ++i)
		{
			vertices[i].Pos = quad.Vertices[i].Position;
			vertices[i].Normal = quad.Vertices[i].Normal;
			vertices[i].Tex = quad.Vertices[i].TexC;
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32::Vertex) * quad.Vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mScreenQuadVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * quad.Indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &quad.Indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mScreenQuadIB));
	}
}