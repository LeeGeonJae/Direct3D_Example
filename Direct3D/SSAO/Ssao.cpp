#include <vector>
#include <cassert>

#include "Ssao.h"
#include "Camera.h"
#include "D3DSample.h"

namespace ssao
{
	Ssao::Ssao(ID3D11Device* device, ID3D11DeviceContext* dc, int width, int height, float fovy, float farZ)
		: md3dDevice(device)
		, mDC(dc)
		, mScreenQuadVB(0)
		, mScreenQuadIB(0)
		, mRandomVectorSRV(0)
		, mNormalDepthRTV(0)
		, mNormalDepthSRV(0)
		, mAmbientRTV0(0)
		, mAmbientSRV0(0)
		, mAmbientRTV1(0)
		, mAmbientSRV1(0)

	{
		OnSize(width, height, fovy, farZ);

		BuildFullScreenQuad();
		BuildOffsetVectors();
		BuildRandomVectorTexture();
	}

	Ssao::~Ssao()
	{
		ReleaseCOM(mScreenQuadVB);
		ReleaseCOM(mScreenQuadIB);
		ReleaseCOM(mRandomVectorSRV);
		ReleaseCOM(mNormalDepthRTV);
		ReleaseCOM(mNormalDepthSRV);
		ReleaseCOM(mAmbientRTV0);
		ReleaseCOM(mAmbientSRV0);
		ReleaseCOM(mAmbientRTV1);
		ReleaseCOM(mAmbientSRV1);

		ReleaseTextureViews();
	}

	ID3D11ShaderResourceView* Ssao::NormalDepthSRV()
	{
		return mNormalDepthSRV;
	}

	ID3D11ShaderResourceView* Ssao::AmbientSRV()
	{
		return mAmbientSRV0;
	}

	void Ssao::OnSize(int width, int height, float fovy, float farZ)
	{
		mRenderTargetWidth = width;
		mRenderTargetHeight = height;

		// We render to ambient map at half the resolution.
		mAmbientMapViewport.TopLeftX = 0.0f;
		mAmbientMapViewport.TopLeftY = 0.0f;
		mAmbientMapViewport.Width = width / 2.0f;
		mAmbientMapViewport.Height = height / 2.0f;
		mAmbientMapViewport.MinDepth = 0.0f;
		mAmbientMapViewport.MaxDepth = 1.0f;

		BuildFrustumFarCorners(fovy, farZ);
		BuildTextureViews();
	}

	void Ssao::SetNormalDepthRenderTarget(ID3D11DepthStencilView* dsv)
	{
		ID3D11RenderTargetView* renderTargets[1] = { mNormalDepthRTV };
		mDC->OMSetRenderTargets(1, renderTargets, dsv);

		// Clear view space normal to (0,0,-1) and clear depth to be very far away.  
		float clearColor[] = { 0.0f, 0.0f, -1.0f, 1e5f };
		mDC->ClearRenderTargetView(mNormalDepthRTV, clearColor);
	}

	void Ssao::ComputeSsao(const Camera& camera, D3DSample& sample)
	{
		// ·»´õ Å¸°Ù°ú ºäÆ÷Æ® ¼³Á¤
		ID3D11RenderTargetView* renderTargets[1] = { mAmbientRTV0 };
		mDC->OMSetRenderTargets(1, renderTargets, 0);
		mDC->ClearRenderTargetView(mAmbientRTV0, reinterpret_cast<const float*>(&Black));
		mDC->RSSetViewports(1, &mAmbientMapViewport);

		// ÀÚ¿ø ¹ÙÀÎµù
		auto VS = sample.mVSs.find("ssao");
		assert(VS != sample.mVSs.end());
		mDC->VSSetShader(VS->second, 0, 0);
		auto PS = sample.mPSs.find("ssao");
		assert(PS != sample.mPSs.end());
		mDC->PSSetShader(PS->second, 0, 0);
		auto CB0 = sample.mBuffers.find("CBFrameSsao");
		assert(CB0 != sample.mBuffers.end());
		mDC->VSSetConstantBuffers(0, 1, &(CB0->second));
		mDC->PSSetConstantBuffers(0, 1, &(CB0->second));
		auto SS0 = sample.mSamplerStates.find("pointBorder");
		auto SS1 = sample.mSamplerStates.find("linearWrap");
		assert(SS0 != sample.mSamplerStates.end());
		assert(SS1 != sample.mSamplerStates.end());
		mDC->PSSetSamplers(0, 1, &(SS0->second));
		mDC->PSSetSamplers(1, 1, &(SS1->second));

		// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
		static const Matrix T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);

