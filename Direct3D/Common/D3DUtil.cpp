#include "pch.h"

#include <stdexcept>

#include "D3DUtil.h"

namespace common
{
	ID3D11ShaderResourceView* D3DHelper::CreateTexture2DArraySRV(ID3D11Device* device
		, ID3D11DeviceContext* context
		, const std::vector<std::wstring>& filenames)
	{
		assert(device != nullptr);
		assert(context != nullptr);
		assert(filenames.size() != 0);

		const UINT SIZE = filenames.size();
		std::vector<ID3D11Texture2D*> srcTex(SIZE);

		for (UINT i = 0; i < SIZE; ++i)
		{
			DirectX::CreateDDSTextureFromFileEx(device,
				filenames[i].c_str(),
				1024 * 1024,
				D3D11_USAGE_STAGING,
				0,
				D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
				0,
				DirectX::DDS_LOADER_FORCE_SRGB,
				(ID3D11Resource**)&srcTex[i],
				nullptr
			);
		}

		D3D11_TEXTURE2D_DESC texElementDesc;
		srcTex[0]->GetDesc(&texElementDesc);

		// 첫번째 텍스처는 압축 텍스처라서 형식이 잘못되어서 이상하게 나온 거구나.. 압축 텍스처 풀려면 디테일한 설정해서 로딩해야 한다.
		// assert(memcmp(&texElementDesc, &texElementDesc1, sizeof(texElementDesc)));
		// assert(memcmp(&texElementDesc, &texElementDesc2, sizeof(texElementDesc)));
		// assert(memcmp(&texElementDesc, &texElementDesc3, sizeof(texElementDesc)));

		D3D11_TEXTURE2D_DESC texArrayDesc;
		texArrayDesc.Width = texElementDesc.Width;
		texArrayDesc.Height = texElementDesc.Height;
		texArrayDesc.MipLevels = texElementDesc.MipLevels;
		texArrayDesc.ArraySize = SIZE;
		texArrayDesc.Format = texElementDesc.Format;
		texArrayDesc.SampleDesc.Count = 1;
		texArrayDesc.SampleDesc.Quality = 0;
		texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
		texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texArrayDesc.CPUAccessFlags = 0;
		texArrayDesc.MiscFlags = 0;

		ID3D11Texture2D* texArray = 0;
		HR(device->CreateTexture2D(&texArrayDesc, 0, &texArray));

		for (UINT i = 0; i < SIZE; ++i)
		{
			for (UINT j = 0; j < texElementDesc.MipLevels; ++j)
			{
				D3D11_MAPPED_SUBRESOURCE mappedTex2D;
				HR(context->Map(srcTex[i], j, D3D11_MAP_READ, 0, &mappedTex2D));

				context->UpdateSubresource(texArray,
					D3D11CalcSubresource(j, i, texElementDesc.MipLevels),
					0,
					mappedTex2D.pData,
					mappedTex2D.RowPitch,
					mappedTex2D.DepthPitch);

				context->Unmap(srcTex[i], j);
			}
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = texArrayDesc.Format;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		viewDesc.Texture2DArray.MostDetailedMip = 0;
		viewDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
		viewDesc.Texture2DArray.FirstArraySlice = 0;
		viewDesc.Texture2DArray.ArraySize = SIZE;

		ID3D11ShaderResourceView* texArraySRV = nullptr;
		HR(device->CreateShaderResourceView(texArray, &viewDesc, &texArraySRV));

		ReleaseCOM(texArray);

		for (UINT i = 0; i < SIZE; ++i)
		{
			ReleaseCOM(srcTex[i]);
		}

		return texArraySRV;
	}

	HRESULT D3DHelper::CreateTextureFromFile(ID3D11Device* d3dDevice, const wchar_t* szFileName, ID3D11ShaderResourceView** textureView)
	{
		HRESULT hr = S_OK;

		hr = DirectX::CreateDDSTextureFromFile(d3dDevice, szFileName, nullptr, textureView);
		if (FAILED(hr))
		{
			hr = DirectX::CreateWICTextureFromFile(d3dDevice, szFileName, nullptr, textureView);
			if (FAILED(hr))
			{
				return hr;
			}
		}
		return S_OK;
	}

	HRESULT D3DHelper::CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
	{
		HRESULT hr = S_OK;

		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows 
		// the shaders to be optimized and to run exactly the way they will run in 
		// the release configuration of this program.
		dwShaderFlags |= D3DCOMPILE_DEBUG;

		// Disable optimizations to further improve shader debugging
		dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ID3DBlob* pErrorBlob = nullptr;
		hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
			dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
		if (FAILED(hr))
		{
			if (pErrorBlob)
			{
				MessageBoxA(NULL, (char*)pErrorBlob->GetBufferPointer(), "CompileShaderFromFile", MB_OK);
				pErrorBlob->Release();
			}
			return hr;
		}
		if (pErrorBlob) pErrorBlob->Release();

		return S_OK;
	}

	ID3D11SamplerState* D3DHelper::CreateSamplerState(ID3D11Device* d3dDevice, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode)
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
		if (FAILED(d3dDevice->CreateSamplerState(&desc, &samplerState))) {
			throw std::runtime_error("Failed to create sampler state");
		}
		return samplerState;
	}

