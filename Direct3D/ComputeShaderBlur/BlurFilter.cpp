#include "BlurFilter.h"

namespace computeShaderBlur
{
	BlurFilter::BlurFilter()
		: mBlurredOutputTexSRV(nullptr)
		, mBlurredOutputTexUAV(nullptr)
	{
	}
	BlurFilter::~BlurFilter()
	{
		ReleaseCOM(mBlurredOutputTexSRV);
		ReleaseCOM(mBlurredOutputTexUAV);
	}

	void BlurFilter::Init(ID3D11Device* device, UINT width, UINT height,
		DXGI_FORMAT format)
	{
		ReleaseCOM(mBlurredOutputTexSRV);
		ReleaseCOM(mBlurredOutputTexUAV);

		mWidth = width;
		mHeight = height;
		mFormat = format;

		D3D11_TEXTURE2D_DESC blurredTexDesc;
		blurredTexDesc.Width = width;
		blurredTexDesc.Height = height;
		blurredTexDesc.MipLevels = 1;
		blurredTexDesc.ArraySize = 1;
		blurredTexDesc.Format = format;
		blurredTexDesc.SampleDesc.Count = 1;
		blurredTexDesc.SampleDesc.Quality = 0;
		blurredTexDesc.Usage = D3D11_USAGE_DEFAULT;
		blurredTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		blurredTexDesc.CPUAccessFlags = 0;
		blurredTexDesc.MiscFlags = 0;

		ID3D11Texture2D* blurredTex = 0;
		HR(device->CreateTexture2D(&blurredTexDesc, 0, &blurredTex));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		HR(device->CreateShaderResourceView(blurredTex, &srvDesc, &mBlurredOutputTexSRV));

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = format;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		HR(device->CreateUnorderedAccessView(blurredTex, &uavDesc, &mBlurredOutputTexUAV));

		// Views save a reference to the texture so we can release our reference.
		ReleaseCOM(blurredTex);

	}
	void BlurFilter::BlurInPlace(ID3D11DeviceContext* context, ID3D11ComputeShader* horizontalBlur, ID3D11ComputeShader* verticalBlur,
		ID3D11ShaderResourceView* inputSRV, ID3D11UnorderedAccessView* inputUAV, int blurCount)
	{
		ID3D11ShaderResourceView* nullSRV[1] = { 0 };
		ID3D11UnorderedAccessView* nullUAV[1] = { 0 };

		for (int i = 0; i < blurCount; ++i)
		{
			// HORIZONTAL blur pass.
			context->CSSetShader(horizontalBlur, 0, 0);
			context->CSSetShaderResources(0, 1, &inputSRV);
			context->CSSetUnorderedAccessViews(0, 1, &mBlurredOutputTexUAV, nullptr);
			UINT numGroupsX = (UINT)ceilf(mWidth / 256.0f);
			context->Dispatch(numGroupsX, mHeight, 1);
			context->CSSetShaderResources(0, 1, nullSRV);
			context->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);

			// // VERTICAL blur pass.
			context->CSSetShader(verticalBlur, 0, 0);
			context->CSSetShaderResources(0, 1, &mBlurredOutputTexSRV);
			context->CSSetUnorderedAccessViews(0, 1, &inputUAV, nullptr);
			UINT numGroupsY = (UINT)ceilf(mHeight / 256.0f);
			context->Dispatch(mWidth, numGroupsY, 1);
			context->CSSetShaderResources(0, 1, nullSRV);
			context->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);
		}

		context->CSSetShader(horizontalBlur, 0, 0);
	}

	ID3D11ShaderResourceView* BlurFilter::GetBlurredOutput()
	{
		return mBlurredOutputTexSRV;
	}

	void BlurFilter::SetGaussianWeights(float sigma)
	{
		float d = 2.0f * sigma * sigma;

		float weights[9];
		float sum = 0.0f;
		for (int i = 0; i < 8; ++i)
		{
			float x = (float)i;
			weights[i] = expf(-x * x / d);

			sum += weights[i];
		}

		// Divide by the sum so all the weights add up to 1.0.
		for (int i = 0; i < 8; ++i)
		{
			weights[i] /= sum;
		}

		// setWeight;	
	}

	void BlurFilter::SetWeights(const float weights[9])
	{
		// setWeight;	
	}
}