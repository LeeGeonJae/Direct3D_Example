#include <cassert>
#include <time.h>
#include <iostream>
#include <sstream>
#include <fstream>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "MathHelper.h"

namespace instancingAndCulling
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mFrustumCullingEnabled(true)
	{
		srand((unsigned int)time((time_t*)NULL));

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, -15.0f);

		XMMATRIX I = XMMatrixIdentity();

		XMMATRIX skullScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
		XMMATRIX skullOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
		XMStoreFloat4x4(&mSkullWorld, XMMatrixMultiply(skullScale, skullOffset));

		mDirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Direction = XMFLOAT3(0.577f, -0.57735f, 0.57735f);

		mDirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
		mDirLights[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
		mDirLights[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

		mDirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Direction = XMFLOAT3(0.0f, -0.707f, -0.707f);

		mSkullMat.Ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		mSkullMat.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mSkullMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	}

	D3DSample::~D3DSample()
	{
		ReleaseCOM(mPerFrameCB);
		ReleaseCOM(mPerObjectCB);

		ReleaseCOM(mVertexShader);
		ReleaseCOM(mVertexShaderBuffer);
		ReleaseCOM(mPixelShader);
		ReleaseCOM(mLinearSampleState);
		ReleaseCOM(mBasic32);
		ReleaseCOM(mInstancedBasic32);

		ReleaseCOM(mSkullVB);
		ReleaseCOM(mSkullIB);
		ReleaseCOM(mInstancedBuffer);
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		buildConstantBuffer();
		buildShader();
		buildInputLayout();

		buildSkullGeometryBuffers();
		buildInstancedBuffer();

		return true;
	}

	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mCam.SetLens(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);

		BoundingFrustum::CreateFromMatrix(mCamFrustum, mCam.GetProj());
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

		if (GetAsyncKeyState('1') & 0x8000)
			mFrustumCullingEnabled = true;

		if (GetAsyncKeyState('2') & 0x8000)
			mFrustumCullingEnabled = false;

		mCam.UpdateViewMatrix();
		mVisibleObjectCount = 0;

		if (mFrustumCullingEnabled)
		{
			Matrix invView;
			mCam.GetView().Invert(invView);

			D3D11_MAPPED_SUBRESOURCE mappedData;
			md3dContext->Map(mInstancedBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

			InstancedData* dataView = reinterpret_cast<InstancedData*>(mappedData.pData);

			for (UINT i = 0; i < mInstancedData.size(); ++i)
			{
				const auto& instancedData = mInstancedData[i];

				Matrix invWorld;
				instancedData.World.Invert(invWorld);

				Matrix toLocal = invView * invWorld;
				BoundingFrustum localspaceFrustum;
				mCamFrustum.Transform(localspaceFrustum, toLocal);

				if (localspaceFrustum.Intersects(mSkullBox))
				{
					dataView[mVisibleObjectCount++] = instancedData;
				}
			}

			md3dContext->Unmap(mInstancedBuffer, 0);
		}
		else
		{
			D3D11_MAPPED_SUBRESOURCE mappedData;
			md3dContext->Map(mInstancedBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

			InstancedData* dataView = reinterpret_cast<InstancedData*>(mappedData.pData);

			for (UINT i = 0; i < mInstancedData.size(); ++i)
			{
				dataView[mVisibleObjectCount++] = mInstancedData[i];
			}

			md3dContext->Unmap(mInstancedBuffer, 0);
		}

		std::wostringstream outs;
		outs.precision(6);
		outs << L"Instancing and Culling Demo" <<
			L"    " << mVisibleObjectCount <<
			L" objects visible out of " << mInstancedData.size();
		mTitle = outs.str();
	}

	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		md3dContext->ClearRenderTargetView(mRenderTargetView, Silver);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		Matrix view = mCam.GetView();
		Matrix proj = mCam.GetProj();
		Matrix viewProj = mCam.GetViewProj();

		// update FrameCB
		memcpy(mCBPerFrame.DirLights, mDirLights, sizeof(mCBPerFrame.DirLights));
		mCBPerFrame.EyePosW = mCam.GetPosition();
		mCBPerFrame.LigthCount = 10;
		mCBPerFrame.FogColor = Silver;
		mCBPerFrame.FogStart = 525.f;
		mCBPerFrame.FogRange = 175.f;
		md3dContext->UpdateSubresource(mPerFrameCB, 0, 0, &mCBPerFrame, 0, 0);

		Matrix world = mSkullWorld;
		Matrix worldInvTranspose = MathHelper::InverseTranspose(world);
		Matrix worldViewProj = world * viewProj;

		mCBPerObject.World = world.Transpose();
		mCBPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
		mCBPerObject.ViewProj = viewProj.Transpose();
		mCBPerObject.Material = mSkullMat;

		md3dContext->UpdateSubresource(mPerObjectCB, 0, 0, &mCBPerObject, 0, 0);

		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		UINT stride[2] = { sizeof(Basic32), sizeof(InstancedData) };
		UINT offset[2] = { 0, 0 };
		ID3D11Buffer* vbs[2] = { mSkullVB, mInstancedBuffer };
		md3dContext->IASetInputLayout(mInstancedBasic32);
		md3dContext->IASetVertexBuffers(0, 2, vbs, stride, offset);
		md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->PSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->VSSetConstantBuffers(1, 1, &mPerFrameCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mPerFrameCB);
		md3dContext->VSSetShader(mVertexShader, 0, 0);
		md3dContext->PSSetShader(mPixelShader, 0, 0);

		md3dContext->DrawIndexedInstanced(
			mSkullIndexCount, // 인스턴스의 색인 개수
			mVisibleObjectCount, // 인스턴스 개수
			0, // 색인 버퍼 offset
			0, // 기준 정점 offset
			0); // 인스턴스 버퍼의 offset

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

	void D3DSample::buildConstantBuffer()
	{
		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(CBPerObject) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(CBPerObject);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerObjectCB));

		static_assert(sizeof(CBPerFrame) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(CBPerFrame);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerFrameCB));
	}
	void D3DSample::buildShader()
	{
		D3D11_SAMPLER_DESC sampleDesc = {};
		sampleDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampleDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampleDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampleDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampleDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampleDesc.MinLOD = 0;
		sampleDesc.MaxLOD = D3D11_FLOAT32_MAX;

		HR(md3dDevice->CreateSamplerState(&sampleDesc, &mLinearSampleState));

		HR(D3DHelper::CompileShaderFromFile(L"InstancedBasic.hlsl", "VS", "vs_5_0", &mVertexShaderBuffer));

		HR(md3dDevice->CreateVertexShader(
			mVertexShaderBuffer->GetBufferPointer(),
			mVertexShaderBuffer->GetBufferSize(),
			NULL,
			&mVertexShader));

		ID3DBlob* pixelShaderBuffer = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"InstancedBasic.hlsl", "PS", "ps_5_0", &pixelShaderBuffer));

		HR(md3dDevice->CreatePixelShader(
			pixelShaderBuffer->GetBufferPointer(),
			pixelShaderBuffer->GetBufferSize(),
			NULL,
			&mPixelShader));

		ReleaseCOM(pixelShaderBuffer);
	}
	void D3DSample::buildInputLayout()
	{
		const D3D11_INPUT_ELEMENT_DESC InstancedBasic32[8] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64,
				D3D11_INPUT_PER_INSTANCE_DATA, // 인스턴스 별 자료를 의미한다.
				1 // 인스턴스별 자료 원소 하나당 그릴 인스턴스 수를 의미한다.
			}
		};

		HR(md3dDevice->CreateInputLayout(
			InstancedBasic32,
			ARRAYSIZE(InstancedBasic32),
			mVertexShaderBuffer->GetBufferPointer(),
			mVertexShaderBuffer->GetBufferSize(),
			&mInstancedBasic32
		));

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

		XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

		XMVECTOR vMin = XMLoadFloat3(&vMinf3);
		XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
		std::vector<Basic32> vertices(vcount);
		for (UINT i = 0; i < vcount; ++i)
		{
			fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
			fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

			XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

			vMin = XMVectorMin(vMin, P);
			vMax = XMVectorMax(vMax, P);
		}

		XMStoreFloat3(&mSkullBox.Center, 0.5f * (vMin + vMax));
		XMStoreFloat3(&mSkullBox.Extents, 0.5f * (vMax - vMin));

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
		vbd.ByteWidth = sizeof(Basic32) * vcount;
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
	void D3DSample::buildInstancedBuffer()
	{
		const int n = 15;
		mInstancedData.resize(n * n * n);

		float width = 200.0f;
		float height = 200.0f;
		float depth = 200.0f;

		float x = -0.5f * width;
		float y = -0.5f * height;
		float z = -0.5f * depth;
		float dx = width / (n - 1);
		float dy = height / (n - 1);
		float dz = depth / (n - 1);

		for (int k = 0; k < n; ++k)
		{
			for (int i = 0; i < n; ++i)
			{
				for (int j = 0; j < n; ++j)
				{
					// Position instanced along a 3D grid.
					float tx = x + j * dx;
					float ty = y + i * dy;
					float tz = z + k * dz;

					size_t index = (k * n * n) + (i * n) + j;
					mInstancedData[index].World =
					{
						1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f, 0.0f,
						tx, ty, tz, 1.0f
					};

					// Random color.
					mInstancedData[index].Color.x = MathHelper::RandF(0.0f, 1.0f);
					mInstancedData[index].Color.y = MathHelper::RandF(0.0f, 1.0f);
					mInstancedData[index].Color.z = MathHelper::RandF(0.0f, 1.0f);
					mInstancedData[index].Color.w = 1.0f;
				}
			}
		}

		// 인스턴싱될 데이터 크기만큼 미리 버퍼를 잡아서 생성한다.
		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_DYNAMIC;
		vbd.ByteWidth = sizeof(InstancedData) * mInstancedData.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &mInstancedData[0];

		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mInstancedBuffer));
	}
}