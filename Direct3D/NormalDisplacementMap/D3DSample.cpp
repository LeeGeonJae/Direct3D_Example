#include <fstream>
#include <cassert>
#include <directxtk/DDSTextureLoader.h>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "Basic32.h"
#include "MathHelper.h"
#include "RenderStates.h"
#include "GeometryGenerator.h"

namespace normalDisplacementMap
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mBasic32(nullptr)
	{
		mTitle = L"Normal- Displacement Map Demo";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		mGridWorld = Matrix::Identity;

		Matrix boxScale = Matrix::CreateScale(3.0f, 1.0f, 3.0f);
		Matrix boxOffset = Matrix::CreateTranslation(0.0f, 0.5f, 0.0f);
		mBoxWorld = boxScale * boxOffset;

		Matrix skullScale = Matrix::CreateScale(0.5f, 0.5f, 0.5f);
		Matrix skullOffset = Matrix::CreateTranslation(0.0f, 1.0f, 0.0f);
		mSkullWorld = skullScale * skullOffset;

		for (int i = 0; i < 5; ++i)
		{
			mCylWorld[i * 2 + 0] = Matrix::CreateTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
			mCylWorld[i * 2 + 1] = Matrix::CreateTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

			mSphereWorld[i * 2 + 0] = Matrix::CreateTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
			mSphereWorld[i * 2 + 1] = Matrix::CreateTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);
		}

		mDirLights[0].Ambient = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[0].Diffuse = Vector4(0.7f, 0.7f, 0.6f, 1.0f);
		mDirLights[0].Specular = Vector4(0.8f, 0.8f, 0.7f, 1.0f);
		mDirLights[0].Direction = Vector3(0.707f, 0.0f, 0.707f);

		mDirLights[1].Ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = Vector4(0.40f, 0.40f, 0.40f, 1.0f);
		mDirLights[1].Specular = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[1].Direction = Vector3(0.0f, -0.707f, 0.707f);

		mDirLights[2].Ambient = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Direction = Vector3(-0.57735f, -0.57735f, -0.57735f);

		mGridMat.Ambient = Vector4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Diffuse = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		mGridMat.Specular = Vector4(0.4f, 0.4f, 0.4f, 16.0f);
		mGridMat.Reflect = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

		mCylinderMat.Ambient = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Diffuse = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Specular = Vector4(1.0f, 1.0f, 1.0f, 32.0f);
		mCylinderMat.Reflect = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

		mSphereMat.Ambient = Vector4(0.2f, 0.3f, 0.4f, 1.0f);
		mSphereMat.Diffuse = Vector4(0.2f, 0.3f, 0.4f, 1.0f);
		mSphereMat.Specular = Vector4(0.9f, 0.9f, 0.9f, 16.0f);
		mSphereMat.Reflect = Vector4(0.4f, 0.4f, 0.4f, 1.0f);

		mBoxMat.Ambient = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Diffuse = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Specular = Vector4(0.8f, 0.8f, 0.8f, 16.0f);
		mBoxMat.Reflect = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

		mSkullMat.Ambient = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mSkullMat.Diffuse = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mSkullMat.Specular = Vector4(0.8f, 0.8f, 0.8f, 16.0f);
		mSkullMat.Reflect = Vector4(0.5f, 0.5f, 0.5f, 1.0f);
	}
	D3DSample::~D3DSample()
	{
		ReleaseCOM(mPerFrameCBPos);
		ReleaseCOM(mVertexShaderPos);
		ReleaseCOM(mPixelShaderPos);
		ReleaseCOM(mPosLayout);
		ReleaseCOM(mLinearSampler);
		ReleaseCOM(mNormalVS);
		ReleaseCOM(mNormalPS);
		ReleaseCOM(mDisplacementVS);
		ReleaseCOM(mDisplacementHS);
		ReleaseCOM(mDisplacementDS);
		ReleaseCOM(mDisplacementPS);
		ReleaseCOM(mSamLinear);
		ReleaseCOM(mPerObjectDisplacemnetCB);
		ReleaseCOM(mPerFrameDisplacemnetCB);
		ReleaseCOM(mDisplacementIL);

		SafeDelete(mBasic32);
		SafeDelete(mSky);

		ReleaseCOM(mShapesVB);
		ReleaseCOM(mShapesIB);
		ReleaseCOM(mSkullVB);
		ReleaseCOM(mSkullIB);
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
		RenderStates::Init(md3dDevice);

		mSky = new Sky(md3dDevice, L"../Resource/Textures/snowcube1024.dds", 5000.0f);

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/floor.dds", 0, &mStoneTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/bricks.dds", 0, &mBrickTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/floor_nmap.dds", 0, &mStoneNormalTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/bricks_nmap.dds", 0, &mBrickNormalTexSRV));

		buildElement();
		buildSky();
		buildShapeGeometryBuffers();
		buildSkullGeometryBuffers();

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
		// if (GetAsyncKeyState('2') & 0x8000)
		// 	mRenderOptions = RenderOptionsBasic;
		// 
		// if (GetAsyncKeyState('3') & 0x8000)
		// 	mRenderOptions = RenderOptionsNormalMap;
		// 
		// if (GetAsyncKeyState('4') & 0x8000)
		// 	mRenderOptions = RenderOptionsDisplacementMap;
	}
	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		md3dContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Silver));
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		mCam.UpdateViewMatrix();

		Matrix view = mCam.GetView();
		Matrix proj = mCam.GetProj();
		Matrix viewProj = mCam.GetViewProj();

		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		// 토폴로지
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

		// 프레임 상수버퍼 바인딩 업데이트
		Basic32::PerFrame& perFrame = mBasic32->GetPerFrame();
		memcpy(perFrame.DirLights, mDirLights, sizeof(perFrame.DirLights));
		memcpy(mCBPerFrameDisplacement.DirLights, mDirLights, sizeof(mCBPerFrameDisplacement.DirLights));
		perFrame.EyePosW = mCBPerFrameDisplacement.EyePosW = mCam.GetPosition(); // 월드 시야 위치
		perFrame.bUseFog = mCBPerFrameDisplacement.bUseFog = true;
		perFrame.bUseLight = mCBPerFrameDisplacement.bUseLight = true;
		perFrame.bUseTexutre = mCBPerFrameDisplacement.bUseTexure = true;
		perFrame.LigthCount = mCBPerFrameDisplacement.LightCount = 3;
		perFrame.FogColor = mCBPerFrameDisplacement.FogColor = Silver;
		perFrame.FogStart = mCBPerFrameDisplacement.FogStart = 25.f;
		perFrame.FogRange = mCBPerFrameDisplacement.FogRange = 125.f;
		mCBPerFrameDisplacement.HeightScale = 0.1f;
		mCBPerFrameDisplacement.MaxTessDistance = 1.f;
		mCBPerFrameDisplacement.MinTessDistance = 25.f;
		mCBPerFrameDisplacement.MinTessFactor = 1.f;
		mCBPerFrameDisplacement.MaxTessFactor = 5.f;

		md3dContext->VSSetConstantBuffers(1, 1, &mPerFrameDisplacemnetCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mPerFrameDisplacemnetCB);
		md3dContext->HSSetConstantBuffers(1, 1, &mPerFrameDisplacemnetCB);
		md3dContext->DSSetConstantBuffers(1, 1, &mPerFrameDisplacemnetCB);
		md3dContext->UpdateSubresource(mPerFrameDisplacemnetCB, 0, 0, &mCBPerFrameDisplacement, 0, 0);

		// 오브젝트 상수버퍼 바인딩
		auto* perObjectCB = mBasic32->GetPerObjectCB();
		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectDisplacemnetCB);
		md3dContext->PSSetConstantBuffers(0, 1, &mPerObjectDisplacemnetCB);
		md3dContext->HSSetConstantBuffers(0, 1, &mPerObjectDisplacemnetCB);
		md3dContext->DSSetConstantBuffers(0, 1, &mPerObjectDisplacemnetCB);

		// 쉐이더 바인딩
		md3dContext->VSSetShader(mDisplacementVS, 0, 0);
		md3dContext->HSSetShader(mDisplacementHS, 0, 0);
		md3dContext->DSSetShader(mDisplacementDS, 0, 0);
		md3dContext->PSSetShader(mDisplacementPS, 0, 0);

		// 샘플 스테이트 바인딩
		md3dContext->VSSetSamplers(0, 1, &mLinearSampler);
		md3dContext->HSSetSamplers(0, 1, &mLinearSampler);
		md3dContext->DSSetSamplers(0, 1, &mLinearSampler);
		md3dContext->PSSetSamplers(0, 1, &mLinearSampler);

		// 스카이 큐브맵
		auto* skySRV = mSky->GetCubeMapSRV();
		md3dContext->PSSetShaderResources(2, 1, &skySRV);

		Basic32::PerObject& perObject = mBasic32->GetPerObject();
		Matrix world;
		Matrix worldInvTranspose;
		Matrix worldViewProj;

		UINT stride = sizeof(PosNormalTexTan);
		UINT offset = 0;

		md3dContext->IASetInputLayout(mDisplacementIL);
		md3dContext->IASetVertexBuffers(0, 1, &mShapesVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mShapesIB, DXGI_FORMAT_R32_UINT, 0);

		if (GetAsyncKeyState('1') & 0x8000)
			md3dContext->RSSetState(RenderStates::WireFrameRS);

		// 바닥 그리기
		world = mGridWorld;
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		mCBPerObjectDisplacement.World = world.Transpose();
		mCBPerObjectDisplacement.WorldInvTranspose = worldInvTranspose.Transpose();
		mCBPerObjectDisplacement.ViewProj = viewProj.Transpose();
		mCBPerObjectDisplacement.WorldViewProj = worldViewProj.Transpose();
		mCBPerObjectDisplacement.Tex = Matrix::CreateScale(8.0f, 10.0f, 1.0f);
		mCBPerObjectDisplacement.Material = mGridMat;
		md3dContext->UpdateSubresource(mPerObjectDisplacemnetCB, 0, 0, &mCBPerObjectDisplacement, 0, 0);

		md3dContext->PSSetShaderResources(0, 1, &mStoneTexSRV);
		md3dContext->PSSetShaderResources(1, 1, &mStoneNormalTexSRV);
		md3dContext->DSSetShaderResources(0, 1, &mStoneTexSRV);
		md3dContext->DSSetShaderResources(1, 1, &mStoneNormalTexSRV);
		md3dContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		// 박스 그리기
		world = mBoxWorld;
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		mCBPerObjectDisplacement.World = world.Transpose();
		mCBPerObjectDisplacement.WorldInvTranspose = worldInvTranspose.Transpose();
		mCBPerObjectDisplacement.WorldViewProj = worldViewProj.Transpose();
		mCBPerObjectDisplacement.Tex = Matrix::CreateScale(2.0f, 1.0f, 1.0f);
		mCBPerObjectDisplacement.Material = mBoxMat;
		md3dContext->UpdateSubresource(mPerObjectDisplacemnetCB, 0, 0, &mCBPerObjectDisplacement, 0, 0);

		md3dContext->PSSetShaderResources(0, 1, &mBrickTexSRV);
		md3dContext->PSSetShaderResources(1, 1, &mBrickNormalTexSRV);
		md3dContext->DSSetShaderResources(0, 1, &mBrickTexSRV);
		md3dContext->DSSetShaderResources(1, 1, &mBrickNormalTexSRV);
		md3dContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		// 원기둥 그리기
		for (int i = 0; i < 10; ++i)
		{
			world = mCylWorld[i];
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			mCBPerObjectDisplacement.World = world.Transpose();
			mCBPerObjectDisplacement.WorldInvTranspose = worldInvTranspose.Transpose();
			mCBPerObjectDisplacement.WorldViewProj = worldViewProj.Transpose();
			mCBPerObjectDisplacement.Tex = Matrix::CreateScale(1.0f, 2.0f, 1.0f);
			mCBPerObjectDisplacement.Material = mCylinderMat;
			md3dContext->UpdateSubresource(mPerObjectDisplacemnetCB, 0, 0, &mCBPerObjectDisplacement, 0, 0);

			md3dContext->PSSetShaderResources(0, 1, &mBrickTexSRV);
			md3dContext->PSSetShaderResources(1, 1, &mBrickNormalTexSRV);
			md3dContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		// 테셀레이션 끄기
		md3dContext->HSSetShader(0, 0, 0);
		md3dContext->DSSetShader(0, 0, 0);
		mBasic32->Bind(md3dContext);

		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 구와 큐브맵 반사
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&mSphereWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			perObject.World = world.Transpose();
			perObject.WorldInvTranspose = worldInvTranspose.Transpose();
			perObject.WorldViewProj = worldViewProj.Transpose();
			perObject.Tex = Matrix::CreateScale(1.0f, 2.0f, 1.0f);
			perObject.Material = mSphereMat;

			perFrame.bUseTexutre = false;
			mBasic32->UpdateSubresource(md3dContext);
			md3dContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

		// 해골 그리기
		md3dContext->RSSetState(0);

		stride = sizeof(Basic32::Vertex);
		md3dContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

		world = mSkullWorld;
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		perObject.World = world.Transpose();
		perObject.WorldInvTranspose = worldInvTranspose.Transpose();
		perObject.WorldViewProj = worldViewProj.Transpose();
		perObject.Tex = Matrix::CreateScale(1.0f, 2.0f, 1.0f);
		perObject.Material = mSphereMat;

		mBasic32->UpdateSubresource(md3dContext);
		md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);

		// 하늘 그리기
		md3dContext->VSSetShader(mVertexShaderPos, 0, 0);
		md3dContext->VSSetConstantBuffers(0, 1, &mPerFrameCBPos);

		md3dContext->PSSetShader(mPixelShaderPos, 0, 0);
		md3dContext->PSSetConstantBuffers(0, 1, &mPerFrameCBPos);
		md3dContext->PSSetSamplers(0, 1, &mLinearSampler);

		// 버퍼 갱신
		Vector3 eyePos = mCam.GetPosition();
		Matrix T = Matrix::CreateTranslation(eyePos);
		Matrix WVP = T * mCam.GetViewProj();
		mCBPerFramePos.WorldViewProj = WVP.Transpose();
		md3dContext->UpdateSubresource(mPerFrameCBPos, 0, 0, &mCBPerFramePos, 0, 0);

		md3dContext->RSSetState(RenderStates::NoCullRS);
		md3dContext->OMSetDepthStencilState(RenderStates::LessEqualDSS, 0);
		mSky->Draw(md3dContext, mCam);
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
			float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.RotatePitch(dy);
			mCam.RotateAxis({ 0, 1, 0 }, dx);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

	void D3DSample::buildElement()
	{
		// 입력 레이아웃
		const D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		// 쉐이더 컴파일
		ID3D10Blob* blob;
		HR(D3DHelper::CompileShaderFromFile(L"NormalMap.hlsl", "VS", "vs_5_0", &blob));
		HR(md3dDevice->CreateVertexShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mNormalVS
		));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(L"NormalMap.hlsl", "PS", "ps_5_0", &blob));
		HR(md3dDevice->CreatePixelShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mNormalPS
		));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(L"DisplacementMap.hlsl", "VS", "vs_5_0", &blob));
		HR(md3dDevice->CreateVertexShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mDisplacementVS
		));
		HR(md3dDevice->CreateInputLayout(
			inputLayoutDesc,
			ARRAYSIZE(inputLayoutDesc),
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			&mDisplacementIL
		));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(L"DisplacementMap.hlsl", "HS", "hs_5_0", &blob));
		HR(md3dDevice->CreateHullShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mDisplacementHS
		));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(L"DisplacementMap.hlsl", "DS", "ds_5_0", &blob));
		HR(md3dDevice->CreateDomainShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mDisplacementDS
		));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(L"DisplacementMap.hlsl", "PS", "ps_5_0", &blob));
		HR(md3dDevice->CreatePixelShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mDisplacementPS
		));
		ReleaseCOM(blob);

		// 샘플 스테이트
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 비교를 통과하지 않는다?
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		md3dDevice->CreateSamplerState(&samplerDesc, &mSamLinear);

		// 상수 버퍼
		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		static_assert(sizeof(PerFrameDisplacement) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerFrameDisplacement);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerFrameDisplacemnetCB));

		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		static_assert(sizeof(PerObjectDisplacement) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerObjectDisplacement);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerObjectDisplacemnetCB));
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
		ReleaseCOM(vertexShaderBufferPos);

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

		// 한 버퍼에 여러 기하구조 담기, drawIndexed에 사용할 인수들
		mBoxVertexOffset = 0;
		mGridVertexOffset = box.Vertices.size();
		mSphereVertexOffset = mGridVertexOffset + grid.Vertices.size();
		mCylinderVertexOffset = mSphereVertexOffset + sphere.Vertices.size();

		mBoxIndexCount = box.Indices.size();
		mGridIndexCount = grid.Indices.size();
		mSphereIndexCount = sphere.Indices.size();
		mCylinderIndexCount = cylinder.Indices.size();

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


		// 정점 버퍼 초기화
		std::vector<PosNormalTexTan> vertices(totalVertexCount);
		UINT k = 0;
		for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = box.Vertices[i].Position;
			vertices[k].Normal = box.Vertices[i].Normal;
			vertices[k].Tex = box.Vertices[i].TexC;
			vertices[k].TangentU = box.Vertices[i].TangentU;
		}
		for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = grid.Vertices[i].Position;
			vertices[k].Normal = grid.Vertices[i].Normal;
			vertices[k].Tex = grid.Vertices[i].TexC;
			vertices[k].TangentU = grid.Vertices[i].TangentU;
		}
		for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = sphere.Vertices[i].Position;
			vertices[k].Normal = sphere.Vertices[i].Normal;
			vertices[k].Tex = sphere.Vertices[i].TexC;
			vertices[k].TangentU = sphere.Vertices[i].TangentU;
		}
		for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
		{
			vertices[k].Pos = cylinder.Vertices[i].Position;
			vertices[k].Normal = cylinder.Vertices[i].Normal;
			vertices[k].Tex = cylinder.Vertices[i].TexC;
			vertices[k].TangentU = cylinder.Vertices[i].TangentU;
		}

		// 버텍스 버퍼
		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(PosNormalTexTan) * totalVertexCount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapesVB));

		// 인덱스 버퍼
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

		// 상수 버퍼 생성
		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32::Vertex) * vcount; // 기본 기하구조 활용
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

		// 인덱스버퍼 생성
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