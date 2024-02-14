#include <cassert>
#include <fstream>
#include <vector>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "MathHelper.h"
#include "RenderStates.h"

namespace computeShader
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mNumElements(32)
	{
	}

	D3DSample::~D3DSample()
	{
		ReleaseCOM(mOutputBuffer);
		ReleaseCOM(mOutputDebugBuffer);

		ReleaseCOM(mInputASRV);
		ReleaseCOM(mInputBSRV);
		ReleaseCOM(mOutputUAV);

		ReleaseCOM(mComputeShader);

		RenderStates::Destroy();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		// build shader
		// build inputLayout
		RenderStates::Init(md3dDevice);

		buildShader();
		buildBuffersAndViews();

		return true;
	}

	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();
	}

	void D3DSample::Update(float deltaTime)
	{

	}

	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);


		md3dContext->ClearRenderTargetView(mRenderTargetView, common::Silver);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		mSwapChain->Present(0, 0);
	}

	void D3DSample::DoComputeWork()
	{
		md3dContext->CSSetShader(mComputeShader, nullptr, 0);
		md3dContext->CSSetShaderResources(0, 1, &mInputASRV);
		md3dContext->CSSetShaderResources(1, 1, &mInputBSRV);
		md3dContext->CSSetUnorderedAccessViews(0, 1, &mOutputUAV, nullptr);

		md3dContext->Dispatch(1, 1, 1);

		ID3D11ShaderResourceView* nullSRV[1] = { 0 };
		md3dContext->CSSetShaderResources(0, 1, nullSRV);

		ID3D11UnorderedAccessView* nullUAV[1] = { 0 };
		md3dContext->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
		md3dContext->CSSetShader(NULL, NULL, 0);

		md3dContext->CopyResource(mOutputDebugBuffer, mOutputBuffer);

		D3D11_MAPPED_SUBRESOURCE mappedData;
		md3dContext->Map(mOutputDebugBuffer, 0, D3D11_MAP_READ, 0, &mappedData);

		Data* dataViews = reinterpret_cast<Data*>(mappedData.pData);

		std::ofstream fout("results.txt");

		for (UINT i = 0; i < mNumElements; ++i)
		{
			fout << "(" << dataViews[i].v1.x
				<< ", " << dataViews[i].v1.y
				<< ", " << dataViews[i].v1.z
				<< ", " << dataViews[i].v2.x
				<< ", " << dataViews[i].v2.y
				<< ")" << std::endl;
		}

		md3dContext->Unmap(mOutputBuffer, 0);

		fout.close();
	}

	void D3DSample::buildShader()
	{
		ID3DBlob* computeShaderBlob = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"../Resource/Shader/VecAddCS.hlsl", "main", "cs_5_0", &computeShaderBlob));
		md3dDevice->CreateComputeShader(computeShaderBlob->GetBufferPointer()
			, computeShaderBlob->GetBufferSize()
			, NULL
			, &mComputeShader);
	}

	void D3DSample::buildBuffersAndViews()
	{
		std::vector<Data> dataA(mNumElements);
		std::vector<Data> dataB(mNumElements);

		for (UINT i = 0; i < mNumElements; ++i)
		{
			dataA[i].v1 = { (float)i, (float)i, (float)i };
			dataA[i].v2 = { (float)i, 0.f };

			dataB[i].v1 = { -(float)i, (float)i, 0.f };
			dataB[i].v2 = { 0.f, -(float)i };
		}

		D3D11_BUFFER_DESC inputDesc;
		inputDesc.Usage = D3D11_USAGE_DEFAULT;
		inputDesc.ByteWidth = sizeof(Data) * mNumElements;
		inputDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		inputDesc.CPUAccessFlags = 0;
		inputDesc.StructureByteStride = sizeof(Data);
		inputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		D3D11_SUBRESOURCE_DATA vInitDataA;
		vInitDataA.pSysMem = &dataA[0];

		ID3D11Buffer* bufferA = nullptr;
		HR(md3dDevice->CreateBuffer(&inputDesc, &vInitDataA, &bufferA));

		D3D11_SUBRESOURCE_DATA vInitDataB;
		vInitDataB.pSysMem = &dataB[0];

		ID3D11Buffer* bufferB;
		HR(md3dDevice->CreateBuffer(&inputDesc, &vInitDataB, &bufferB));

		D3D11_BUFFER_DESC outputDesc;
		outputDesc.Usage = D3D11_USAGE_DEFAULT;
		outputDesc.ByteWidth = sizeof(Data) * mNumElements;
		outputDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		outputDesc.CPUAccessFlags = 0;
		outputDesc.StructureByteStride = sizeof(Data);
		outputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		HR(md3dDevice->CreateBuffer(&outputDesc, 0, &mOutputBuffer));

		outputDesc.Usage = D3D11_USAGE_STAGING;
		outputDesc.BindFlags = 0;
		outputDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		HR(md3dDevice->CreateBuffer(&outputDesc, 0, &mOutputDebugBuffer));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		srvDesc.BufferEx.FirstElement = 0;
		srvDesc.BufferEx.Flags = 0;
		srvDesc.BufferEx.NumElements = mNumElements;

		md3dDevice->CreateShaderResourceView(bufferA, &srvDesc, &mInputASRV);
		md3dDevice->CreateShaderResourceView(bufferB, &srvDesc, &mInputBSRV);

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0;
		uavDesc.Buffer.NumElements = mNumElements;

		md3dDevice->CreateUnorderedAccessView(mOutputBuffer, &uavDesc, &mOutputUAV);

		ReleaseCOM(bufferA);
		ReleaseCOM(bufferB);
	}
}