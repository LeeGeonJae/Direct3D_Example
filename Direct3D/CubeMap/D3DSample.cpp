#include <cassert>
#include <fstream>
#include <directxtk/DDSTextureLoader.h>

#include "D3DSample.h"
#include "MathHelper.h"
#include "GeometryGenerator.h"
#include "RenderStates.h"

namespace cubeMap
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
	{
		mTitle = L"CubeMap Demo";
		mLightCount = 3;
		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, -50.f);

		Matrix I = Matrix::Identity;
		mGridWorld = I;

		Matrix boxScale = Matrix::CreateScale(3.0f, 1.0f, 3.0f);
		Matrix boxOffset = Matrix::CreateTranslation(0.0f, 0.5f, 0.0f);
		mBoxWorld = boxScale * boxOffset;

		Matrix skullScale = Matrix::CreateScale(0.5f, 0.5f, 0.5f);
		Matrix skullOffset = Matrix::CreateTranslation(0.0f, 1.0f, 0.0f);
		mSkullWorld = skullScale * skullOffset;

		for (int i = 0; i < 5; ++i)
		{
			XMStoreFloat4x4(&mCylWorld[i * 2 + 0], Matrix::CreateTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f));
			XMStoreFloat4x4(&mCylWorld[i * 2 + 1], Matrix::CreateTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f));

			XMStoreFloat4x4(&mSphereWorld[i * 2 + 0], Matrix::CreateTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f));
			XMStoreFloat4x4(&mSphereWorld[i * 2 + 1], Matrix::CreateTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f));
		}

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

		mGridMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mGridMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
		mGridMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

		mCylinderMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mCylinderMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
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
		ReleaseCOM(mPerFrameCB);
		ReleaseCOM(mPerFrameCBPos);
		ReleaseCOM(mPerObjectCB);

		ReleaseCOM(mVertexShader);
		ReleaseCOM(mVertexShaderPos);
		ReleaseCOM(mVertexShaderBuffer);
		ReleaseCOM(mVertexShaderBufferPos);
		ReleaseCOM(mPixelShader);
		ReleaseCOM(mPixelShaderPos);
		ReleaseCOM(mBasic32);
		ReleaseCOM(mPosLayout);
		ReleaseCOM(mLinearSampleState);

		SafeDelete(mSky);

		ReleaseCOM(mShapesVB);
		ReleaseCOM(mShapesIB);
		ReleaseCOM(mSkullVB);
		ReleaseCOM(mSkullIB);
		ReleaseCOM(mFloorTexSRV);
		ReleaseCOM(mStoneTexSRV);
		ReleaseCOM(mBrickTexSRV);

		RenderStates::Destroy();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
			return false;

		// Must init Effects first since InputLayouts depend on shader signatures.

		RenderStates::Init(md3dDevice);

		mSky = new Sky(md3dDevice, L"../Resource/Textures/grasscube1024.dds", 100.f);

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/floor.dds", NULL, &mFloorTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/stone.dds", NULL, &mStoneTexSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/bricks.dds", NULL, &mBrickTexSRV));

		buildShader();
		buildInputLayout();
		buildConstantBuffer();

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
		// 카메라 움직임(camera movement)
		if (GetAsyncKeyState('W') & 0x8000)
			mCam.TranslateLook(10.0f * deltaTime);

		if (GetAsyncKeyState('S') & 0x8000)
			mCam.TranslateLook(-10.0f * deltaTime);

		if (GetAsyncKeyState('A') & 0x8000)
			mCam.TranslateRight(-10.0f * deltaTime);

		if (GetAsyncKeyState('D') & 0x8000)
			mCam.TranslateRight(10.0f * deltaTime);

		// 조명 개수 제어(light count control)
		if (GetAsyncKeyState('0') & 0x8000)
			mLightCount = 0;

		if (GetAsyncKeyState('1') & 0x8000)
			mLightCount = 1;

		if (GetAsyncKeyState('2') & 0x8000)
			mLightCount = 2;

		if (GetAsyncKeyState('3') & 0x8000)
			mLightCount = 3;
	}
	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		md3dContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Silver));
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		md3dContext->IASetInputLayout(mBasic32);
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			UINT stride = sizeof(Basic32);
			UINT offset = 0;
		
			mCam.UpdateViewMatrix();
		
			Matrix view = mCam.GetView();
			Matrix proj = mCam.GetProj();
			Matrix viewProj = mCam.GetViewProj();
		
			float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		
			// 프레임 상수버퍼 업데이트
			memcpy(mCBPerFrame.DirLights, mDirLights, sizeof(mCBPerFrame.DirLights));
			mCBPerFrame.EyePosW = mCam.GetPosition();
			mCBPerFrame.FogColor = Silver;
			mCBPerFrame.FogStart = 500.f;
			mCBPerFrame.FogRange = 500.f;
			mCBPerFrame.LigthCount = mLightCount;
			md3dContext->UpdateSubresource(mPerFrameCB, 0, 0, &mCBPerFrame, 0, 0);
		
			Matrix world;
			Matrix worldInvTranspose;
			Matrix worldViewProj;
		
			md3dContext->IASetVertexBuffers(0, 1, &mShapesVB, &stride, &offset);
			md3dContext->IASetIndexBuffer(mShapesIB, DXGI_FORMAT_R32_UINT, 0);
		
			// 오브젝트 상수버퍼 업데이트 임시 함수(람다)
			auto updatePerObject = [&](const Matrix& world, const Matrix& worldInvTranspose, const Matrix& WVP, const Matrix& Tex, const Material& matrial)
				{
					mCBPerObject.World = world.Transpose();
					mCBPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
					mCBPerObject.WorldViewProj = WVP.Transpose();
					mCBPerObject.Tex = Tex.Transpose();
					mCBPerObject.Material = matrial;
					md3dContext->UpdateSubresource(mPerObjectCB, 0, 0, &mCBPerObject, 0, 0);
				};
		
			// basic32 쉐이더 바인딩
			md3dContext->VSSetShader(mVertexShader, 0, 0);
			md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCB);
			md3dContext->VSSetConstantBuffers(1, 1, &mPerFrameCB);
		
			md3dContext->PSSetShader(mPixelShader, 0, 0);
			md3dContext->PSSetConstantBuffers(0, 1, &mPerObjectCB);
			md3dContext->PSSetConstantBuffers(1, 1, &mPerFrameCB);
		
			// 땅 그리기
			world = mGridWorld;
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;
			updatePerObject(world, worldInvTranspose, worldViewProj, Matrix::CreateScale(6.0f, 8.0f, 1.0f), mGridMat);
			md3dContext->PSSetShaderResources(0, 1, &mFloorTexSRV);
			md3dContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);
		
			// 박스 그리기
			world = mBoxWorld;
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;
			updatePerObject(world, worldInvTranspose, worldViewProj, Matrix::Identity, mBoxMat);
			md3dContext->PSSetShaderResources(0, 1, &mStoneTexSRV);
			md3dContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);
		
			// 원기둥 그리기
			for (int i = 0; i < 10; ++i)
			{
				world = mCylWorld[i];
				worldInvTranspose = MathHelper::InverseTranspose(world);
				worldViewProj = world * view * proj;
		
				updatePerObject(world, worldInvTranspose, worldViewProj, Matrix::Identity, mCylinderMat);
				md3dContext->PSSetShaderResources(0, 1, &mBrickTexSRV);
				md3dContext->DrawIndexed(mCylinderIndexCount, mCylinderIndexOffset, mCylinderVertexOffset);
			}
		
			// 큐브맵에 반사된 구 그리기
			for (int i = 0; i < 10; ++i)
			{
				world = mSphereWorld[i];
				worldInvTranspose = MathHelper::InverseTranspose(world);
				worldViewProj = world * view * proj;
				updatePerObject(world, worldInvTranspose, worldViewProj, Matrix::Identity, mSphereMat);
				md3dContext->PSSetShaderResources(0, 1, &mStoneTexSRV);
				md3dContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
			}
		
			// 해골 그리기
			md3dContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
			md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);
		
			world = mSkullWorld;
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;
			updatePerObject(world, worldInvTranspose, worldViewProj, Matrix::Identity, mSkullMat);
			md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);

			// 바인딩
		md3dContext->VSSetShader(mVertexShaderPos, 0, 0);
		md3dContext->VSSetConstantBuffers(0, 1, &mPerFrameCBPos);

		md3dContext->PSSetShader(mPixelShaderPos, 0, 0);
		md3dContext->PSSetConstantBuffers(0, 1, &mPerFrameCBPos);

		// 버퍼 갱신
		Vector3 eyePos = mCam.GetPosition();
		Matrix W = Matrix::CreateTranslation(eyePos);
		Matrix V = mCam.GetView();
		Matrix P = mCam.GetProj();
		Matrix WVP = W;
		WVP *= V;
		WVP *= P;

		mCBPerFramePos.WorldViewProj = WVP.Transpose();
		md3dContext->UpdateSubresource(mPerFrameCBPos, 0, 0, &mCBPerFramePos, 0, 0);
		auto* SRV = mSky->GetCubeMapSRV();
		md3dContext->PSSetShaderResources(0, 1, &SRV);
		md3dContext->PSSetSamplers(0, 1, &mLinearSampleState);

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

		static_assert(sizeof(CBPerFramePos) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(CBPerFramePos);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerFrameCBPos));
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

		HR(D3DHelper::CompileShaderFromFile(L"Basic.hlsl", "VS", "vs_5_0", &mVertexShaderBuffer));

		HR(md3dDevice->CreateVertexShader(
			mVertexShaderBuffer->GetBufferPointer(),
			mVertexShaderBuffer->GetBufferSize(),
			NULL,
			&mVertexShader));

		ID3DBlob* pixelShaderBuffer = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"Basic.hlsl", "PS", "ps_5_0", &pixelShaderBuffer));

		HR(md3dDevice->CreatePixelShader(
			pixelShaderBuffer->GetBufferPointer(),
			pixelShaderBuffer->GetBufferSize(),
			NULL,
			&mPixelShader));

		ReleaseCOM(pixelShaderBuffer);

		HR(D3DHelper::CompileShaderFromFile(L"Sky.hlsl", "VS", "vs_5_0", &mVertexShaderBufferPos));

		HR(md3dDevice->CreateVertexShader(
			mVertexShaderBufferPos->GetBufferPointer(),
			mVertexShaderBufferPos->GetBufferSize(),
			NULL,
			&mVertexShaderPos));

		ID3DBlob* pixelShaderBufferPos = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"Sky.hlsl", "PS", "ps_5_0", &pixelShaderBufferPos));

		HR(md3dDevice->CreatePixelShader(
			pixelShaderBufferPos->GetBufferPointer(),
			pixelShaderBufferPos->GetBufferSize(),
			NULL,
			&mPixelShaderPos));

		ReleaseCOM(pixelShaderBufferPos);
	}
	void D3DSample::buildInputLayout()
	{
		const D3D11_INPUT_ELEMENT_DESC basic32[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		HR(md3dDevice->CreateInputLayout(
			basic32,
			ARRAYSIZE(basic32),
			mVertexShaderBuffer->GetBufferPointer(),
			mVertexShaderBuffer->GetBufferSize(),
			&mBasic32
		));

		const D3D11_INPUT_ELEMENT_DESC posDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		HR(md3dDevice->CreateInputLayout(
			posDesc,
			ARRAYSIZE(posDesc),
			mVertexShaderBufferPos->GetBufferPointer(),
			mVertexShaderBufferPos->GetBufferSize(),
			&mPosLayout
		));
	}

	void D3DSample::buildShapeGeometryBuffers()
	{
		GeometryGenerator::MeshData box;
		GeometryGenerator::MeshData grid;
		GeometryGenerator::MeshData sphere;
		GeometryGenerator::MeshData cylinder;

		GeometryGenerator geoGen;
		GeometryGenerator::CreateBox(1.0f, 1.0f, 1.0f, &box);
		GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40, &grid);
		GeometryGenerator::CreateSphere(0.5f, 20, 20, &sphere);
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
			vertices[k].Tex = cylinder.Vertices[i].TexC;
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32) * totalVertexCount;
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
}