		Matrix P = camera.GetProj();
		Matrix PT = P * T;

		sample.mFrameSsao.ViewToTexSpace = PT.Transpose();
		memcpy(sample.mFrameSsao.OffsetVectors, mOffsets, sizeof(sample.mFrameSsao.OffsetVectors));
		memcpy(sample.mFrameSsao.FrustumCorners, mFrustumFarCorner, sizeof(sample.mFrameSsao.FrustumCorners));
		mDC->UpdateSubresource(CB0->second, 0, 0, &(sample.mFrameSsao), 0, 0);

		sample.md3dContext->PSSetShaderResources(0, 1, &mNormalDepthSRV);
		sample.md3dContext->PSSetShaderResources(1, 1, &mRandomVectorSRV);

		UINT stride = sizeof(Basic32);
		UINT offset = 0;

		auto inputlayout = sample.mInputLayouts.find("basic32");
		mDC->IASetInputLayout(inputlayout->second);
		mDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mDC->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
		mDC->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);
		mDC->DrawIndexed(6, 0, 0);
	}

	void Ssao::BlurAmbientMap(int blurCount, D3DSample& sample)
	{
		for (int i = 0; i < blurCount; ++i)
		{
			// Ping-pong the two ambient map textures as we apply
			// horizontal and vertical blur passes.
			blurAmbientMap(mAmbientSRV0, mAmbientRTV1, true, sample);
			blurAmbientMap(mAmbientSRV1, mAmbientRTV0, false, sample);
		}
	}

	void Ssao::blurAmbientMap(ID3D11ShaderResourceView* inputSRV, ID3D11RenderTargetView* outputRTV, bool horzBlur, D3DSample& sample)
	{
		ID3D11RenderTargetView* renderTargets[1] = { outputRTV };
		mDC->OMSetRenderTargets(1, renderTargets, 0);
		mDC->ClearRenderTargetView(outputRTV, reinterpret_cast<const float*>(&Black));
		mDC->RSSetViewports(1, &mAmbientMapViewport);

		auto VS = sample.mVSs.find("ssaoBlur");
		assert(VS != sample.mVSs.end());
		mDC->VSSetShader(VS->second, 0, 0);
		auto PS = sample.mPSs.find("ssaoBlur");
		assert(PS != sample.mPSs.end());
		mDC->PSSetShader(PS->second, 0, 0);
		auto CB0 = sample.mBuffers.find("CBFrameSsaoBlur");
		assert(CB0 != sample.mBuffers.end());
		mDC->VSSetConstantBuffers(0, 1, &(CB0->second));
		mDC->PSSetConstantBuffers(0, 1, &(CB0->second));
		auto SS0 = sample.mSamplerStates.find("linearWrap");
		assert(SS0 != sample.mSamplerStates.end());
		mDC->PSSetSamplers(0, 1, &(SS0->second));

		sample.mFrameSsaoBlur.TexelWidth = 1.f / mAmbientMapViewport.Width;
		sample.mFrameSsaoBlur.TexelHeight = 1.f / mAmbientMapViewport.Height;
		sample.mFrameSsaoBlur.bHorizontalBlur = horzBlur;
		mDC->UpdateSubresource(CB0->second, 0, 0, &(sample.mFrameSsaoBlur), 0, 0);

		sample.md3dContext->PSSetShaderResources(0, 1, &mNormalDepthSRV);
		sample.md3dContext->PSSetShaderResources(1, 1, &inputSRV);

		UINT stride = sizeof(Basic32);
		UINT offset = 0;

		auto inputLayout = sample.mInputLayouts.find("basic32");
		mDC->IASetInputLayout(inputLayout->second);
		mDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mDC->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
		mDC->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);
		mDC->DrawIndexed(6, 0, 0);

		ID3D11ShaderResourceView* nullSRV = { nullptr };
		sample.md3dContext->PSSetShaderResources(1, 1, &nullSRV);
	}

	void Ssao::BuildFrustumFarCorners(float fovy, float farZ)
	{
		float aspect = (float)mRenderTargetWidth / (float)mRenderTargetHeight;

		float halfHeight = farZ * tanf(0.5f * fovy);
		float halfWidth = aspect * halfHeight;

		mFrustumFarCorner[0] = Vector4(-halfWidth, -halfHeight, farZ, 0.0f);
		mFrustumFarCorner[1] = Vector4(-halfWidth, +halfHeight, farZ, 0.0f);
		mFrustumFarCorner[2] = Vector4(+halfWidth, +halfHeight, farZ, 0.0f);
		mFrustumFarCorner[3] = Vector4(+halfWidth, -halfHeight, farZ, 0.0f);
	}

	void Ssao::BuildFullScreenQuad()
	{
		Basic32 v[4];

		v[0].Pos = XMFLOAT3(-1.0f, -1.0f, 0.0f);
		v[1].Pos = XMFLOAT3(-1.0f, +1.0f, 0.0f);
		v[2].Pos = XMFLOAT3(+1.0f, +1.0f, 0.0f);
		v[3].Pos = XMFLOAT3(+1.0f, -1.0f, 0.0f);

		// Store far plane frustum corner indices in Normal.x slot.
		v[0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		v[1].Normal = XMFLOAT3(1.0f, 0.0f, 0.0f);
		v[2].Normal = XMFLOAT3(2.0f, 0.0f, 0.0f);
		v[3].Normal = XMFLOAT3(3.0f, 0.0f, 0.0f);

		v[0].Tex = XMFLOAT2(0.0f, 1.0f);
		v[1].Tex = XMFLOAT2(0.0f, 0.0f);
		v[2].Tex = XMFLOAT2(1.0f, 0.0f);
		v[3].Tex = XMFLOAT2(1.0f, 1.0f);

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32) * 4;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = v;
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mScreenQuadVB));

		USHORT indices[6] =
		{
			0, 1, 2,
			0, 2, 3
		};

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(USHORT) * 6;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.StructureByteStride = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = indices;
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mScreenQuadIB));
	}

	void Ssao::BuildTextureViews()
	{
		ReleaseTextureViews();

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = mRenderTargetWidth;
		texDesc.Height = mRenderTargetHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;


		ID3D11Texture2D* normalDepthTex = 0;
		HR(md3dDevice->CreateTexture2D(&texDesc, 0, &normalDepthTex));
		HR(md3dDevice->CreateShaderResourceView(normalDepthTex, 0, &mNormalDepthSRV));
		HR(md3dDevice->CreateRenderTargetView(normalDepthTex, 0, &mNormalDepthRTV));

		// view saves a reference.
		ReleaseCOM(normalDepthTex);

		// Render ambient map at half resolution.
		texDesc.Width = mRenderTargetWidth / 2;
		texDesc.Height = mRenderTargetHeight / 2;
		texDesc.Format = DXGI_FORMAT_R16_FLOAT;
		ID3D11Texture2D* ambientTex0 = 0;
		HR(md3dDevice->CreateTexture2D(&texDesc, 0, &ambientTex0));
		HR(md3dDevice->CreateShaderResourceView(ambientTex0, 0, &mAmbientSRV0));
		HR(md3dDevice->CreateRenderTargetView(ambientTex0, 0, &mAmbientRTV0));

		ID3D11Texture2D* ambientTex1 = 0;
		HR(md3dDevice->CreateTexture2D(&texDesc, 0, &ambientTex1));
		HR(md3dDevice->CreateShaderResourceView(ambientTex1, 0, &mAmbientSRV1));
		HR(md3dDevice->CreateRenderTargetView(ambientTex1, 0, &mAmbientRTV1));


		// view saves a reference.
		ReleaseCOM(ambientTex0);
		ReleaseCOM(ambientTex1);
	}

	void Ssao::ReleaseTextureViews()
	{
		ReleaseCOM(mNormalDepthRTV);
		ReleaseCOM(mNormalDepthSRV);

		ReleaseCOM(mAmbientRTV0);
		ReleaseCOM(mAmbientSRV0);

		ReleaseCOM(mAmbientRTV1);
		ReleaseCOM(mAmbientSRV1);
	}

	void Ssao::BuildRandomVectorTexture()
	{
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = 256;
		texDesc.Height = 256;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_IMMUTABLE;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.SysMemPitch = 256 * sizeof(Color);

		std::vector<Color> color(256 * 256);
		for (int i = 0; i < 256; ++i)
		{
			for (int j = 0; j < 256; ++j)
			{
				XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());

				color[i * 256 + j] = Color(v.x, v.y, v.z, 0.0f);
			}
		}

		initData.pSysMem = color[0];

		ID3D11Texture2D* tex = 0;
		HR(md3dDevice->CreateTexture2D(&texDesc, &initData, &tex));

		HR(md3dDevice->CreateShaderResourceView(tex, 0, &mRandomVectorSRV));

		// view saves a reference.
		ReleaseCOM(tex);
	}

	void Ssao::BuildOffsetVectors()
	{
		// Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
		// and the 6 center points along each cube face.  We always alternate the points on 
		// opposites sides of the cubes.  This way we still get the vectors spread out even
		// if we choose to use less than 14 samples.

		// 8 cube corners
		mOffsets[0] = Vector4(+1.0f, +1.0f, +1.0f, 0.0f);
		mOffsets[1] = Vector4(-1.0f, -1.0f, -1.0f, 0.0f);

		mOffsets[2] = Vector4(-1.0f, +1.0f, +1.0f, 0.0f);
		mOffsets[3] = Vector4(+1.0f, -1.0f, -1.0f, 0.0f);

		mOffsets[4] = Vector4(+1.0f, +1.0f, -1.0f, 0.0f);
		mOffsets[5] = Vector4(-1.0f, -1.0f, +1.0f, 0.0f);

		mOffsets[6] = Vector4(-1.0f, +1.0f, -1.0f, 0.0f);
		mOffsets[7] = Vector4(+1.0f, -1.0f, +1.0f, 0.0f);

		// 6 centers of cube faces
		mOffsets[8] = Vector4(-1.0f, 0.0f, 0.0f, 0.0f);
		mOffsets[9] = Vector4(+1.0f, 0.0f, 0.0f, 0.0f);

		mOffsets[10] = Vector4(0.0f, -1.0f, 0.0f, 0.0f);
		mOffsets[11] = Vector4(0.0f, +1.0f, 0.0f, 0.0f);

		mOffsets[12] = Vector4(0.0f, 0.0f, -1.0f, 0.0f);
		mOffsets[13] = Vector4(0.0f, 0.0f, +1.0f, 0.0f);

		for (int i = 0; i < 14; ++i)
		{
			// Create random lengths in [0.25, 1.0].
			float s = MathHelper::RandF(0.25f, 1.0f);

			XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&mOffsets[i]));

			XMStoreFloat4(&mOffsets[i], v);
		}
	}
}