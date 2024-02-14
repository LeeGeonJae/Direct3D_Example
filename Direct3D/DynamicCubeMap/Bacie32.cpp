#include "Basic32.h"

namespace dynamicCubeMap
{
	Basic32::Basic32(ID3D11Device* device, const std::wstring& filename)
		: mPerObject{}
		, mPerFrame{}
		, mPerObjectCB(nullptr)
		, mPerFrameCB(nullptr)
		, mVertexShader(nullptr)
		, mPixelShader(nullptr)
		, mInputLayout(nullptr)
		, mLinearSampler(nullptr)
	{
		using namespace common;

		// 쉐이더 컴파일
		ID3DBlob* vertexShaderBuffer = nullptr;
		HR(D3DHelper::CompileShaderFromFile(filename.c_str(), "VS", "vs_5_0", &vertexShaderBuffer));

		HR(device->CreateVertexShader(
			vertexShaderBuffer->GetBufferPointer(),
			vertexShaderBuffer->GetBufferSize(),
			NULL,
			&mVertexShader));

		ID3DBlob* pixelShaderBuffer = nullptr;
		HR(D3DHelper::CompileShaderFromFile(filename.c_str(), "PS", "ps_5_0", &pixelShaderBuffer));

		HR(device->CreatePixelShader(
			pixelShaderBuffer->GetBufferPointer(),
			pixelShaderBuffer->GetBufferSize(),
			NULL,
			&mPixelShader));

		ReleaseCOM(pixelShaderBuffer);

		const D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		HR(device->CreateInputLayout(
			inputElementDesc,
			ARRAYSIZE(inputElementDesc),
			vertexShaderBuffer->GetBufferPointer(),
			vertexShaderBuffer->GetBufferSize(),
			&mInputLayout
		));

		ReleaseCOM(vertexShaderBuffer);

		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(PerObject) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerObject);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		HR(device->CreateBuffer(&cbd, NULL, &mPerObjectCB));

		static_assert(sizeof(PerFrame) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerFrame);
		HR(device->CreateBuffer(&cbd, NULL, &mPerFrameCB));

		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		samplerDesc.MaxAnisotropy = 4;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 비교를 통과하지 않는다?
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		device->CreateSamplerState(&samplerDesc, &mLinearSampler);
	}
	Basic32::~Basic32()
	{
		ReleaseCOM(mPerObjectCB);
		ReleaseCOM(mPerFrameCB);

		ReleaseCOM(mVertexShader);
		ReleaseCOM(mPixelShader);
		ReleaseCOM(mInputLayout);
		ReleaseCOM(mLinearSampler);
	}

	void Basic32::UpdateSubresource(ID3D11DeviceContext* context)
	{
		context->UpdateSubresource(mPerObjectCB, 0, 0, &mPerObject, 0, 0);
		context->UpdateSubresource(mPerFrameCB, 0, 0, &mPerFrame, 0, 0);
	}
	void Basic32::Bind(ID3D11DeviceContext* context, ID3D11ShaderResourceView* SRV)
	{
		context->IASetInputLayout(mInputLayout);

		context->VSSetShader(mVertexShader, 0, 0);
		context->VSSetConstantBuffers(0, 1, &mPerObjectCB);
		context->VSSetConstantBuffers(1, 1, &mPerFrameCB);

		context->PSSetShader(mPixelShader, 0, 0);
		context->PSSetConstantBuffers(0, 1, &mPerObjectCB);
		context->PSSetConstantBuffers(1, 1, &mPerFrameCB);
		context->PSSetSamplers(0, 1, &mLinearSampler);
		context->PSSetShaderResources(0, 1, &SRV);
	}
}