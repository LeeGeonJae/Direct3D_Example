#pragma once

#include <cassert>
#include <vector>
#include <string>

#include <dxgi.h>
#include <d3dcompiler.h>
#include <directxtk/DDSTextureLoader.h>
#include <directxtk/WICTextureLoader.h>

#include "MathHelper.h"

#pragma comment(lib, "d3dcompiler.lib")

#if defined(DEBUG) | defined(_DEBUG)
#ifndef HR
#define HR(x)                                               \
	{                                                       \
		HRESULT hr = (x);                                   \
		assert(SUCCEEDED(hr));								\
	}
#endif

#else
#ifndef HR
#define HR(x) (x)
#endif
#endif 

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }
#define SafeDelete(x) { delete x; x = 0; }

namespace common
{
	constexpr DirectX::SimpleMath::Color White = { 1.0f, 1.0f, 1.0f, 1.0f };
	constexpr DirectX::SimpleMath::Color Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	constexpr DirectX::SimpleMath::Color Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	constexpr DirectX::SimpleMath::Color Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	constexpr DirectX::SimpleMath::Color Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	constexpr DirectX::SimpleMath::Color Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
	constexpr DirectX::SimpleMath::Color Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	constexpr DirectX::SimpleMath::Color Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
	constexpr DirectX::SimpleMath::Color Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
	constexpr DirectX::SimpleMath::Color LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };

	class D3DHelper
	{
	public:
		static ID3D11ShaderResourceView* CreateTexture2DArraySRV(ID3D11Device* device, ID3D11DeviceContext* context, const std::vector<std::wstring>& filenames);
		static HRESULT CreateTextureFromFile(ID3D11Device* d3dDevice, const wchar_t* szFileName, ID3D11ShaderResourceView** textureView);
		static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
		static ID3D11SamplerState* CreateSamplerState(ID3D11Device* d3dDevice, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode);
		static ID3D11Buffer* CreateConstantBuffer(ID3D11Device* d3dDevice, const void* data, UINT size);
		template<typename T>
		static ID3D11Buffer* CreateConstantBuffer(ID3D11Device* d3dDevice, const T* data = nullptr);
		static void ExtractFrustumPlanes(DirectX::SimpleMath::Vector4 planes[6], DirectX::SimpleMath::Matrix M);
		static ID3D11ShaderResourceView* CreateRandomTexture1DSRV(ID3D11Device* device);
		static std::wstring ConvertStrToWStr(const std::string& str);
		static std::string ConvertWStrToStr(const std::wstring& wstr);
	};

	template<typename T>
	ID3D11Buffer* D3DHelper::CreateConstantBuffer(ID3D11Device* d3dDevice, const T* data)
	{
		static_assert(sizeof(T) % 16 == 0, "must be align");
		return createConstantBuffer(d3dDevice, data, sizeof(T));
	}
}