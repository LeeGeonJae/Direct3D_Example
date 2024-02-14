#include <cassert>
#include <fstream>
#include <vector>
#include <directxtk/DDSTextureLoader.h>
#include <directxtk/WICTextureLoader.h>

#include "D3DSample.h"
#include "RenderStates.h"

namespace stenciling
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mTheta(1.24f * MathHelper::Pi)
		, mPhi(0.42f * MathHelper::Pi)
		, mRadius(12.0f)
		, mSkullTranslation(0.0f, 1.0f, -5.0f)
	{
		mEnable4xMsaa = false;

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mRoomWorld = Matrix::Identity;
		mView = Matrix::Identity;
		mProj = Matrix::Identity;

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

		mRoomMat.Ambient = { 0.5f, 0.5f, 0.5f, 1.0f };
		mRoomMat.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
		mRoomMat.Specular = { 0.4f, 0.4f, 0.4f, 16.0f };

		mSkullMat.Ambient = { 0.5f, 0.5f, 0.5f, 1.0f };
		mSkullMat.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
		mSkullMat.Specular = { 0.4f, 0.4f, 0.4f, 16.0f };

		mMirrorMat.Ambient = { 0.5f, 0.5f, 0.5f, 1.0f };
		mMirrorMat.Diffuse = { 1.0f, 1.0f, 1.0f, 0.5f };
		mMirrorMat.Specular = { 0.4f, 0.4f, 0.4f, 16.0f };

		mShadowMat.Ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
		mShadowMat.Diffuse = { 0.0f, 0.0f, 0.0f, 0.5f };
		mShadowMat.Specular = { 0.0f, 0.0f, 0.0f, 16.0f };
	}

	D3DSample::~D3DSample()
	{
		ReleaseCOM(mPerObjectCB);
		ReleaseCOM(mPerFrameCB);

		ReleaseCOM(mVertexShader);
		ReleaseCOM(mVertexShaderBlob);
		ReleaseCOM(mPixelShader);
		ReleaseCOM(mInputLayout);

		ReleaseCOM(mLinearSampler);

		ReleaseCOM(mRoomVB);

		ReleaseCOM(mSkullVB);
		ReleaseCOM(mSkullIB);

		ReleaseCOM(mFloorDiffuseMapSRV);
		ReleaseCOM(mWallDiffuseMapSRV);
		ReleaseCOM(mMirrorDiffuseMapSRV);

		RenderStates::Destroy();
	}
	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		RenderStates::Init(md3dDevice);

		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/checkboard.dds", NULL, &mFloorDiffuseMapSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/brick01.dds", NULL, &mWallDiffuseMapSRV));
		HR(DirectX::CreateDDSTextureFromFile(md3dDevice, L"../Resource/Textures/ice.dds", NULL, &mMirrorDiffuseMapSRV));

		buildConstantBuffer();
		buildShader();
		buildInputLayout();

		buildRoomGeometryBuffers();
		buildSkullGeometryBuffers();

		return true;
	}
	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mProj = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
	}

	void D3DSample::Update(float deltaTime)
	{
		float x = mRadius * sinf(mPhi) * cosf(mTheta);
		float z = mRadius * sinf(mPhi) * sinf(mTheta);
		float y = mRadius * cosf(mPhi);

		mEyePosW = { x, y, z };

		Vector4 pos = { x, y, z, 1.0f };
		Vector4 target = Vector4::Zero;
		Vector4 up = { 0.0f, 1.0f, 0.0f, 0.0f };

		mView = XMMatrixLookAtLH(pos, target, up);

		mSkullTranslation.y = MathHelper::Max(mSkullTranslation.y, 0.0f);

		static float s_time = 0.f;
		s_time += 0.001;

		Matrix skullRotate = Matrix::CreateRotationY(s_time);
		//Matrix skullRotate = Matrix::CreateRotationY(0.5f * MathHelper::Pi);
		Matrix skullScale = Matrix::CreateScale(0.45f, 0.45f, 0.45f);
		Matrix skullOffset = Matrix::CreateTranslation(mSkullTranslation.x, mSkullTranslation.y, mSkullTranslation.z);
		mSkullWorld = skullRotate * skullScale * skullOffset;
	}

	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };

		md3dContext->ClearRenderTargetView(mRenderTargetView, color);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		// 인풋 레이아웃, 버택스, 인덱스, 토폴로지
		md3dContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dContext->IASetInputLayout(mInputLayout);

		// 버택스 쉐이더, 상수버퍼
		md3dContext->VSSetShader(mVertexShader, NULL, 0);
		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCB);

		// 픽셀 쉐이더, 쉐이더리소스뷰, 샘플러스테이트, 상수버퍼
		md3dContext->PSSetShader(mPixelShader, NULL, 0);
		md3dContext->PSSetConstantBuffers(0, 1, &mPerFrameCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mPerObjectCB);
		md3dContext->PSSetSamplers(0, 1, &mLinearSampler);

		memcpy(mCBPerFrame.DirLight, mDirLights, sizeof(mCBPerFrame.DirLight));
		mCBPerFrame.EyePosW = mEyePosW;
		mCBPerFrame.LigthCount = 3;
		mCBPerFrame.FogColor = common::Black;
		mCBPerFrame.FogStart = 2.f;
		mCBPerFrame.FogRange = 40.f;
		mCBPerFrame.bUseTexutre = true;
		mCBPerFrame.bUseLight = true;
		mCBPerFrame.bUseFog = true;
		md3dContext->UpdateSubresource(mPerFrameCB, 0, NULL, &mCBPerFrame, 0, 0);

		// 벽이랑 바닥 해골 그냥 그리기
		UINT stride = sizeof(Vertex);
		UINT offset = 0u;

		md3dContext->IASetVertexBuffers(0, 1, &mRoomVB, &stride, &offset);
		updateCBPerObject(mRoomWorld, Matrix::Identity, mRoomMat);
		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);
		md3dContext->PSSetShaderResources(0, 1, &mFloorDiffuseMapSRV);
		md3dContext->Draw(6, 0);

		md3dContext->PSSetShaderResources(0, 1, &mWallDiffuseMapSRV);
		md3dContext->Draw(18, 6);

		md3dContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);
		updateCBPerObject(mSkullWorld, Matrix::Identity, mSkullMat);
		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);
		mCBPerFrame.bUseTexutre = false;
		md3dContext->UpdateSubresource(mPerFrameCB, 0, NULL, &mCBPerFrame, 0, 0);
		md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);

		// 미러를 스텐실에 그리기
		md3dContext->IASetVertexBuffers(0, 1, &mRoomVB, &stride, &offset);

		updateCBPerObject(mRoomWorld, Matrix::Identity, mRoomMat);
		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);

		float blendFactor[] = { 0.f, 0.f, 0.f, 0.f };
		md3dContext->OMSetBlendState(RenderStates::NoRenderTargetWritesBS, blendFactor, 0xff'ff'ff'ff);
		md3dContext->OMSetDepthStencilState(RenderStates::MarkMirrorDSS, 1); // 깊이 판정이 참인 경우 1을 쓴다.
		md3dContext->Draw(6, 24);
		md3dContext->OMSetDepthStencilState(NULL, 0);
		md3dContext->OMSetBlendState(NULL, blendFactor, 0xff'ff'ff'ff);

		// 반사된 해골 그리기
		md3dContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

		Plane mirrorPlane(0.f, 0.f, 1.f, 0.f);
		Matrix r = DirectX::XMMatrixReflect(mirrorPlane);
		updateCBPerObject(mSkullWorld * r, Matrix::Identity, mSkullMat);
		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);

		mCBPerFrame.bUseTexutre = false;
		mCBPerFrame.DirLight[0].Direction = DirectX::XMVector3TransformNormal(mCBPerFrame.DirLight[0].Direction, r);
		mCBPerFrame.DirLight[1].Direction = DirectX::XMVector3TransformNormal(mCBPerFrame.DirLight[1].Direction, r);
		mCBPerFrame.DirLight[2].Direction = DirectX::XMVector3TransformNormal(mCBPerFrame.DirLight[2].Direction, r);
		md3dContext->UpdateSubresource(mPerFrameCB, 0, NULL, &mCBPerFrame, 0, 0);

		md3dContext->RSSetState(RenderStates::CullClockwiseRS); // 반시계방향을 전면으로 가정한다.
		md3dContext->OMSetDepthStencilState(RenderStates::DrawReflectionDSS, 1); // 스텐실이 1과 같으면 렌더링한다.
		md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);
		md3dContext->RSSetState(NULL);
		md3dContext->OMSetDepthStencilState(NULL, 0);

		// 반사된 바닥 그리기
		md3dContext->IASetVertexBuffers(0, 1, &mRoomVB, &stride, &offset);
		updateCBPerObject(mRoomWorld * r, Matrix::Identity, mRoomMat);
		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);
		md3dContext->PSSetShaderResources(0, 1, &mWallDiffuseMapSRV);
		mCBPerFrame.bUseTexutre = true;
		md3dContext->UpdateSubresource(mPerFrameCB, 0, NULL, &mCBPerFrame, 0, 0);

		md3dContext->RSSetState(RenderStates::CullClockwiseRS); // 반시계방향을 전면으로 가정한다.
		md3dContext->OMSetDepthStencilState(RenderStates::DrawReflectionDSS, 1); // 스텐실이 1과 같으면 렌더링한다.
		md3dContext->PSSetShaderResources(0, 1, &mFloorDiffuseMapSRV);
		md3dContext->Draw(6, 0);
		md3dContext->RSSetState(NULL);
		md3dContext->OMSetDepthStencilState(NULL, 0);

		// 투명도 혼합
		md3dContext->IASetVertexBuffers(0, 1, &mRoomVB, &stride, &offset);

		updateCBPerObject(mRoomWorld, Matrix::Identity, mMirrorMat);
		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);

		mCBPerFrame.bUseTexutre = true;
		md3dContext->UpdateSubresource(mPerFrameCB, 0, NULL, &mCBPerFrame, 0, 0);

		md3dContext->PSSetShaderResources(0, 1, &mMirrorDiffuseMapSRV);

		md3dContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		md3dContext->Draw(6, 24);
		md3dContext->OMSetBlendState(NULL, blendFactor, 0xffffffff);

		// 그림자
		md3dContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

		Plane shadowPlane(0.f, 1.f, 0.f, 0.f);
		Matrix s = DirectX::XMMatrixShadow(shadowPlane, -mDirLights[0].Direction);
		Matrix shadowOffsetY = Matrix::CreateTranslation(0.f, 0.001f, 0.0f);
		updateCBPerObject(mSkullWorld * s * shadowOffsetY, Matrix::Identity, mShadowMat);
		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);

		md3dContext->OMSetDepthStencilState(RenderStates::NoDoubleBlendDSS, 0);
		md3dContext->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);
		md3dContext->OMSetBlendState(NULL, blendFactor, 0xffffffff);
		md3dContext->OMSetDepthStencilState(NULL, 0);

		mSwapChain->Present(0, 0);
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
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			// Update angles based on input to orbit camera around box.
			mTheta += dx;
			mPhi += dy;

			// Restrict the angle mPhi.
			mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		}
		else if ((btnState & MK_RBUTTON) != 0)
		{
			// Make each pixel correspond to 0.01 unit in the scene.
			float dx = 0.01f * static_cast<float>(x - mLastMousePos.x);
			float dy = 0.01f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = MathHelper::Clamp(mRadius, 3.0f, 50.0f);
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

		static_assert(sizeof(CBPerObject) % 16 == 0, "error");
		cbd.ByteWidth = sizeof(CBPerObject);
		md3dDevice->CreateBuffer(&cbd, NULL, &mPerObjectCB);
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

		HR(CompileShaderFromFile(L"../Resource/Shader/StencilingVS.hlsl", "main", "vs_5_0", &mVertexShaderBlob));
		md3dDevice->CreateVertexShader(mVertexShaderBlob->GetBufferPointer(), mVertexShaderBlob->GetBufferSize(), NULL, &mVertexShader);

		ID3DBlob* pixelShaderBlob = nullptr;
		HR(CompileShaderFromFile(L"../Resource/Shader/StencilingPS.hlsl", "main", "ps_5_0", &pixelShaderBlob));

		md3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), NULL, &mPixelShader);
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

	void D3DSample::buildRoomGeometryBuffers()
	{
		//   |--------------|
		//   |              |
		//   |----|----|----|
		//   |Wall|Mirr|Wall|
		//   |    | or |    |
		//   /--------------/
		//  /   Floor      /
		// /--------------/

		Vertex v[30];

		// Floor: Observe we tile texture coordinates.
		v[0] = Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
		v[1] = Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		v[2] = Vertex(7.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);

		v[3] = Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
		v[4] = Vertex(7.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
		v[5] = Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f);

		// Wall: Observe we tile texture coordinates, and that we
		// leave a gap in the middle for the mirror.
		v[6] = Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
		v[7] = Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
		v[8] = Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);

		v[9] = Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
		v[10] = Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);
		v[11] = Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f);

		v[12] = Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
		v[13] = Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
		v[14] = Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);

		v[15] = Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
		v[16] = Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
		v[17] = Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f);

		v[18] = Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
		v[19] = Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
		v[20] = Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);

		v[21] = Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
		v[22] = Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);
		v[23] = Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f);

		// Mirror
		v[24] = Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
		v[25] = Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
		v[26] = Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

		v[27] = Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
		v[28] = Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
		v[29] = Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertex) * 30;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = v;
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mRoomVB));
	}
	void D3DSample::buildSkullGeometryBuffers()
	{
		std::ifstream fin("../Resource/Models/skull.txt");

		if (!fin)
		{
			MessageBox(0, L"file not found", 0, 0);
			return;
		}

		UINT vcount = 0;
		UINT tcount = 0;
		std::string ignore;

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		std::vector<Vertex> vertices(vcount);
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
		vbd.ByteWidth = sizeof(Vertex) * vcount;
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
