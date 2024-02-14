#pragma once

#include <Windows.h>
#include <directxtk/SimpleMath.h>

#include "D3DUtil.h"

namespace computeShaderBlur
{
	class BlurFilter
	{
	public:
		BlurFilter();
		~BlurFilter();

		void Init(ID3D11Device* device, UINT width, UINT Height, DXGI_FORMAT format);
		void BlurInPlace(ID3D11DeviceContext* context, ID3D11ComputeShader* horizontalBlur, ID3D11ComputeShader* verticalBlur,ID3D11ShaderResourceView* inputSRV, ID3D11UnorderedAccessView* inputUAV, int blurCount);

		ID3D11ShaderResourceView* GetBlurredOutput();

		void SetGaussianWeights(float sigma);
		void SetWeights(const float weights[9]);

	private:
		UINT mWidth;
		UINT mHeight;
		DXGI_FORMAT mFormat;

		ID3D11ShaderResourceView* mBlurredOutputTexSRV;
		ID3D11UnorderedAccessView* mBlurredOutputTexUAV;
	};
}