#include <cassert>

#include "D3DSample.h"
#include "RenderStates.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"

namespace geometryShader
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mTheta(1.3f * MathHelper::Pi)
		, mPhi(0.4f * MathHelper::Pi)
		, mRadius(80.0f)
	{
		mEnable4xMsaa = true;

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mLandWorld = Matrix::Identity;
		mWavesWorld = Matrix::Identity;
		mView = Matrix::Identity;
		mProj = Matrix::Identity;

		Matrix boxScale = DirectX::XMMatrixScaling(15.0f, 15.0f, 15.0f);
		Matrix boxOffset = DirectX::XMMatrixTranslation(8.0f, 5.0f, -15.0f);
		XMStoreFloat4x4(&mBoxWorld, boxScale * boxOffset);

		Matrix grassTexScale = DirectX::XMMatrixScaling(5.0f, 5.0f, 0.0f);
		XMStoreFloat4x4(&mGrassTexTransform, grassTexScale);

		mDirLights[0].Ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
		mDirLights[0].Diffuse = { 0.5f, 0.5f, 0.5f, 1.0f };
		mDirLights[0].Specular = { 0.5f, 0.5f, 0.5f, 1.0f };
		mDirLights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };

		mDirLights[1].Ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
		mDirLights[1].Diffuse = { 0.20f, 0.20f, 0.20f, 1.0f };
		mDirLights[1].Specular = { 0.25f, 0.25f, 0.25f, 1.0f };
		mDirLights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };

		mDirLights[2].Ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
		mDirLights[2].Diffuse = { 0.2f, 0.2f, 0.2f, 1.0f };
		mDirLights[2].Specular = { 0.0f, 0.0f, 0.0f, 1.0f };
		mDirLights[2].Direction = { 0.0f, -0.707f, -0.707f };

		mLandMat.Ambient = { 0.5f, 0.5f, 0.5f, 1.0f };
		mLandMat.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
		mLandMat.Specular = { 0.2f, 0.2f, 0.2f, 16.0f };

		mWavesMat.Ambient = { 0.5f, 0.5f, 0.5f, 1.0f };
		mWavesMat.Diffuse = { 1.0f, 1.0f, 1.0f, 0.5f };
		mWavesMat.Specular = { 0.8f, 0.8f, 0.8f, 32.0f };

		mBoxMat.Ambient = { 0.5f, 0.5f, 0.5f, 1.0f };
		mBoxMat.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
		mBoxMat.Specular = { 0.4f, 0.4f, 0.4f, 16.0f };

		mTreeMat.Ambient = { 0.5f, 0.5f, 0.5f, 1.0f };
		mTreeMat.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
		mTreeMat.Specular = { 0.2f, 0.2f, 0.2f, 16.0f };
	}
	D3DSample::~D3DSample()
	{
		ReleaseCOM(mPerFrameCB);

		ReleaseCOM(mPerObjectCBBasic32);
		ReleaseCOM(mInputLayoutBasic32);
		ReleaseCOM(mVertexShaderBasic32);
		ReleaseCOM(mVertexShaderBlobBasic32);
		ReleaseCOM(mPixelShaderBasic32);

		ReleaseCOM(mPerObjectCBSprite);
		ReleaseCOM(mInputLayoutSprite);
		ReleaseCOM(mVertexShaderSprite);
		ReleaseCOM(mVertexShaderBlobSprite);
		ReleaseCOM(mGeometryShader);
		ReleaseCOM(mPixelShaderSprite);

		ReleaseCOM(mLinearSamplerWrap);
		ReleaseCOM(mLinearSamplerClamp);

		ReleaseCOM(mLandVB);
		ReleaseCOM(mLandIB);

		ReleaseCOM(mWavesVB);
		ReleaseCOM(mWavesIB);

		ReleaseCOM(mBoxVB);
		ReleaseCOM(mBoxIB);

		ReleaseCOM(mTreeSpritesVB);

		ReleaseCOM(mGrassMapSRV);
		ReleaseCOM(mWavesMapSRV);
		ReleaseCOM(mBoxMapSRV);
		ReleaseCOM(mTreeTextureMapArraySRV);

		RenderStates::Destroy();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		RenderStates::Init(md3dDevice);

		// 텍스처 로딩
		HR(CreateDDSTextureFromFile(md3dDevice,
			L"../Resource/Textures/grass.dds", NULL, &mGrassMapSRV));

		HR(CreateDDSTextureFromFile(md3dDevice,
			L"../Resource/Textures/water2.dds", NULL, &mWavesMapSRV));

		HR(CreateDDSTextureFromFile(md3dDevice,
			L"../Resource/Textures/WireFence.dds", NULL, &mBoxMapSRV));

		std::vector<std::wstring> treeFilenames;
		treeFilenames.push_back(L"../Resource/Textures/tree3.dds");
		treeFilenames.push_back(L"../Resource/Textures/tree1.dds");
		treeFilenames.push_back(L"../Resource/Textures/tree2.dds");
		treeFilenames.push_back(L"../Resource/Textures/tree3.dds");

		mTreeTextureMapArraySRV = D3DHelper::CreateTexture2DArraySRV(md3dDevice, md3dContext, treeFilenames);

		buildConstantBuffer();
		buildShader();
		buildInputLayout();

		buildLandGeometryBuffers();
		buildWaveGeometryBuffers();
		buildCrateGeometryBuffers();
		buildTreeSpritesBuffer();

		return true;
	}
	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mProj = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	}

	void D3DSample::Update(float deltaTime)
	{
		// Convert Spherical to Cartesian coordinates.
		float x = mRadius * sinf(mPhi) * cosf(mTheta);
		float z = mRadius * sinf(mPhi) * sinf(mTheta);
		float y = mRadius * cosf(mPhi);

		mEyePosW = { x, y, z };

		// Build the view matrix.
		Vector4 pos = XMVectorSet(x, y, z, 1.0f);
		Vector4 target = XMVectorZero();
		Vector4 up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		mView = XMMatrixLookAtLH(pos, target, up);

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

		vertex::Basic32* v = reinterpret_cast<vertex::Basic32*>(mappedData.pData);
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

		mWaterTexTransform = wavesScale * wavesOffset;
	}

	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		md3dContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&common::Silver));
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		float blendFactor[] = { 0.f, 0.f, 0.f, 0.f };

		drawTreeSprites(mView * mProj);

		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dContext->IASetInputLayout(mInputLayoutBasic32);

		UINT stride = sizeof(vertex::Basic32);
		UINT offset = 0;

		mCBPerFrame.bUseTexutre = true;
		memcpy(mCBPerFrame.DirLight, mDirLights, sizeof(mCBPerFrame.DirLight));
		mCBPerFrame.EyePosW = mEyePosW;
		mCBPerFrame.FogColor = common::Silver;
		mCBPerFrame.FogStart = 15.f;
		mCBPerFrame.FogRange = 175.f;

		md3dContext->UpdateSubresource(mPerFrameCB, 0, NULL, &mCBPerFrame, 0, 0);

		auto bindBuffer = [&](ID3D11Buffer* VB, ID3D11Buffer* IB)
			{
				md3dContext->IASetVertexBuffers(0, 1, &VB, &stride, &offset);
				md3dContext->IASetIndexBuffer(IB, DXGI_FORMAT_R32_UINT, 0);
			};

		auto applyCB = [&](const Matrix& world, const Material& mat, const Matrix& tex, ID3D11ShaderResourceView* SRV)
			{
				mCBPerObjectBasic32.World = world;
				mCBPerObjectBasic32.WorldInvTranspose = MathHelper::InverseTranspose(world);
				mCBPerObjectBasic32.WorldViewProj = world * mView * mProj;
				mCBPerObjectBasic32.Material = mat;
				mCBPerObjectBasic32.Tex = tex;

				mCBPerObjectBasic32.World = mCBPerObjectBasic32.World.Transpose();
				mCBPerObjectBasic32.WorldInvTranspose = mCBPerObjectBasic32.WorldInvTranspose.Transpose();
				mCBPerObjectBasic32.WorldViewProj = mCBPerObjectBasic32.WorldViewProj.Transpose();

				md3dContext->UpdateSubresource(mPerObjectCBBasic32, 0, NULL, &mCBPerObjectBasic32, 0, 0);
				md3dContext->PSSetShaderResources(0, 1, &SRV);
			};

		setShader(mVertexShaderBasic32, nullptr, mPixelShaderBasic32);

		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCBBasic32);

		md3dContext->PSSetSamplers(0, 1, &mLinearSamplerWrap);
		md3dContext->PSSetConstantBuffers(0, 1, &mPerFrameCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mPerObjectCBBasic32);


		bindBuffer(mBoxVB, mBoxIB);
		applyCB(mBoxWorld, mBoxMat, Matrix::Identity, mBoxMapSRV);
		md3dContext->RSSetState(RenderStates::NoCullRS); // 컬링 안함
		md3dContext->DrawIndexed(36, 0, 0);
		md3dContext->RSSetState(NULL);

		bindBuffer(mLandVB, mLandIB);
		applyCB(mLandWorld, mLandMat, mGrassTexTransform, mGrassMapSRV);
		md3dContext->DrawIndexed(mLandIndexCount, 0, 0);

		bindBuffer(mWavesVB, mWavesIB);
		applyCB(mWavesWorld, mWavesMat, mWaterTexTransform, mWavesMapSRV);

		md3dContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		md3dContext->DrawIndexed(3 * mWaves.GetTriangleCount(), 0, 0);
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
			// Make each pixel correspond to 0.01 unit in the scene.
			float dx = 0.1f * static_cast<float>(x - mLastMousePos.x);
			float dy = 0.1f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = MathHelper::Clamp(mRadius, 20.0f, 500.0f);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

	void D3DSample::setShader(ID3D11VertexShader* VS, ID3D11GeometryShader* GS, ID3D11PixelShader* PS)
	{
		md3dContext->VSSetShader(VS, NULL, 0);
		md3dContext->GSSetShader(GS, NULL, 0);
		md3dContext->PSSetShader(PS, NULL, 0);
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
		md3dDevice->CreateBuffer(&cbd, NULL, &mPerFrameCB);

		static_assert(sizeof(mCBPerObjectBasic32) % 16 == 0, "error");
		cbd.ByteWidth = sizeof(mCBPerObjectBasic32);
		md3dDevice->CreateBuffer(&cbd, NULL, &mPerObjectCBBasic32);

		static_assert(sizeof(mCBPerObjectSprite) % 16 == 0, "error");
		cbd.ByteWidth = sizeof(mCBPerObjectSprite);
		md3dDevice->CreateBuffer(&cbd, NULL, &mPerObjectCBSprite);

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

		md3dDevice->CreateSamplerState(&samplerDesc, &mLinearSamplerWrap);

		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		md3dDevice->CreateSamplerState(&samplerDesc, &mLinearSamplerClamp);

		HR(D3DHelper::CompileShaderFromFile(L"../Resource/Shader/StencilingVS.hlsl", "main", "vs_5_0", &mVertexShaderBlobBasic32));
		md3dDevice->CreateVertexShader(mVertexShaderBlobBasic32->GetBufferPointer()
			, mVertexShaderBlobBasic32->GetBufferSize()
			, NULL
			, &mVertexShaderBasic32);

		HR(D3DHelper::CompileShaderFromFile(L"../Resource/Shader/BillboardVS.hlsl", "main", "vs_5_0", &mVertexShaderBlobSprite));
		md3dDevice->CreateVertexShader(mVertexShaderBlobSprite->GetBufferPointer()
			, mVertexShaderBlobSprite->GetBufferSize()
			, NULL
			, &mVertexShaderSprite);

		ID3DBlob* geoShaderBlob = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"../Resource/Shader/BillboardGS.hlsl", "main", "gs_5_0", &geoShaderBlob));
		md3dDevice->CreateGeometryShader(geoShaderBlob->GetBufferPointer()
			, geoShaderBlob->GetBufferSize()
			, NULL
			, &mGeometryShader);

		ID3DBlob* pixelShaderBlob = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"../Resource/Shader/StencilingPS.hlsl", "main", "ps_5_0", &pixelShaderBlob));
		md3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer()
			, pixelShaderBlob->GetBufferSize()
			, NULL
			, &mPixelShaderBasic32);

		HR(D3DHelper::CompileShaderFromFile(L"../Resource/Shader/BillboardPS.hlsl", "main", "ps_5_0", &pixelShaderBlob));
		md3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer()
			, pixelShaderBlob->GetBufferSize()
			, NULL
			, &mPixelShaderSprite);
	}
	void D3DSample::buildInputLayout()
	{
		D3D11_INPUT_ELEMENT_DESC basic32InputDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		md3dDevice->CreateInputLayout(basic32InputDesc
			, ARRAYSIZE(basic32InputDesc)
			, mVertexShaderBlobBasic32->GetBufferPointer()
			, mVertexShaderBlobBasic32->GetBufferSize()
			, &mInputLayoutBasic32);

		D3D11_INPUT_ELEMENT_DESC spriteInputDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		md3dDevice->CreateInputLayout(spriteInputDesc
			, ARRAYSIZE(spriteInputDesc)
			, mVertexShaderBlobSprite->GetBufferPointer()
			, mVertexShaderBlobSprite->GetBufferSize()
			, &mInputLayoutSprite);
	}

	float D3DSample::getHillHeight(float x, float z) const
	{
		return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
	}
	Vector3 D3DSample::getHillNormal(float x, float z) const
	{
		Vector3 n(
			-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
			1.0f,
			-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));
		n.Normalize();

		return n;
	}
	void D3DSample::buildLandGeometryBuffers()
	{
		GeometryGenerator::MeshData grid;

		GeometryGenerator geoGen;

		geoGen.CreateGrid(160.0f, 160.0f, 50, 50, &grid);

		mLandIndexCount = grid.Indices.size();

		std::vector<vertex::Basic32> vertices(grid.Vertices.size());
		for (size_t i = 0; i < grid.Vertices.size(); ++i)
		{
			Vector3 p = grid.Vertices[i].Position;

			p.y = getHillHeight(p.x, p.z);

			vertices[i].Pos = p;
			vertices[i].Normal = getHillNormal(p.x, p.z);
			vertices[i].Tex = grid.Vertices[i].TexC;
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(vertex::Basic32) * grid.Vertices.size();
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
		mWaves.Init(160, 160, 1.0f, 0.03f, 3.25f, 0.4f);

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_DYNAMIC;
		vbd.ByteWidth = sizeof(vertex::Basic32) * mWaves.GetVertexCount();
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

				k += 6; // next quad
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

		std::vector<vertex::Basic32> vertices(box.Vertices.size());

		for (UINT i = 0; i < box.Vertices.size(); ++i)
		{
			vertices[i].Pos = box.Vertices[i].Position;
			vertices[i].Normal = box.Vertices[i].Normal;
			vertices[i].Tex = box.Vertices[i].TexC;
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(vertex::Basic32) * box.Vertices.size();
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
	void D3DSample::buildTreeSpritesBuffer()
	{
		vertex::Sprite v[TreeCount];

		for (UINT i = 0; i < TreeCount; ++i)
		{
			float x = MathHelper::RandF(-35.0f, 35.0f);
			float z = MathHelper::RandF(-35.0f, 35.0f);
			float y = getHillHeight(x, z);

			y += 10.0f;

			v[i].Pos = XMFLOAT3(x, y, z);
			v[i].Size = XMFLOAT2(24.0f, 24.0f);
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(vertex::Sprite) * TreeCount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = v;
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mTreeSpritesVB));
	}
	void D3DSample::drawTreeSprites(Matrix viewProj)
	{
		UINT stride = sizeof(vertex::Sprite);
		UINT offset = 0;

		mCBPerObjectSprite.ViewProj = viewProj.Transpose();
		mCBPerObjectSprite.Material = mTreeMat;
		md3dContext->UpdateSubresource(mPerObjectCBSprite, NULL, 0, &mCBPerObjectSprite, 0, 0);

		md3dContext->IASetVertexBuffers(0, 1, &mTreeSpritesVB, &stride, &offset);
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		md3dContext->IASetInputLayout(mInputLayoutSprite);

		setShader(mVertexShaderSprite, mGeometryShader, mPixelShaderSprite);
		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCBSprite);

		md3dContext->GSSetConstantBuffers(0, 1, &mPerObjectCBSprite);
		md3dContext->GSSetConstantBuffers(1, 1, &mPerFrameCB);

		md3dContext->PSSetSamplers(0, 1, &mLinearSamplerClamp);
		md3dContext->PSSetConstantBuffers(0, 1, &mPerFrameCB);
		md3dContext->PSSetShaderResources(0, 1, &mTreeTextureMapArraySRV);

		float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		md3dContext->OMSetBlendState(RenderStates::AlphaToCoverageBS, blendFactor, 0xffffffff);
		md3dContext->Draw(TreeCount, 0);
		md3dContext->OMSetBlendState(NULL, blendFactor, 0xffffffff);
	}
}