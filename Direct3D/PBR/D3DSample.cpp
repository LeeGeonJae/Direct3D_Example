#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <fstream>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <cassert>
#include <directxtk/DDSTextureLoader.h>
#include <cassert>

#include "Mesh.h"
#include "Image.h"
#include "D3DSample.h"
#include "MathHelper.h"
#include "Utils.h"
#include "d3dUtil.h"
#include "MathHelper.h"

namespace initalization
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
	{
		mLights[0].Direction = { 0.0f,  0.0f, 1.0f };
		mLights[1].Direction = { 1.0f,  0.0f, 0.0f };
		mLights[2].Direction = { 0.0f, -1.0f, 0.0f };
		mLights[0].Radiance = { 1.0f, 1.f, 1.f };
		mLights[1].Radiance = { 1.0f, 1.f, 1.f };
		mLights[2].Radiance = { 1.0f, 1.f, 1.f };
		mLights[0].bIsUsed = true;
		mLights[1].bIsUsed = true;
		mLights[2].bIsUsed = true;
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		mCam.SetPosition(0.0f, 2.0f, -150.0f);

		mFramebuffer = createFrameBuffer(mWidth, mHeight, m4xMsaaQuality, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT);

		mResolveFramebuffer = createFrameBuffer(mWidth, mHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, (DXGI_FORMAT)0);


		const std::vector<D3D11_INPUT_ELEMENT_DESC> meshInputLayout = {
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		const std::vector<D3D11_INPUT_ELEMENT_DESC> skyboxInputLayout = {
			{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		ID3D11UnorderedAccessView* const nullUAV[] = { nullptr };
		ID3D11Buffer* const nullBuffer[] = { nullptr };

		D3D11_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_BACK;
		rasterizerDesc.FrontCounterClockwise = true;
		rasterizerDesc.DepthClipEnable = true;
		if (FAILED(md3dDevice->CreateRasterizerState(&rasterizerDesc, &mDefaultRasterizerState))) {
			throw std::runtime_error("Failed to create default rasterizer state");
		}

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
		if (FAILED(md3dDevice->CreateDepthStencilState(&depthStencilDesc, &mDefaultDepthStencilState))) {
			throw std::runtime_error("Failed to create default depth-stencil state");
		}

		depthStencilDesc.DepthEnable = false;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		if (FAILED(md3dDevice->CreateDepthStencilState(&depthStencilDesc, &mSkyboxDepthStencilState))) {
			throw std::runtime_error("Failed to create skybox depth-stencil state");
		}

		mDefaultSampler = createSamplerState(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP);
		mComputeSampler = createSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);

		mVertexCB = createConstantBuffer<CBVertex>();
		mPixelCB = createConstantBuffer<CBPixel>();

		mPBRProgram = createShaderProgram(
			compileShader("shaders/pbr.hlsl", "VS", "vs_5_0"),
			compileShader("shaders/pbr.hlsl", "PS", "ps_5_0"),
			&meshInputLayout
		);
		mSkyboxProgram = createShaderProgram(
			compileShader("shaders/skybox.hlsl", "VS", "vs_5_0"),
			compileShader("shaders/skybox.hlsl", "PS", "ps_5_0"),
			&skyboxInputLayout
		);
		mTonemapProgram = createShaderProgram(
			compileShader("shaders/tonemap.hlsl", "VS", "vs_5_0"),
			compileShader("shaders/tonemap.hlsl", "PS", "ps_5_0"),
			nullptr
		);

		mPBRModel = createMeshBuffer(Mesh::fromFile("meshes/cerberus.fbx"));
		mSkybox = createMeshBuffer(Mesh::fromFile("meshes/skybox.obj"));

		mAlbedoTexture = createTexture(Image::fromFile("textures/cerberus_A.png"), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
		mNormalTexture = createTexture(Image::fromFile("textures/cerberus_N.png"), DXGI_FORMAT_R8G8B8A8_UNORM);
		mMetalnessTexture = createTexture(Image::fromFile("textures/cerberus_M.png", 1), DXGI_FORMAT_R8_UNORM);
		mRoughnessTexture = createTexture(Image::fromFile("textures/cerberus_R.png", 1), DXGI_FORMAT_R8_UNORM);

		{
			// 큐브맵과 순서 없는 접근 뷰 생성
			Texture envTextureUnfiltered = createTextureCube(1024, 1024, DXGI_FORMAT_R16G16B16A16_FLOAT);
			createTextureUAV(envTextureUnfiltered, 0);

			// 계산 셰이더로 등장방형(직사각형) 텍스처를 큐브맵에 매핑시킨다.
			{
				ComputeProgram equirectToCubeProgram = createComputeProgram(compileShader("shaders/equirect2cube.hlsl", "main", "cs_5_0"));
				Texture envTextureEquirect = createTexture(Image::fromFile("textures/environment.hdr"), DXGI_FORMAT_R32G32B32A32_FLOAT, 1);

				md3dContext->CSSetShaderResources(0, 1, &envTextureEquirect.srv);
				md3dContext->CSSetUnorderedAccessViews(0, 1, &envTextureUnfiltered.uav, nullptr);
				md3dContext->CSSetSamplers(0, 1, &mComputeSampler);
				md3dContext->CSSetShader(equirectToCubeProgram.computeShader, nullptr, 0);
				md3dContext->Dispatch(envTextureUnfiltered.width / 32, envTextureUnfiltered.height / 32, 6);
				md3dContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
			}

			// 밉 수준 생성
			md3dContext->GenerateMips(envTextureUnfiltered.srv);

			// Compute pre-filtered specular environment map.
			{
				struct SpecularMapFilterSettingsCB
				{
					float roughness;
					float padding[3];
				};
				ComputeProgram spmapProgram = createComputeProgram(compileShader("shaders/spmap.hlsl", "main", "cs_5_0"));
				ID3D11Buffer* spmapCB = createConstantBuffer<SpecularMapFilterSettingsCB>();

				mEnvTexture = createTextureCube(1024, 1024, DXGI_FORMAT_R16G16B16A16_FLOAT);

				// Copy 0th mipmap level into destination environment map.
				for (int arraySlice = 0; arraySlice < 6; ++arraySlice) {
					const UINT subresourceIndex = D3D11CalcSubresource(0, arraySlice, mEnvTexture.levels);
					md3dContext->CopySubresourceRegion(mEnvTexture.texture, subresourceIndex, 0, 0, 0, envTextureUnfiltered.texture, subresourceIndex, nullptr);
				}

				md3dContext->CSSetShaderResources(0, 1, &envTextureUnfiltered.srv);
				md3dContext->CSSetSamplers(0, 1, &mComputeSampler);
				md3dContext->CSSetShader(spmapProgram.computeShader, nullptr, 0);

				// Pre-filter rest of the mip chain.
				const float deltaRoughness = 1.0f / max(float(mEnvTexture.levels - 1), 1.0f);
				for (UINT level = 1, size = 512; level < mEnvTexture.levels; ++level, size /= 2) {
					const UINT numGroups = max(1, size / 32);
					createTextureUAV(mEnvTexture, level);

					const SpecularMapFilterSettingsCB spmapConstants = { level * deltaRoughness };
					md3dContext->UpdateSubresource(spmapCB, 0, nullptr, &spmapConstants, 0, 0);

					md3dContext->CSSetConstantBuffers(0, 1, &spmapCB);
					md3dContext->CSSetUnorderedAccessViews(0, 1, &mEnvTexture.uav, nullptr);
					md3dContext->Dispatch(numGroups, numGroups, 6);
				}
				md3dContext->CSSetConstantBuffers(0, 1, nullBuffer);
				md3dContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
			}
		}

		// Compute diffuse irradiance cubemap.
		{
			ComputeProgram irmapProgram = createComputeProgram(compileShader("shaders/irmap.hlsl", "main", "cs_5_0"));

			mIrmapTexture = createTextureCube(32, 32, DXGI_FORMAT_R16G16B16A16_FLOAT, 1);
			createTextureUAV(mIrmapTexture, 0);

			md3dContext->CSSetShaderResources(0, 1, &mEnvTexture.srv);
			md3dContext->CSSetSamplers(0, 1, &mComputeSampler);
			md3dContext->CSSetUnorderedAccessViews(0, 1, &mIrmapTexture.uav, nullptr);
			md3dContext->CSSetShader(irmapProgram.computeShader, nullptr, 0);
			md3dContext->Dispatch(mIrmapTexture.width / 32, mIrmapTexture.height / 32, 6);
			md3dContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
		}

		// Compute Cook-Torrance BRDF 2D LUT for split-sum approximation.
		{
			ComputeProgram spBRDFProgram = createComputeProgram(compileShader("shaders/spbrdf.hlsl", "main", "cs_5_0"));

			mSpBRDF_LUT = createTexture(256, 256, DXGI_FORMAT_R16G16_FLOAT, 1);
			mBRDFSampler = createSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
			createTextureUAV(mSpBRDF_LUT, 0);

			md3dContext->CSSetUnorderedAccessViews(0, 1, &mSpBRDF_LUT.uav, nullptr);
			md3dContext->CSSetShader(spBRDFProgram.computeShader, nullptr, 0);
			md3dContext->Dispatch(mSpBRDF_LUT.width / 32, mSpBRDF_LUT.height / 32, 1);
			md3dContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
		}

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
			mCam.TranslateLook(100.0f * deltaTime);

		if (GetAsyncKeyState('S') & 0x8000)
			mCam.TranslateLook(-100.0f * deltaTime);

		if (GetAsyncKeyState('A') & 0x8000)
			mCam.TranslateRight(-100.0f * deltaTime);

		if (GetAsyncKeyState('D') & 0x8000)
			mCam.TranslateRight(100.0f * deltaTime);

		mCam.UpdateViewMatrix();

		bUseIBL = GetAsyncKeyState('1') & 0x8000;
		bUseToneMappingAndGamma = GetAsyncKeyState('2') & 0x8000;
	}

	void D3DSample::Render()
	{
		static float radian = 0.f;
		// radian += 0.01f;

		const Matrix projectionMatrix = mCam.GetProj();
		const Matrix viewRotationMatrix = Matrix::Identity;
		const Matrix worldMatrix = Matrix::CreateRotationY(radian);
		const Matrix viewMatrix = mCam.GetView();
		const Vector3 eyePosition = mCam.GetPosition();

		Vector3 eyePos = mCam.GetPosition();
		Matrix T = Matrix::CreateTranslation(eyePos);
		Matrix WVP = T * mCam.GetViewProj();
		// Update transform constant buffer (for vertex shaders).
		{
			CBVertex transformConstants;
			transformConstants.ViewProjectionMatrix = viewMatrix * projectionMatrix;
			transformConstants.SkyProjectionMatrix = WVP;
			transformConstants.WorldMatrix = worldMatrix;

			transformConstants.ViewProjectionMatrix = transformConstants.ViewProjectionMatrix.Transpose();
			transformConstants.SkyProjectionMatrix = transformConstants.SkyProjectionMatrix.Transpose();
			transformConstants.WorldMatrix = transformConstants.WorldMatrix.Transpose();

			md3dContext->UpdateSubresource(mVertexCB, 0, nullptr, &transformConstants, 0, 0);
		}


		// Update shading constant buffer (for pixel shader).
		{
			CBPixel shadingConstants;
			shadingConstants.EyePosition = Vector4(eyePosition);
			shadingConstants.UseIBL.x = bUseIBL ? 1.0f : 0;

			for (int i = 0; i < 3; ++i) {
				Light& light = mLights[i];

				if (GetAsyncKeyState('3') & 0x8000)
					light.Direction += Vector3(1, 1, 1) * 0.01f;
				if (GetAsyncKeyState('4') & 0x8000)
					light.Direction += Vector3(1, 1, 1) * -0.01f;

				shadingConstants.lights[i].Direction = Vector4(light.Direction);
				shadingConstants.lights[i].Direction.Normalize();

				if (light.bIsUsed) {
					shadingConstants.lights[i].Radiance = Vector4(light.Radiance);
				}
				else {
					shadingConstants.lights[i].Radiance = { 0, 0, 0, 0 };
				}
			}
			md3dContext->UpdateSubresource(mPixelCB, 0, nullptr, &shadingConstants, 0, 0);
		}

		if (bUseToneMappingAndGamma)
		{
			// Prepare framebuffer for rendering.
			const float clear_color_with_alpha[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			md3dContext->OMSetRenderTargets(1, &mFramebuffer.rtv, mFramebuffer.dsv);
			md3dContext->ClearRenderTargetView(mFramebuffer.rtv, clear_color_with_alpha);
			md3dContext->ClearDepthStencilView(mFramebuffer.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
		}
		else
		{
			// Prepare framebuffer for rendering.
			const float clear_color_with_alpha[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			md3dContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
			md3dContext->ClearRenderTargetView(mRenderTargetView, clear_color_with_alpha);
			md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
			md3dContext->OMSetDepthStencilState(0, 0);
		}


		// Set known pipeline state.
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dContext->VSSetConstantBuffers(0, 1, &mVertexCB);
		md3dContext->PSSetConstantBuffers(0, 1, &mPixelCB);

		// Draw skybox.
		if (bUseIBL)
		{
			md3dContext->IASetInputLayout(mSkyboxProgram.inputLayout);
			md3dContext->IASetVertexBuffers(0, 1, &mSkybox.vertexBuffer, &mSkybox.stride, &mSkybox.offset);
			md3dContext->IASetIndexBuffer(mSkybox.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
			md3dContext->VSSetShader(mSkyboxProgram.vertexShader, nullptr, 0);
			md3dContext->PSSetShader(mSkyboxProgram.pixelShader, nullptr, 0);
			md3dContext->PSSetShaderResources(0, 1, &mEnvTexture.srv);
			md3dContext->PSSetSamplers(0, 1, &mDefaultSampler);
			md3dContext->OMSetDepthStencilState(mSkyboxDepthStencilState, 0);
			md3dContext->DrawIndexed(mSkybox.numElements, 0, 0);
		}

		// Draw PBR model.
		ID3D11ShaderResourceView* const pbrModelSRVs[] = {
			mAlbedoTexture.srv,
			mNormalTexture.srv,
			mMetalnessTexture.srv,
			mRoughnessTexture.srv,
			mEnvTexture.srv,
			mIrmapTexture.srv,
			mSpBRDF_LUT.srv,
		};
		ID3D11SamplerState* const pbrModelSamplers[] = {
			mDefaultSampler,
			mBRDFSampler,
		};

		md3dContext->IASetInputLayout(mPBRProgram.inputLayout);
		md3dContext->IASetVertexBuffers(0, 1, &mPBRModel.vertexBuffer, &mPBRModel.stride, &mPBRModel.offset);
		md3dContext->IASetIndexBuffer(mPBRModel.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		md3dContext->VSSetShader(mPBRProgram.vertexShader, nullptr, 0);
		md3dContext->PSSetShader(mPBRProgram.pixelShader, nullptr, 0);
		md3dContext->PSSetShaderResources(0, 7, pbrModelSRVs);
		md3dContext->PSSetSamplers(0, 2, pbrModelSamplers);
		md3dContext->OMSetDepthStencilState(mDefaultDepthStencilState, 0);
		md3dContext->DrawIndexed(mPBRModel.numElements, 0, 0);

		if (bUseToneMappingAndGamma)
		{
			// Resolve multisample framebuffer.
			resolveFrameBuffer(mFramebuffer, mResolveFramebuffer, DXGI_FORMAT_R16G16B16A16_FLOAT);
			md3dContext->RSSetState(mDefaultRasterizerState);

			// Draw a full screen triangle for postprocessing/tone mapping.
			md3dContext->OMSetRenderTargets(1, &mRenderTargetView, nullptr);
			md3dContext->IASetInputLayout(nullptr);
			md3dContext->VSSetShader(mTonemapProgram.vertexShader, nullptr, 0);
			md3dContext->PSSetShader(mTonemapProgram.pixelShader, nullptr, 0);
			md3dContext->PSSetShaderResources(0, 1, &mResolveFramebuffer.srv);
			md3dContext->PSSetSamplers(0, 1, &mComputeSampler);
			md3dContext->Draw(3, 0);
			md3dContext->RSSetState(0);

		}

		mSwapChain->Present(1, 0);
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

	MeshBuffer D3DSample::createMeshBuffer(const std::shared_ptr<class Mesh>& mesh) const
	{
		MeshBuffer buffer = {};
		buffer.stride = sizeof(Mesh::Vertex);
		buffer.numElements = static_cast<UINT>(mesh->faces().size() * 3);

		const size_t vertexDataSize = mesh->vertices().size() * sizeof(Mesh::Vertex);
		const size_t indexDataSize = mesh->faces().size() * sizeof(Mesh::Face);

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = (UINT)vertexDataSize;
			desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = &mesh->vertices()[0];
			if (FAILED(md3dDevice->CreateBuffer(&desc, &data, &buffer.vertexBuffer))) {
				throw std::runtime_error("Failed to create vertex buffer");
			}
		}
		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = (UINT)indexDataSize;
			desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = &mesh->faces()[0];
			if (FAILED(md3dDevice->CreateBuffer(&desc, &data, &buffer.indexBuffer))) {
				throw std::runtime_error("Failed to create index buffer");
			}
		}
		return buffer;
	}

	Texture D3DSample::createTexture(UINT width, UINT height, DXGI_FORMAT format, UINT levels) const
	{
		Texture texture;
		texture.width = width;
		texture.height = height;
		texture.levels = (levels > 0) ? levels : Utility::numMipmapLevels(width, height);

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = levels;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		if (levels == 0) {
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}

		if (FAILED(md3dDevice->CreateTexture2D(&desc, nullptr, &texture.texture))) {
			throw std::runtime_error("Failed to create 2D texture");
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		if (FAILED(md3dDevice->CreateShaderResourceView(texture.texture, &srvDesc, &texture.srv))) {
			throw std::runtime_error("Failed to create 2D texture SRV");
		}
		return texture;
	}

	Texture D3DSample::createTextureCube(UINT width, UINT height, DXGI_FORMAT format, UINT levels) const
	{
		Texture texture;
		texture.width = width;
		texture.height = height;
		texture.levels = (levels > 0) ? levels : Utility::numMipmapLevels(width, height);

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = levels;
		desc.ArraySize = 6;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		if (levels == 0) {
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}

		if (FAILED(md3dDevice->CreateTexture2D(&desc, nullptr, &texture.texture))) {
			throw std::runtime_error("Failed to create cubemap texture");
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = -1;
		if (FAILED(md3dDevice->CreateShaderResourceView(texture.texture, &srvDesc, &texture.srv))) {
			throw std::runtime_error("Failed to create cubemap texture SRV");
		}
		return texture;
	}

	Texture D3DSample::createTexture(const std::shared_ptr<Image>& image, DXGI_FORMAT format, UINT levels) const
	{
		Texture texture = createTexture(image->width(), image->height(), format, levels);
		md3dContext->UpdateSubresource(texture.texture, 0, nullptr, image->pixels<void>(), image->pitch(), 0);
		if (levels == 0) {
			md3dContext->GenerateMips(texture.srv);
		}
		return texture;
	}

	void D3DSample::createTextureUAV(Texture& texture, UINT mipSlice) const
	{
		assert(texture.texture);

		D3D11_TEXTURE2D_DESC desc;
		texture.texture->GetDesc(&desc);

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = desc.Format;
		if (desc.ArraySize == 1) {
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = mipSlice;
		}
		else {
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.MipSlice = mipSlice;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = desc.ArraySize;
		}

		if (FAILED(md3dDevice->CreateUnorderedAccessView(texture.texture, &uavDesc, &texture.uav))) {
			throw std::runtime_error("Failed to create texture UAV");
		}
	}

	ID3D11SamplerState* D3DSample::createSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode) const
	{
		D3D11_SAMPLER_DESC desc = {};
		desc.Filter = filter;
		desc.AddressU = addressMode;
		desc.AddressV = addressMode;
		desc.AddressW = addressMode;
		desc.MaxAnisotropy = (filter == D3D11_FILTER_ANISOTROPIC) ? D3D11_REQ_MAXANISOTROPY : 1;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;

		ID3D11SamplerState* samplerState;
		if (FAILED(md3dDevice->CreateSamplerState(&desc, &samplerState))) {
			throw std::runtime_error("Failed to create sampler state");
		}
		return samplerState;
	}

	ShaderProgram D3DSample::createShaderProgram(ID3DBlob* vsBytecode, ID3DBlob* psBytecode, const std::vector<D3D11_INPUT_ELEMENT_DESC>* inputLayoutDesc) const
	{
		ShaderProgram program;

		if (FAILED(md3dDevice->CreateVertexShader(vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), nullptr, &program.vertexShader))) {
			throw std::runtime_error("Failed to create vertex shader from compiled bytecode");
		}
		if (FAILED(md3dDevice->CreatePixelShader(psBytecode->GetBufferPointer(), psBytecode->GetBufferSize(), nullptr, &program.pixelShader))) {
			throw std::runtime_error("Failed to create pixel shader from compiled bytecode");
		}

		if (inputLayoutDesc) {
			if (FAILED(md3dDevice->CreateInputLayout(inputLayoutDesc->data(), (UINT)inputLayoutDesc->size(), vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), &program.inputLayout))) {
				throw std::runtime_error("Failed to create shader program input layout");
			}
		}
		return program;
	}

	ComputeProgram D3DSample::createComputeProgram(ID3DBlob* csBytecode) const
	{
		ComputeProgram program;
		if (FAILED(md3dDevice->CreateComputeShader(csBytecode->GetBufferPointer(), csBytecode->GetBufferSize(), nullptr, &program.computeShader))) {
			throw std::runtime_error("Failed to create compute shader from compiled bytecode");
		}
		return program;
	}

	FrameBuffer D3DSample::createFrameBuffer(UINT width, UINT height, UINT samples, DXGI_FORMAT colorFormat, DXGI_FORMAT depthstencilFormat) const
	{
		FrameBuffer fb;
		fb.width = width;
		fb.height = height;
		fb.samples = samples;

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = samples;

		if (colorFormat != DXGI_FORMAT_UNKNOWN) {
			desc.Format = colorFormat;
			desc.BindFlags = D3D11_BIND_RENDER_TARGET;
			if (samples <= 1) {
				desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}
			if (FAILED(md3dDevice->CreateTexture2D(&desc, nullptr, &fb.colorTexture))) {
				throw std::runtime_error("Failed to create FrameBuffer color texture");
			}

			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = desc.Format;
			rtvDesc.ViewDimension = (samples > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
			if (FAILED(md3dDevice->CreateRenderTargetView(fb.colorTexture, &rtvDesc, &fb.rtv))) {
				throw std::runtime_error("Failed to create FrameBuffer render target view");
			}

			if (samples <= 1) {
				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = desc.Format;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = 1;
				if (FAILED(md3dDevice->CreateShaderResourceView(fb.colorTexture, &srvDesc, &fb.srv))) {
					throw std::runtime_error("Failed to create FrameBuffer shader resource view");
				}
			}
		}

		if (depthstencilFormat != DXGI_FORMAT_UNKNOWN) {
			desc.Format = depthstencilFormat;
			desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			if (FAILED(md3dDevice->CreateTexture2D(&desc, nullptr, &fb.depthStencilTexture))) {
				throw std::runtime_error("Failed to create FrameBuffer depth-stencil texture");
			}

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = desc.Format;
			dsvDesc.ViewDimension = (samples > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
			if (FAILED(md3dDevice->CreateDepthStencilView(fb.depthStencilTexture, &dsvDesc, &fb.dsv))) {
				throw std::runtime_error("Failed to create FrameBuffer depth-stencil view");
			}
		}

		return fb;
	}

	void D3DSample::resolveFrameBuffer(const FrameBuffer& srcfb, const FrameBuffer& dstfb, DXGI_FORMAT format) const
	{
		if (srcfb.colorTexture != dstfb.colorTexture) {
			md3dContext->ResolveSubresource(dstfb.colorTexture, 0, srcfb.colorTexture, 0, format);
		}
	}

	ID3D11Buffer* D3DSample::createConstantBuffer(const void* data, UINT size) const
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(size);
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		D3D11_SUBRESOURCE_DATA bufferData = {};
		bufferData.pSysMem = data;

		ID3D11Buffer* buffer;
		const D3D11_SUBRESOURCE_DATA* bufferDataPtr = data ? &bufferData : nullptr;
		if (FAILED(md3dDevice->CreateBuffer(&desc, bufferDataPtr, &buffer))) {
			throw std::runtime_error("Failed to create constant buffer");
		}
		return buffer;
	}

	ID3DBlob* D3DSample::compileShader(const std::string& filename, const std::string& entryPoint, const std::string& profile)
	{
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		flags |= D3DCOMPILE_DEBUG;
		flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ID3DBlob* shader = nullptr;
		ID3DBlob* errorBlob = nullptr;

		std::printf("Compiling HLSL shader: %s [%s]\n", filename.c_str(), entryPoint.c_str());

		if (FAILED(D3DCompileFromFile(Utility::convertToUTF16(filename).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), profile.c_str(), flags, 0, &shader, &errorBlob))) {
			std::string errorMsg = "Shader compilation failed: " + filename;
			if (errorBlob != nullptr) {
				errorMsg += std::string("\n") + static_cast<const char*>(errorBlob->GetBufferPointer());
			}
			throw std::runtime_error(errorMsg);
		}
		return shader;
	}
}