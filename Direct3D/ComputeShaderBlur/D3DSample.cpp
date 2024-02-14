#include <cassert>
#include <directxtk/DDSTextureLoader.h>

#include "D3DSample.h"
#include "MathHelper.h"
#include "RenderStates.h"
#include "D3DUtil.h"
#include "GeometryGenerator.h"

namespace computeShaderBlur
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mTheta(1.3f * MathHelper::Pi)
		, mPhi(0.4f * MathHelper::Pi)
		, mRadius(80.0f)
	{
		mEnable4xMsaa = false;
		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		XMMATRIX I = XMMatrixIdentity();
		XMStoreFloat4x4(&mLandWorld, I);
		XMStoreFloat4x4(&mWavesWorld, I);
		XMStoreFloat4x4(&mView, I);
		XMStoreFloat4x4(&mProj, I);

		XMMATRIX boxScale = XMMatrixScaling(15.0f, 15.0f, 15.0f);
		XMMATRIX boxOffset = XMMatrixTranslation(8.0f, 5.0f, -15.0f);
		XMStoreFloat4x4(&mBoxWorld, boxScale * boxOffset);

		XMMATRIX grassTexScale = XMMatrixScaling(5.0f, 5.0f, 0.0f);
		XMStoreFloat4x4(&mGrassTexTransform, grassTexScale);

		mDirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

		mDirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
		mDirLights[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
		mDirLights[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

		mDirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Direction = XMFLOAT3(0.0f, -0.707f, -0.707f);

		mLandMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mLandMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mLandMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

		mWavesMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mWavesMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
		mWavesMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

		mBoxMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mBoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mBoxMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
	}
	D3DSample::~D3DSample()
	{
		md3dContext->ClearState();

		ReleaseCOM(mPerObjectCB);
		ReleaseCOM(mPerFrameCB);

		ReleaseCOM(mVertexShader);
		ReleaseCOM(mVertexShaderBlob);
		ReleaseCOM(mPixelShader);
		ReleaseCOM(mInputLayout);
		ReleaseCOM(mLinearSampler);
		ReleaseCOM(mComputeShaderH);
		ReleaseCOM(mComputeShaderV);

		ReleaseCOM(mLandVB);
		ReleaseCOM(mLandIB);
		ReleaseCOM(mWavesVB);
		ReleaseCOM(mWavesIB);
		ReleaseCOM(mBoxVB);
		ReleaseCOM(mBoxIB);
		ReleaseCOM(mScreenQuadVB);
		ReleaseCOM(mScreenQuadIB);

		ReleaseCOM(mGrassMapSRV);
		ReleaseCOM(mWavesMapSRV);
		ReleaseCOM(mCrateSRV);

		ReleaseCOM(mOffscreenSRV);
		ReleaseCOM(mOffscreenUAV);
		ReleaseCOM(mOffscreenRTV);

		RenderStates::Destroy();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		RenderStates::Init(md3dDevice);

		HR(CreateDDSTextureFromFile(md3dDevice,
			L"../Resource/Textures/grass.dds", 0, &mGrassMapSRV));

		HR(CreateDDSTextureFromFile(md3dDevice,
			L"../Resource/Textures/water2.dds", 0, &mWavesMapSRV));

		HR(CreateDDSTextureFromFile(md3dDevice,
			L"../Resource/Textures/WireFence.dds", 0, &mCrateSRV));

		mWaves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

		buildShader();
		buildInputLayout();
		buildConstantBuffer();

		buildLandGeometryBuffers();
		buildWaveGeometryBuffers();
		buildCrateGeometryBuffers();
		buildScreenQuadGeometryBuffers();
		buildOffscreenViews();

		return true;
	}
	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		buildOffscreenViews();
		mBlur.Init(md3dDevice, mWidth, mHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
		mProj = XMMatrixPerspectiveFovLH(MathHelper::Pi * 0.25f, GetAspectRatio(), 1.f, 1000.f);
	}

	void D3DSample::Update(float deltaTime)
	{
		float x = mRadius * sinf(mPhi) * cosf(mTheta);
		float z = mRadius * sinf(mPhi) * sinf(mTheta);
		float y = mRadius * cosf(mPhi);

		mEyePosW = XMFLOAT3(x, y, z);

		XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
		XMVECTOR target = XMVectorZero();
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
		XMStoreFloat4x4(&mView, V);

		static float t_base = 0.0f;
		if ((mTimer.GetTotalTime() - t_base) >= 0.1f)
		{
			t_base += 0.1f;

			DWORD i = 5 + rand() % (mWaves.GetRowCount() - 10);
			DWORD j = 5 + rand() % (mWaves.GetColumnCount() - 10);

			float r = MathHelper::RandF(0.5f, 1.0f);

			mWaves.Disturb(i, j, r);
		}

		mWaves.Update(deltaTime);

		D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(md3dContext->Map(mWavesVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

		Basic32* v = reinterpret_cast<Basic32*>(mappedData.pData);
		for (UINT i = 0; i < mWaves.GetVertexCount(); ++i)
		{
			v[i].Pos = mWaves[i];
			v[i].Normal = mWaves.GetNormal(i);

			v[i].Tex.x = 0.5f + mWaves[i].x / mWaves.GetWidth();
			v[i].Tex.y = 0.5f - mWaves[i].z / mWaves.GetDepth();
		}

		md3dContext->Unmap(mWavesVB, 0);

		XMMATRIX wavesScale = XMMatrixScaling(5.0f, 5.0f, 0.0f);

		mWaterTexOffset.y += 0.05f * deltaTime;
		mWaterTexOffset.x += 0.1f * deltaTime;
		XMMATRIX wavesOffset = XMMatrixTranslation(mWaterTexOffset.x, mWaterTexOffset.y, 0.0f);

		XMStoreFloat4x4(&mWaterTexTransform, wavesScale * wavesOffset);
	}

	void D3DSample::Render()
	{
		// offsetScreen에 렌더링
		ID3D11RenderTargetView* renderTargets[1] = { mOffscreenRTV };
		md3dContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView);

		memcpy(mCBPerFrame.DirLight, mDirLights, sizeof(mCBPerFrame.DirLight));
		mCBPerFrame.bUseTexutre = true;
		mCBPerFrame.bUseLight = true;
		mCBPerFrame.bUseFog = true;
		mCBPerFrame.LigthCount = 3;
		mCBPerFrame.EyePosW = mEyePosW;
		mCBPerFrame.FogColor = Silver;
		mCBPerFrame.FogStart = 25.f;
		mCBPerFrame.FogRange = 175.f;
		md3dContext->UpdateSubresource(mPerFrameCB, 0, nullptr, &mCBPerFrame, 0, 0);

		md3dContext->ClearRenderTargetView(mOffscreenRTV, reinterpret_cast<const float*>(&Silver));
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		md3dContext->VSSetShader(mVertexShader, nullptr, 0);
		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->PSSetShader(mPixelShader, nullptr, 0);
		md3dContext->PSSetSamplers(0, 1, &mLinearSampler);
		md3dContext->PSSetConstantBuffers(0, 1, &mPerFrameCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mPerObjectCB);
		drawWrapper();

		// 백버퍼 RenderTarget 바인딩
		renderTargets[0] = mRenderTargetView;
		md3dContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView);

		// offsetScreen을 자원뷰로 블러 처리
		mBlur.BlurInPlace(md3dContext, mComputeShaderH, mComputeShaderV, mOffscreenSRV, mOffscreenUAV, 4);

		md3dContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Silver));
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		// offsetScreen을 인자로 최종 그리기
		mCBPerFrame.bUseLight = false;
		mCBPerFrame.bUseFog = false;
		md3dContext->UpdateSubresource(mPerFrameCB, 0, nullptr, &mCBPerFrame, 0, 0);
		drawScreenQuad();

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

			mTheta += dx;
			mPhi += dy;

			mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		}
		else if ((btnState & MK_RBUTTON) != 0)
		{
			float dx = 0.1f * static_cast<float>(x - mLastMousePos.x);
			float dy = 0.1f * static_cast<float>(y - mLastMousePos.y);

			mRadius += dx - dy;

			mRadius = MathHelper::Clamp(mRadius, 20.0f, 500.0f);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}
	void D3DSample::updateCBPerObject(const Matrix& worldMat, const Matrix& texMat, const Material& material)
	{
		mCBPerObject.World = worldMat;
		mCBPerObject.WorldInvTranspose = MathHelper::InverseTranspose(worldMat);
		mCBPerObject.WorldViewProj = worldMat * mView * mProj;

		mCBPerObject.Tex = texMat;

		mCBPerObject.Material = material;

		mCBPerObject.World = mCBPerObject.World.Transpose();
		mCBPerObject.WorldInvTranspose = mCBPerObject.WorldInvTranspose.Transpose();
		mCBPerObject.WorldViewProj = mCBPerObject.WorldViewProj.Transpose();
		mCBPerObject.Tex = mCBPerObject.Tex.Transpose();

		md3dContext->UpdateSubresource(mPerObjectCB, 0, nullptr, &mCBPerObject, 0, 0);
	}
	void D3DSample::buildShader()
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 비교를 통과하지 않는다?
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		md3dDevice->CreateSamplerState(&samplerDesc, &mLinearSampler);

		HR(D3DHelper::CompileShaderFromFile(L"../Resource/Shader/StencilingVS.hlsl", "main", "vs_5_0", &mVertexShaderBlob));
		md3dDevice->CreateVertexShader(mVertexShaderBlob->GetBufferPointer(), mVertexShaderBlob->GetBufferSize(), NULL, &mVertexShader);

		ID3DBlob* pixelShaderBlob = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"../Resource/Shader/StencilingPS.hlsl", "main", "ps_5_0", &pixelShaderBlob));
		md3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), NULL, &mPixelShader);

		ID3DBlob* computeShaderBlob = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"BlurCS.hlsl", "HorzBlurCS", "cs_5_0", &computeShaderBlob));
		md3dDevice->CreateComputeShader(computeShaderBlob->GetBufferPointer(), computeShaderBlob->GetBufferSize(), NULL, &mComputeShaderH);

		HR(D3DHelper::CompileShaderFromFile(L"BlurCS.hlsl", "VertBlurCS", "cs_5_0", &computeShaderBlob));
		md3dDevice->CreateComputeShader(computeShaderBlob->GetBufferPointer(), computeShaderBlob->GetBufferSize(), NULL, &mComputeShaderV);
	}
	void D3DSample::buildInputLayout()
	{
		D3D11_INPUT_ELEMENT_DESC inputDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		md3dDevice->CreateInputLayout(inputDesc, ARRAYSIZE(inputDesc), mVertexShaderBlob->GetBufferPointer(), mVertexShaderBlob->GetBufferSize(), &mInputLayout);
	}
	void D3DSample::buildConstantBuffer()
	{
		D3D11_BUFFER_DESC cbd = {};
		static_assert(sizeof(CBPerFrame) % 16 == 0, "error");
		cbd.ByteWidth = sizeof(CBPerFrame);
		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		cbd.StructureByteStride = 0;
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerFrameCB));

		static_assert(sizeof(CBPerObject) % 16 == 0, "error");
		cbd.ByteWidth = sizeof(CBPerObject);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerObjectCB));
	}

	void D3DSample::updateWaves()
	{

	}
	void D3DSample::drawWrapper()
	{
		md3dContext->IASetInputLayout(mInputLayout);
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		UINT stride = sizeof(Basic32);
		UINT offset = 0;

		XMMATRIX view = XMLoadFloat4x4(&mView);
		XMMATRIX proj = XMLoadFloat4x4(&mProj);
		XMMATRIX viewProj = view * proj;

		updateCBPerObject(mBoxWorld, Matrix::Identity, mBoxMat);
		md3dContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);
		md3dContext->PSSetShaderResources(0, 1, &mCrateSRV);
		md3dContext->RSSetState(RenderStates::NoCullRS);
		md3dContext->DrawIndexed(36, 0, 0);
		md3dContext->RSSetState(0);

		updateCBPerObject(mLandWorld, mGrassTexTransform, mLandMat);
		md3dContext->IASetVertexBuffers(0, 1, &mLandVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mLandIB, DXGI_FORMAT_R32_UINT, 0);
		md3dContext->PSSetShaderResources(0, 1, &mGrassMapSRV);
		md3dContext->DrawIndexed(mLandIndexCount, 0, 0);

		updateCBPerObject(mWavesWorld, mWaterTexTransform, mWavesMat);
		md3dContext->IASetVertexBuffers(0, 1, &mWavesVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mWavesIB, DXGI_FORMAT_R32_UINT, 0);
		md3dContext->PSSetShaderResources(0, 1, &mWavesMapSRV);

		md3dContext->OMSetBlendState(
			RenderStates::TransparentBS, // 랜더스테이트
			blendFactor, // factor
			0xffffffff); // 다중 표본화 샘플 마스크
		md3dContext->DrawIndexed(3 * mWaves.GetTriangleCount(), 0, 0);
		md3dContext->OMSetBlendState(
			NULL, // 기본 블랜드 스테이트로 설정
			blendFactor,
			0xffffffff);
	}

	void D3DSample::drawScreenQuad()
	{
		md3dContext->IASetInputLayout(mInputLayout);
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT stride = sizeof(Basic32);
		UINT offset = 0;

		mCBPerObject.World = Matrix::Identity;
		mCBPerObject.WorldInvTranspose = Matrix::Identity;
		mCBPerObject.WorldViewProj = Matrix::Identity;
		mCBPerObject.Tex = Matrix::Identity;
		md3dContext->UpdateSubresource(mPerObjectCB, 0, nullptr, &mCBPerObject, 0, 0);

		md3dContext->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R32_UINT, 0);
		auto* SRV = mBlur.GetBlurredOutput();
		md3dContext->PSSetShaderResources(0, 1, &SRV);

		md3dContext->DrawIndexed(6, 0, 0);
	}

	float D3DSample::getHillHeight(float x, float z)const
	{
		return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
	}
	Vector3 D3DSample::getHillNormal(float x, float z)const
	{
		XMFLOAT3 n(
			-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
			1.0f,
			-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

		XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
		XMStoreFloat3(&n, unitNormal);

		return n;
	}
	void D3DSample::buildLandGeometryBuffers()
	{
		GeometryGenerator::MeshData grid;

		GeometryGenerator geoGen;

		geoGen.CreateGrid(160.0f, 160.0f, 50, 50, &grid);

		mLandIndexCount = grid.Indices.size();

		std::vector<Basic32> vertices(grid.Vertices.size());
		for (UINT i = 0; i < grid.Vertices.size(); ++i)
		{
			XMFLOAT3 p = grid.Vertices[i].Position;

			p.y = getHillHeight(p.x, p.z);

			vertices[i].Pos = p;
			vertices[i].Normal = getHillNormal(p.x, p.z);
			vertices[i].Tex = grid.Vertices[i].TexC;
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32) * grid.Vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mLandVB));

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * mLandIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &grid.Indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mLandIB));
	}
	void D3DSample::buildWaveGeometryBuffers()
	{
		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_DYNAMIC;
		vbd.ByteWidth = sizeof(Basic32) * mWaves.GetVertexCount();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&vbd, 0, &mWavesVB));

		std::vector<UINT> indices(3 * mWaves.GetTriangleCount()); // 3 indices per face

		UINT m = mWaves.GetRowCount();
		UINT n = mWaves.GetColumnCount();
		int k = 0;
		for (UINT i = 0; i < m - 1; ++i)
		{
			for (DWORD j = 0; j < n - 1; ++j)
			{
				indices[k] = i * n + j;
				indices[k + 1] = i * n + j + 1;
				indices[k + 2] = (i + 1) * n + j;

				indices[k + 3] = (i + 1) * n + j;
				indices[k + 4] = i * n + j + 1;
				indices[k + 5] = (i + 1) * n + j + 1;

				k += 6; // next quadx
			}
		}

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mWavesIB));
	}
	void D3DSample::buildCrateGeometryBuffers()
	{
		GeometryGenerator::MeshData box;

		GeometryGenerator geoGen;
		geoGen.CreateBox(1.0f, 1.0f, 1.0f, &box);

		std::vector<Basic32> vertices(box.Vertices.size());

		for (UINT i = 0; i < box.Vertices.size(); ++i)
		{
			vertices[i].Pos = box.Vertices[i].Position;
			vertices[i].Normal = box.Vertices[i].Normal;
			vertices[i].Tex = box.Vertices[i].TexC;
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32) * box.Vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * box.Indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &box.Indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
	}
	void D3DSample::buildScreenQuadGeometryBuffers()
	{
		GeometryGenerator::MeshData quad;

		GeometryGenerator geoGen;
		geoGen.CreateFullscreenQuad(&quad);

		std::vector<Basic32> vertices(quad.Vertices.size());

		for (UINT i = 0; i < quad.Vertices.size(); ++i)
		{
			vertices[i].Pos = quad.Vertices[i].Position;
			vertices[i].Normal = quad.Vertices[i].Normal;
			vertices[i].Tex = quad.Vertices[i].TexC;
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32) * quad.Vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mScreenQuadVB));

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

	void D3DSample::buildOffscreenViews()
	{
		// 화면에 대응되는 크기르 텍스처 만들어서 자원뷰를 각각 엮는다.

		ReleaseCOM(mOffscreenSRV);
		ReleaseCOM(mOffscreenRTV);
		ReleaseCOM(mOffscreenUAV);

		D3D11_TEXTURE2D_DESC texDesc;

		texDesc.Width = mWidth;
		texDesc.Height = mHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS; // 렌더 타겟, 쉐이더 자원 뷰, 무순서 액세스뷰
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		ID3D11Texture2D* offscreenTex = 0;
		HR(md3dDevice->CreateTexture2D(&texDesc, 0, &offscreenTex));

		HR(md3dDevice->CreateShaderResourceView(offscreenTex, 0, &mOffscreenSRV));
		HR(md3dDevice->CreateRenderTargetView(offscreenTex, 0, &mOffscreenRTV));
		HR(md3dDevice->CreateUnorderedAccessView(offscreenTex, 0, &mOffscreenUAV));

		ReleaseCOM(offscreenTex);
	}
}