	ID3D11Buffer* D3DHelper::CreateConstantBuffer(ID3D11Device* d3dDevice, const void* data, UINT size)
	{
		assert(d3dDevice != nullptr);

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = static_cast<UINT>(size);
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		D3D11_SUBRESOURCE_DATA bufferData = {};
		bufferData.pSysMem = data;

		ID3D11Buffer* buffer;
		const D3D11_SUBRESOURCE_DATA* bufferDataPtr = data ? &bufferData : nullptr;
		if (FAILED(d3dDevice->CreateBuffer(&desc, bufferDataPtr, &buffer))) {
			throw std::runtime_error("Failed to create constant buffer");
		}
		return buffer;
	}

	void D3DHelper::ExtractFrustumPlanes(DirectX::SimpleMath::Vector4 planes[6], DirectX::SimpleMath::Matrix M)
	{
		using namespace DirectX::SimpleMath;

		//
		// Left
		//
		planes[0].x = M(0, 3) + M(0, 0);
		planes[0].y = M(1, 3) + M(1, 0);
		planes[0].z = M(2, 3) + M(2, 0);
		planes[0].w = M(3, 3) + M(3, 0);

		//
		// Right
		//
		planes[1].x = M(0, 3) - M(0, 0);
		planes[1].y = M(1, 3) - M(1, 0);
		planes[1].z = M(2, 3) - M(2, 0);
		planes[1].w = M(3, 3) - M(3, 0);

		//
		// Bottom
		//
		planes[2].x = M(0, 3) + M(0, 1);
		planes[2].y = M(1, 3) + M(1, 1);
		planes[2].z = M(2, 3) + M(2, 1);
		planes[2].w = M(3, 3) + M(3, 1);

		//
		// Top
		//
		planes[3].x = M(0, 3) - M(0, 1);
		planes[3].y = M(1, 3) - M(1, 1);
		planes[3].z = M(2, 3) - M(2, 1);
		planes[3].w = M(3, 3) - M(3, 1);

		//
		// Near
		//
		planes[4].x = M(0, 2);
		planes[4].y = M(1, 2);
		planes[4].z = M(2, 2);
		planes[4].w = M(3, 2);

		//
		// Far
		//
		planes[5].x = M(0, 3) - M(0, 2);
		planes[5].y = M(1, 3) - M(1, 2);
		planes[5].z = M(2, 3) - M(2, 2);
		planes[5].w = M(3, 3) - M(3, 2);

		// Normalize the plane equations.
		for (int i = 0; i < 6; ++i)
		{
			planes[i].Normalize();
		}
	}

	ID3D11ShaderResourceView* D3DHelper::CreateRandomTexture1DSRV(ID3D11Device* device)
	{
		using namespace DirectX::SimpleMath;

		// 랜덤 백터 생성
		const size_t ELEMENT_COUNT = 1024u;

		std::vector<Vector4> randomValues(ELEMENT_COUNT);

		for (size_t i = 0; i < ELEMENT_COUNT; ++i)
		{
			randomValues[i].x = MathHelper::RandF(-1.0f, 1.0f);
			randomValues[i].y = MathHelper::RandF(-1.0f, 1.0f);
			randomValues[i].z = MathHelper::RandF(-1.0f, 1.0f);
			randomValues[i].w = MathHelper::RandF(-1.0f, 1.0f);
		}

		// 텍스처 생성
		D3D11_TEXTURE1D_DESC texDesc;
		texDesc.Width = ELEMENT_COUNT;
		texDesc.MipLevels = 1;
		texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		texDesc.Usage = D3D11_USAGE_IMMUTABLE;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;
		texDesc.ArraySize = 1;
		
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = &randomValues[0];
		initData.SysMemPitch = ELEMENT_COUNT * sizeof(Vector4);
		initData.SysMemSlicePitch = 0;

		ID3D11Texture1D* randomTex = 0;
		HR(device->CreateTexture1D(&texDesc, &initData, &randomTex));

		// 쉐이더 리소스 뷰 생성
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		viewDesc.Format = texDesc.Format;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D; // 1D 텍스처
		viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
		viewDesc.Texture1D.MostDetailedMip = 0;

		ID3D11ShaderResourceView* randomTexSRV = 0;
		HR(device->CreateShaderResourceView(randomTex, &viewDesc, &randomTexSRV));

		ReleaseCOM(randomTex);

		return randomTexSRV;
	}

	std::wstring D3DHelper::ConvertStrToWStr(const std::string& str)
	{
		int num_chars = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
		std::wstring wstrTo;
		if (num_chars)
		{
			wstrTo.resize(num_chars);
			MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstrTo[0], num_chars);
		}
		return wstrTo;
	}

	std::string D3DHelper::ConvertWStrToStr(const std::wstring& wstr)
	{
		int num_chars = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
		std::string strTo;
		if (num_chars > 0)
		{
			strTo.resize(num_chars);
			WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), &strTo[0], num_chars, NULL, NULL);
		}
		return strTo;
	}
}
