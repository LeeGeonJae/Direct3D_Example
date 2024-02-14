#include <cassert>

#include "D3DSample.h"
#include "MathHelper.h"
#include "D3DUtil.h"
#include "RenderStates.h"

namespace tessellation
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mTheta(1.3f * MathHelper::Pi)
		, mPhi(0.2f * MathHelper::Pi)
		, mRadius(80.0f)
	{
		mEnable4xMsaa = false;

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;
	}
	D3DSample::~D3DSample()
	{
		ReleaseCOM(mPerObjectCB);
		ReleaseCOM(mPerFrameCB);
		ReleaseCOM(mInputLayout);
		ReleaseCOM(mVertexShader);
		ReleaseCOM(mVertexShaderBlob);
		ReleaseCOM(mHullShader);
		ReleaseCOM(mDomainShader);
		ReleaseCOM(mPixelShader);
		ReleaseCOM(mQuadPatchVB);

		ReleaseCOM(mVertexShaderBazier);
		ReleaseCOM(mVertexShaderBlobBazier);
		ReleaseCOM(mHullShaderBazier);
		ReleaseCOM(mDomainShaderBazier);
		ReleaseCOM(mPixelShaderBazier);
		ReleaseCOM(mQuadPatchBazierVB);

		RenderStates::Destroy();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		RenderStates::Init(md3dDevice);

		buildConstantBuffer();
		buildShader();
		buildInputLayout();
		buildQuadPatchBuffer();

		return true;
	}

	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mProj = XMMatrixPerspectiveFovLH(MathHelper::Pi * 0.25f, GetAspectRatio(), 1.f, 1000.f);
	}

	void D3DSample::Update(float deltaTime)
	{
		float x = mRadius * sinf(mPhi) * cosf(mTheta);
		float z = mRadius * sinf(mPhi) * sinf(mTheta);
		float y = mRadius * cosf(mPhi);

		mEyePosW = XMFLOAT3(x, y, z);

		// Build the view matrix.
		XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
		XMVECTOR target = XMVectorZero();
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
		XMStoreFloat4x4(&mView, V);
	}

	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };

		md3dContext->ClearRenderTargetView(mRenderTargetView, color);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->VSSetConstantBuffers(1, 1, &mPerFrameCB);
		md3dContext->VSSetShader(mVertexShader, 0, 0);
		md3dContext->HSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->HSSetConstantBuffers(1, 1, &mPerFrameCB);
		md3dContext->HSSetShader(mHullShader, 0, 0);
		md3dContext->DSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->DSSetConstantBuffers(1, 1, &mPerFrameCB);
		md3dContext->DSSetShader(mDomainShader, 0, 0);
		md3dContext->PSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mPerFrameCB);
		md3dContext->PSSetShader(mPixelShader, 0, 0);

		float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		XMMATRIX view = XMLoadFloat4x4(&mView);
		XMMATRIX proj = XMLoadFloat4x4(&mProj);
		XMMATRIX viewProj = view * proj;

		md3dContext->IASetInputLayout(mInputLayout);
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

		UINT stride = sizeof(Pos);
		UINT offset = 0;

		// Set per frame constants.
		mCBPerFrame.EyePosW = mEyePosW;
		mCBPerFrame.FogColor = Silver;
		mCBPerFrame.FogStart = 15.f;
		mCBPerFrame.FogRange = 175.f;
		mCBPerFrame.bUseFog = true;
		mCBPerFrame.bUseLight = true;
		mCBPerFrame.bUseTexutre = true;

		md3dContext->UpdateSubresource(mPerFrameCB, 0, 0, &mCBPerFrame, 0, 0);

		md3dContext->IASetVertexBuffers(0, 1, &mQuadPatchVB, &stride, &offset);

		Matrix world = Matrix::Identity;
		mCBPerObject.World = world.Transpose();
		mCBPerObject.WorldInvTranspose = MathHelper::InverseTranspose(world).Transpose();
		mCBPerObject.WorldViewProj = (world * view * proj).Transpose();
		mCBPerObject.Tex = Matrix::Identity;

		md3dContext->UpdateSubresource(mPerObjectCB, 0, 0, &mCBPerObject, 0, 0);

		md3dContext->RSSetState(RenderStates::WireFrameRS);
		md3dContext->Draw(4, 0);

		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
		md3dContext->VSSetShader(mVertexShaderBazier, 0, 0);
		md3dContext->HSSetShader(mHullShaderBazier, 0, 0);
		md3dContext->DSSetShader(mDomainShaderBazier, 0, 0);
		md3dContext->PSSetShader(mPixelShaderBazier, 0, 0);
		md3dContext->IASetVertexBuffers(0, 1, &mQuadPatchBazierVB, &stride, &offset);
		// md3dContext->Draw(16, 0);

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

			// Update angles based on input to orbit camera around box.
			mTheta += dx;
			mPhi += dy;

			// Restrict the angle mPhi.
			mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		}
		else if ((btnState & MK_RBUTTON) != 0)
		{
			float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
			float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = MathHelper::Clamp(mRadius, 5.0f, 300.0f);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
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
	void D3DSample::buildShader()
	{
		HR(D3DHelper::CompileShaderFromFile(L"VertexShader.hlsl", "main", "vs_5_0", &mVertexShaderBlob));
		md3dDevice->CreateVertexShader(mVertexShaderBlob->GetBufferPointer(), mVertexShaderBlob->GetBufferSize(), NULL, &mVertexShader);

		ID3DBlob* blob = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"PixelShader.hlsl", "main", "ps_5_0", &blob));
		md3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &mPixelShader);

		HR(D3DHelper::CompileShaderFromFile(L"HullShader.hlsl", "HS", "hs_5_0", &blob));
		md3dDevice->CreateHullShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &mHullShader);

		HR(D3DHelper::CompileShaderFromFile(L"DomainShader.hlsl", "main", "ds_5_0", &blob));
		md3dDevice->CreateDomainShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &mDomainShader);

		HR(D3DHelper::CompileShaderFromFile(L"Bazier.hlsl", "VS", "vs_5_0", &mVertexShaderBlob));
		md3dDevice->CreateVertexShader(mVertexShaderBlob->GetBufferPointer(), mVertexShaderBlob->GetBufferSize(), NULL, &mVertexShaderBazier);

		HR(D3DHelper::CompileShaderFromFile(L"Bazier.hlsl", "PS", "ps_5_0", &blob));
		md3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &mPixelShaderBazier);

		HR(D3DHelper::CompileShaderFromFile(L"Bazier.hlsl", "HS", "hs_5_0", &blob));
		md3dDevice->CreateHullShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &mHullShaderBazier);

		HR(D3DHelper::CompileShaderFromFile(L"Bazier.hlsl", "DS", "ds_5_0", &blob));
		md3dDevice->CreateDomainShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &mDomainShaderBazier);
	}
	void D3DSample::buildInputLayout()
	{
		D3D11_INPUT_ELEMENT_DESC inputDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		md3dDevice->CreateInputLayout(inputDesc, ARRAYSIZE(inputDesc), mVertexShaderBlob->GetBufferPointer(), mVertexShaderBlob->GetBufferSize(), &mInputLayout);
	}

	void D3DSample::buildQuadPatchBuffer()
	{
		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(XMFLOAT3) * 4;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;

		XMFLOAT3 vertices[4] =
		{
			XMFLOAT3(-10.0f, 0.0f, +10.0f),
			XMFLOAT3(+10.0f, 0.0f, +10.0f),
			XMFLOAT3(-10.0f, 0.0f, -10.0f),
			XMFLOAT3(+10.0f, 0.0f, -10.0f)
		};

		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = vertices;
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mQuadPatchVB));

		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(XMFLOAT3) * 16;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;

		XMFLOAT3 verticesBazier[16] =
		{
			// Row 0
			XMFLOAT3(-10.0f, -10.0f, +15.0f),
			XMFLOAT3(-5.0f,  0.0f, +15.0f),
			XMFLOAT3(+5.0f,  0.0f, +15.0f),
			XMFLOAT3(+10.0f, 0.0f, +15.0f),

			// Row 1
			XMFLOAT3(-15.0f, 0.0f, +5.0f),
			XMFLOAT3(-5.0f,  0.0f, +5.0f),
			XMFLOAT3(+5.0f,  20.0f, +5.0f),
			XMFLOAT3(+15.0f, 0.0f, +5.0f),

			// Row 2
			XMFLOAT3(-15.0f, 0.0f, -5.0f),
			XMFLOAT3(-5.0f,  0.0f, -5.0f),
			XMFLOAT3(+5.0f,  0.0f, -5.0f),
			XMFLOAT3(+15.0f, 0.0f, -5.0f),

			// Row 3
			XMFLOAT3(-10.0f, 10.0f, -15.0f),
			XMFLOAT3(-5.0f,  0.0f, -15.0f),
			XMFLOAT3(+5.0f,  0.0f, -15.0f),
			XMFLOAT3(+25.0f, 10.0f, -15.0f)
		};

		vinitData.pSysMem = verticesBazier;
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mQuadPatchBazierVB));
	}
}