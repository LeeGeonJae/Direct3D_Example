#include <cassert>
#include <fstream>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "MathHelper.h"
#include "GeometryGenerator.h"
#include "RenderStates.h"
#include "Ssao.h"

namespace ssao
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mLightRotationAngle(0.0f)
	{
		mTitle = L"SSAO Demo";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
		mSceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);

		Matrix I = Matrix::Identity;

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

		mDirLights[0].Ambient = Vector4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Diffuse = Vector4(0.5f, 0.5f, 0.4f, 1.0f);
		mDirLights[0].Specular = Vector4(0.8f, 0.8f, 0.7f, 1.0f);
		mDirLights[0].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

		// Shadow acne gets worse as we increase the slope of the polygon (from the
		// perspective of the light).
		//mDirLights[0].Direction = XMFLOAT3(5.0f/sqrtf(50.0f), -5.0f/sqrtf(50.0f), 0.0f);
		//mDirLights[0].Direction = XMFLOAT3(10.0f/sqrtf(125.0f), -5.0f/sqrtf(125.0f), 0.0f);
		//mDirLights[0].Direction = XMFLOAT3(10.0f/sqrtf(116.0f), -4.0f/sqrtf(116.0f), 0.0f);
		//mDirLights[0].Direction = XMFLOAT3(10.0f/sqrtf(109.0f), -3.0f/sqrtf(109.0f), 0.0f);

		mDirLights[1].Ambient = Vector4(0.1f, 0.1f, 0.1f, 1.0f);
		mDirLights[1].Diffuse = Vector4(0.40f, 0.40f, 0.40f, 1.0f);
		mDirLights[1].Specular = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[1].Direction = XMFLOAT3(0.707f, -0.707f, 0.0f);

		mDirLights[2].Ambient = Vector4(0.1f, 0.1f, 0.1f, 1.0f);
		mDirLights[2].Diffuse = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Direction = XMFLOAT3(0.0f, 0.0, -1.0f);

		mOriginalLightDir[0] = mDirLights[0].Direction;
		mOriginalLightDir[1] = mDirLights[1].Direction;
		mOriginalLightDir[2] = mDirLights[2].Direction;

		mGridMat.Ambient = Vector4(0.7f, 0.7f, 0.7f, 1.0f);
		mGridMat.Diffuse = Vector4(0.6f, 0.6f, 0.6f, 1.0f);
		mGridMat.Specular = Vector4(0.4f, 0.4f, 0.4f, 16.0f);
		mGridMat.Reflect = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

		mCylinderMat.Ambient = Vector4(0.8f, 0.8f, 0.8f, 1.0f);
		mCylinderMat.Diffuse = Vector4(0.4f, 0.4f, 0.4f, 1.0f);
		mCylinderMat.Specular = Vector4(1.0f, 1.0f, 1.0f, 32.0f);
		mCylinderMat.Reflect = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

		mSphereMat.Ambient = Vector4(0.3f, 0.4f, 0.5f, 1.0f);
		mSphereMat.Diffuse = Vector4(0.2f, 0.3f, 0.4f, 1.0f);
		mSphereMat.Specular = Vector4(0.9f, 0.9f, 0.9f, 16.0f);
		mSphereMat.Reflect = Vector4(0.3f, 0.3f, 0.3f, 1.0f);

		mBoxMat.Ambient = Vector4(0.8f, 0.8f, 0.8f, 1.0f);
		mBoxMat.Diffuse = Vector4(0.4f, 0.4f, 0.4f, 1.0f);
		mBoxMat.Specular = Vector4(0.8f, 0.8f, 0.8f, 16.0f);
		mBoxMat.Reflect = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

		mSkullMat.Ambient = Vector4(0.5f, 0.5f, 0.5f, 1.0f);
		mSkullMat.Diffuse = Vector4(0.2f, 0.2f, 0.2f, 1.0f);
		mSkullMat.Specular = Vector4(0.5f, 0.5f, 0.5f, 16.0f);
		mSkullMat.Reflect = Vector4(0.3f, 0.3f, 0.3f, 1.0f);
	}
	D3DSample::~D3DSample()
	{
		SafeDelete(mSsao);

		auto releaseMap = [](auto& map)
			{
				for (auto& pair : map)
				{
					ReleaseCOM(pair.second);
				}
				map.clear();
			};

		releaseMap(mBuffers);
		releaseMap(mSRVs);
		releaseMap(mInputLayouts);
		releaseMap(mVSs);
		releaseMap(mPSs);
		releaseMap(mSamplerStates);
		releaseMap(mBlobs);

		RenderStates::Destroy();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		RenderStates::Init(md3dDevice);

		buildAll();

		mCam.SetLens(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
		mSsao = new Ssao(md3dDevice, md3dContext, mWidth, mHeight, mCam.GetFovY(), mCam.GetFarZ());

		ID3D11ShaderResourceView* SRV;
		DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/floor.dds", NULL, &SRV);
		mSRVs.insert({ "floor" , SRV });
		DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/bricks.dds", NULL, &SRV);
		mSRVs.insert({ "brick" , SRV });

		buildShapeGeometryBuffers();
		buildSkullGeometryBuffers();
		buildScreenQuadGeometryBuffers();

		return true;
	}

	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mCam.SetLens(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);

		if (mSsao)
		{
			mSsao->OnSize(mWidth, mHeight, mCam.GetFovY(), mCam.GetFarZ());
		}
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

		mLightRotationAngle += 0.1f * deltaTime;
		Matrix R = XMMatrixRotationY(mLightRotationAngle);
		for (int i = 0; i < 3; ++i)
		{
			XMVECTOR lightDir = XMLoadFloat3(&mOriginalLightDir[i]);
			lightDir = XMVector3TransformNormal(lightDir, R);
			XMStoreFloat3(&mDirLights[i].Direction, lightDir);
		}

		mCam.UpdateViewMatrix();
	}

	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		UINT stride = sizeof(Basic32);
		UINT offset = 0;
		auto inputLayout = mInputLayouts.find("basic32");
		assert(inputLayout != mInputLayouts.end());
		md3dContext->IASetInputLayout(inputLayout->second);

		float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };
		md3dContext->ClearRenderTargetView(mRenderTargetView, color);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		if (GetAsyncKeyState('1') & 0x8000)
		{
			// 노말/깊이 맵을 렌더 대상 뷰로 바인딩한다.
			mSsao->SetNormalDepthRenderTarget(mDepthStencilView);

			// 장면의 노말과 깊이값을 그린다.
			drawSceneToSsaoNormalDepthMap();

			// 차폐도를 계산한다.
			mSsao->ComputeSsao(mCam, *this);
			mSsao->BlurAmbientMap(4, *this);
			md3dContext->OMSetDepthStencilState(RenderStates::EqualsDSS, 0);

		}
		ID3D11RenderTargetView* renderTargets[1] = { mRenderTargetView };
		md3dContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView);
		md3dContext->RSSetViewports(1, &mScreenViewport);

		md3dContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Silver));

		auto VS = mVSs.find("basic");
		auto PS = mPSs.find("basic");
		assert(VS != mVSs.end());
		assert(PS != mPSs.end());

		md3dContext->VSSetShader(VS->second, 0, 0);
		md3dContext->PSSetShader(PS->second, 0, 0);

		// ssao 맵 바인딩
		auto ssaoSRV = mSsao->AmbientSRV();
		md3dContext->PSSetShaderResources(1, 1, &ssaoSRV);

		// 샘플러 스테이트 바인딩
		auto sampler = mSamplerStates.find("linearWrap");
		assert(sampler != mSamplerStates.end());
		md3dContext->PSSetSamplers(0, 1, &(sampler->second));

		// 프레임버퍼 업데이트 및 바인딩   
		memcpy(mFrameBasic.DirLights, mDirLights, sizeof(mFrameBasic.DirLights));
		mFrameBasic.EyePosW = mCam.GetPosition();
		memcpy(mFrameBasic.DirLights, mDirLights, sizeof(mFrameBasic.DirLights));
		mFrameBasic.LightCount = 3;
		mFrameBasic.FogColor = Silver;
		mFrameBasic.FogStart = 20.f;
		mFrameBasic.FogRange = 20.f;
		mFrameBasic.UseTexure = true;

		// basic32 상수 버퍼 바인딩
		auto objectCB = mBuffers.find("CBObjectBasic");
		assert(objectCB != mBuffers.end());
		md3dContext->VSSetConstantBuffers(0, 1, &(objectCB->second));
		md3dContext->PSSetConstantBuffers(0, 1, &(objectCB->second));
		auto frameCB = mBuffers.find("CBFrameBasic");
		assert(frameCB != mBuffers.end());
		md3dContext->UpdateSubresource(frameCB->second, 0, 0, &mFrameBasic, 0, 0);
		md3dContext->VSSetConstantBuffers(1, 1, &(frameCB->second));
		md3dContext->PSSetConstantBuffers(1, 1, &(frameCB->second));

		// 자주 쓰이는 변수들
		Matrix view = mCam.GetView();
		Matrix proj = mCam.GetProj();
		Matrix viewProj = mCam.GetViewProj();

		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		Matrix toTexSpace(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);

		// 람다 함수 정의
		auto bindVB = [&](auto name)
			{
				auto VB = mBuffers.find(name);
				if (VB != mBuffers.end())
				{
					md3dContext->IASetVertexBuffers(0, 1, &(VB->second), &stride, &offset);
				}
			};
		auto bindIB = [&](auto name)
			{
				auto IB = mBuffers.find(name);
				if (IB != mBuffers.end())
				{
					md3dContext->IASetIndexBuffer(IB->second, DXGI_FORMAT_R32_UINT, 0);
				}
			};
		auto updateBasic = [&](auto w, auto texTransform, auto material, auto diffuseName)
			{
				Matrix world = w;
				Matrix worldInvTranspose = MathHelper::InverseTranspose(w);
				Matrix worldViewProj = w * view * proj;

				mObjectBasic.World = world.Transpose();
				mObjectBasic.WorldInvTranspose = worldInvTranspose.Transpose();
				mObjectBasic.WorldViewProj = worldViewProj.Transpose();
				mObjectBasic.WorldViewProjTex = (worldViewProj * toTexSpace).Transpose();
				mObjectBasic.TexTransform = texTransform.Transpose();
				mObjectBasic.Material = material;

				auto objectCB = mBuffers.find("CBObjectBasic");
				assert(objectCB != mBuffers.end());
				md3dContext->UpdateSubresource(objectCB->second, 0, 0, &mObjectBasic, 0, 0);

				auto diffuseSRV = mSRVs.find(diffuseName);
				if (diffuseSRV != mSRVs.end())
				{
					md3dContext->PSSetShaderResources(0, 1, &(diffuseSRV->second));
				}
			};

		// shape buffer 바인딩
		bindVB("shapeVB");
		bindIB("shapeIB");

		// 텍스처 사용함
		mFrameBasic.UseTexure = true;
		md3dContext->UpdateSubresource(frameCB->second, 0, 0, &mFrameBasic, 0, 0);

		// 격자 그리기
		updateBasic(mGridWorld, Matrix::CreateScale(8, 10, 1), mGridMat, "floor");
		md3dContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		// 박스 그리기
		updateBasic(mBoxWorld, Matrix::CreateScale(2, 1, 1), mBoxMat, "brick");
		md3dContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		// 원기둥 그리기
		for (int i = 0; i < 10; ++i)
		{
			updateBasic(mCylWorld[i], Matrix::CreateScale(1, 2, 1), mCylinderMat, "brick");
			md3dContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		// 텍스처 사용 안함
		mFrameBasic.UseTexure = false;
		md3dContext->UpdateSubresource(frameCB->second, 0, 0, &mFrameBasic, 0, 0);

		// 구 그리기
		for (int i = 0; i < 10; ++i)
		{
			updateBasic(mSphereWorld[i], Matrix::Identity, mSphereMat, "");
			md3dContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

		// 해골 버퍼 바인딩
		bindVB("skullVB");
		bindIB("skullIB");

		// 해골 그리기
		updateBasic(mSkullWorld, Matrix::Identity, mSkullMat, "");
		md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);

		// 기본 깊이/스텐실 상태로 복원
		md3dContext->OMSetDepthStencilState(0, 0);

		// 디버그 렌더
		//bindVB("quadVB");
		//bindIB("quadIB");
		//Matrix world(
		//	0.5f, 0.0f, 0.0f, 0.0f,
		//	0.0f, 0.5f, 0.0f, 0.0f,
		//	0.0f, 0.0f, 1.0f, 0.0f,
		//	0.5f, -0.5f, 0.0f, 1.0f);
		//md3dContext->PSSetShaderResources(0, 1, &ssaoSRV);

		updateBasic(Matrix::Identity, Matrix::Identity, mSkullMat, "ssao");
		md3dContext->DrawIndexed(6, 0, 0);

		// ssao 맵에 다시 그릴 수 있게 srv 해제
		ID3D11ShaderResourceView* nullSRV[16] = { 0 };
		md3dContext->PSSetShaderResources(0, 16, nullSRV);

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

	void D3DSample::buildAll()
	{
		buildBuffer();
		buidlSRV();
		buildVS({ {"basic", L"Basic.hlsl"},
				{"ssao", L"Ssao.hlsl"},
				{"ssaoBlur", L"SsaoBlur.hlsl"},
				{"ssaoNormalDepth", L"SsaoNormalDepth.hlsl"} });
		buildPS({ {"basic", L"Basic.hlsl"},
				{"ssao", L"Ssao.hlsl"},
				{"ssaoBlur", L"SsaoBlur.hlsl"},
				{"ssaoNormalDepth", L"SsaoNormalDepth.hlsl"} });
		buildInputLayout();
		buildSamplerState();
	}

	void D3DSample::buildBuffer()
	{
		ID3D11Buffer* cb;

		D3D11_BUFFER_DESC cbd = {};
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(ObjectSsaoNormalDepth) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(ObjectSsaoNormalDepth);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &cb));
		mBuffers.insert({ "CBObjectSsaoNormalDepth", cb });

		cbd = {};
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(ObjectBasic) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(ObjectBasic);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &cb));
		mBuffers.insert({ "CBObjectBasic", cb });

		cbd = {};
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(FrameBasic) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(FrameBasic);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &cb));
		mBuffers.insert({ "CBFrameBasic", cb });

		cbd = {};
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(FrameSsao) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(FrameSsao);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &cb));
		mBuffers.insert({ "CBFrameSsao", cb });

		cbd = {};
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(FrameSsaoBlur) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(FrameSsaoBlur);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &cb));
		mBuffers.insert({ "CBFrameSsaoBlur", cb });
	}
	void D3DSample::buidlSRV()
	{
		ID3D11ShaderResourceView* SRV = nullptr;
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/floor.dds", 0, &SRV));
		mSRVs.insert({ "floor", SRV });

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/bricks.dds", 0, &SRV));
		mSRVs.insert({ "brick", SRV });

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/floor_nmap.dds", 0, &SRV));
		mSRVs.insert({ "floorNoraml", SRV });

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/bricks_nmap.dds", 0, &SRV));
		mSRVs.insert({ "brickNormal", SRV });
	}
	void D3DSample::buildInputLayout()
	{
		ID3D11InputLayout* inputLayout = nullptr;

		const D3D11_INPUT_ELEMENT_DESC inputDescBasic32[3] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		auto find = mBlobs.find("basic");
		assert(find != mBlobs.end());
		ID3D10Blob* blob = find->second;
		HR(md3dDevice->CreateInputLayout(
			inputDescBasic32,
			ARRAYSIZE(inputDescBasic32),
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			&inputLayout
		));
		mInputLayouts.insert({ "basic32", inputLayout });

	}
	void D3DSample::buildVS(const std::vector<std::pair<std::string, std::wstring>>& keyFileNames)
	{
		for (auto keyFileNamePair : keyFileNames)
		{
			ID3D11VertexShader* VS = nullptr;
			ID3D10Blob* blob = nullptr;

			HR(D3DHelper::CompileShaderFromFile(keyFileNamePair.second.c_str(), "VS", "vs_5_0", &blob));
			HR(md3dDevice->CreateVertexShader(
				blob->GetBufferPointer(),
				blob->GetBufferSize(),
				NULL,
				&VS));

			mVSs.insert({ keyFileNamePair.first, VS });
			mBlobs.insert({ keyFileNamePair.first, blob });
		}
	}
	void D3DSample::buildPS(const std::vector<std::pair<std::string, std::wstring>>& keyFileNames)
	{
		for (auto keyFileNamePair : keyFileNames)
		{
			ID3D11PixelShader* PS = nullptr;
			ID3D10Blob* blob = nullptr;

			HR(D3DHelper::CompileShaderFromFile(keyFileNamePair.second.c_str(), "PS", "ps_5_0", &blob));
			HR(md3dDevice->CreatePixelShader(
				blob->GetBufferPointer(),
				blob->GetBufferSize(),
				NULL,
				&PS));

			mPSs.insert({ keyFileNamePair.first, PS });
			mBlobs.insert({ keyFileNamePair.first, blob });
		}
	}
	void D3DSample::buildSamplerState()
	{
		ID3D11SamplerState* samplerState = nullptr;

		D3D11_SAMPLER_DESC sampleDesc = {};
		sampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampleDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
		sampleDesc.MinLOD = 0;
		sampleDesc.MaxLOD = D3D11_FLOAT32_MAX;
		HR(md3dDevice->CreateSamplerState(&sampleDesc, &samplerState));
		mSamplerStates.insert({ "linearWrap", samplerState });

		sampleDesc = {};
		sampleDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampleDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
		sampleDesc.MinLOD = 0;
		sampleDesc.MaxLOD = D3D11_FLOAT32_MAX;
		HR(md3dDevice->CreateSamplerState(&sampleDesc, &samplerState));
		mSamplerStates.insert({ "pointClamp", samplerState });

		sampleDesc = {};
		sampleDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		sampleDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
		sampleDesc.BorderColor[0] = { 0.f };
		sampleDesc.BorderColor[1] = { 0.f };
		sampleDesc.BorderColor[2] = { 0.f };
		sampleDesc.BorderColor[3] = { 1e5f };
		sampleDesc.MinLOD = 0;
		sampleDesc.MaxLOD = D3D11_FLOAT32_MAX;
		HR(md3dDevice->CreateSamplerState(&sampleDesc, &samplerState));
		mSamplerStates.insert({ "pointBorder", samplerState });

		sampleDesc = {};
		sampleDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampleDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
		sampleDesc.MinLOD = 0;
		sampleDesc.MaxLOD = D3D11_FLOAT32_MAX;
		HR(md3dDevice->CreateSamplerState(&sampleDesc, &samplerState));
		mSamplerStates.insert({ "pointWrap", samplerState });
	}

	void D3DSample::drawSceneToSsaoNormalDepthMap()
	{
		Matrix view = mCam.GetView();
		Matrix proj = mCam.GetProj();
		Matrix viewProj = XMMatrixMultiply(view, proj);

		UINT stride = sizeof(Basic32);
		UINT offset = 0;

		auto VS = mVSs.find("ssaoNormalDepth");
		assert(VS != mVSs.end());
		md3dContext->VSSetShader(VS->second, 0, 0);
		auto PS = mPSs.find("ssaoNormalDepth");
		assert(PS != mPSs.end());
		md3dContext->PSSetShader(PS->second, 0, 0);
		auto CB0 = mBuffers.find("CBObjectSsaoNormalDepth");
		assert(CB0 != mBuffers.end());
		md3dContext->VSSetConstantBuffers(0, 1, &(CB0->second));
		md3dContext->PSSetConstantBuffers(0, 1, &(CB0->second));
		auto SS0 = mSamplerStates.find("pointBorder");
		assert(SS0 != mSamplerStates.end());
		md3dContext->PSSetSamplers(0, 1, &(SS0->second));

		// 람다 함수 정의
		auto bindVB = [&](auto name)
			{
				auto VB = mBuffers.find(name);
				if (VB != mBuffers.end())
				{
					md3dContext->IASetVertexBuffers(0, 1, &(VB->second), &stride, &offset);
				}
			};
		auto bindIB = [&](auto name)
			{
				auto IB = mBuffers.find(name);
				if (IB != mBuffers.end())
				{
					md3dContext->IASetIndexBuffer(IB->second, DXGI_FORMAT_R32_UINT, 0);
				}
			};
		auto updateSsaoNormalDepth = [&](auto w, auto texTransform)
			{
				Matrix world = w;
				Matrix worldInvTranspose = MathHelper::InverseTranspose(world);
				Matrix worldView = world * view;
				Matrix worldInvTransposeView = worldInvTranspose * view;
				Matrix worldViewProj = world * view * proj;

				mPerObjectSsaoNormalDepth.WorldView = worldView.Transpose();
				mPerObjectSsaoNormalDepth.WorldInvTransposeView = worldInvTransposeView.Transpose();
				mPerObjectSsaoNormalDepth.WorldViewProj = worldViewProj.Transpose();
				mPerObjectSsaoNormalDepth.TexTransform = texTransform.Transpose();

				auto CB = mBuffers.find("CBObjectSsaoNormalDepth");
				assert(CB != mBuffers.end());
				md3dContext->UpdateSubresource(CB->second, 0, 0, &mPerObjectSsaoNormalDepth, 0, 0);
			};

		// 쉐이프 버퍼 바인딩
		bindVB("shapeVB");
		bindIB("shapeIB");

		// 격자 그리기
		updateSsaoNormalDepth(mGridWorld, Matrix::CreateScale(8.0f, 10.0f, 1.0f));
		md3dContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

		// 박스 그리기
		updateSsaoNormalDepth(mBoxWorld, Matrix::CreateScale(2.0f, 1.0f, 1.0f));
		md3dContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

		// 원기둥 그리기
		for (int i = 0; i < 10; ++i)
		{
			updateSsaoNormalDepth(mCylWorld[i], Matrix::CreateScale(1.0f, 2.0f, 1.0f));
			md3dContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
		}

		// 원 그리기
		for (int i = 0; i < 10; ++i)
		{
			updateSsaoNormalDepth(mSphereWorld[i], Matrix::Identity);
			md3dContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
		}

		// 해골 버퍼 바인딩
		bindVB("skullVB");
		bindIB("skullIB");

		// 해골 그리기
		updateSsaoNormalDepth(mSkullWorld, Matrix::Identity);
		md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);
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

		std::vector<Basic32> vertices(totalVertexCount);

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
			vertices[k].Normal.Normalize();
			vertices[k].Tex = cylinder.Vertices[i].TexC;
		}

		ID3D11Buffer* buffer = nullptr;

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32) * totalVertexCount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &buffer));
		mBuffers.insert({ "shapeVB", buffer });

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
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &buffer));
		mBuffers.insert({ "shapeIB", buffer });
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

		std::vector<Basic32> vertices(vcount);
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

		ID3D11Buffer* buffer = nullptr;

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32) * vcount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &buffer));
		mBuffers.insert({ "skullVB", buffer });

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * mSkullIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &buffer));
		mBuffers.insert({ "skullIB", buffer });
	}
	void D3DSample::buildScreenQuadGeometryBuffers()
	{
		GeometryGenerator::MeshData quad;

		GeometryGenerator geoGen;
		GeometryGenerator::CreateFullscreenQuad(&quad);

		//
		// Extract the vertex elements we are interested in and pack the
		// vertices of all the meshes into one vertex buffer.
		//

		std::vector<Basic32> vertices(quad.Vertices.size());

		for (UINT i = 0; i < quad.Vertices.size(); ++i)
		{
			vertices[i].Pos = quad.Vertices[i].Position;
			vertices[i].Normal = quad.Vertices[i].Normal;
			vertices[i].Tex = quad.Vertices[i].TexC;
		}

		ID3D11Buffer* buffer;
		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32) * quad.Vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &buffer));
		mBuffers.insert({ "quadVB", buffer });

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * quad.Indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &quad.Indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &buffer));
		mBuffers.insert({ "quadIB", buffer });
	}
}