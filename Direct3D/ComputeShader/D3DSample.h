#pragma once

#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"

namespace computeShader
{
	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct Data
	{
		Vector3 v1;
		Vector2 v2;
	};

	class D3DSample final : public D3DProcessor
	{
	public:
		D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name);
		~D3DSample();

		bool Init();
		void OnResize();
		void Update(float deltaTime) override;
		void Render() override;

		void DoComputeWork();

	private:
		void buildShader();
		void buildBuffersAndViews();

	private:
		ID3D11Buffer* mOutputBuffer;
		ID3D11Buffer* mOutputDebugBuffer;

		ID3D11ShaderResourceView* mInputASRV;
		ID3D11ShaderResourceView* mInputBSRV;
		ID3D11UnorderedAccessView* mOutputUAV;

		ID3D11ComputeShader* mComputeShader;

		UINT mNumElements;
	};
}