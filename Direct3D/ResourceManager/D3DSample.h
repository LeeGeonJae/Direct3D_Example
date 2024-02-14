#pragma once

#include <vector>
#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"

#include "Camera.h"
#include "Model.h"
#include "SkinnedModel.h"

namespace resourceManager
{
	using namespace common;

	struct VSConstantBufferInfo
	{
		DirectX::SimpleMath::Matrix WorldTransform;
		DirectX::SimpleMath::Matrix ViewTransform;
		DirectX::SimpleMath::Matrix ProjectionTransform;
		DirectX::SimpleMath::Vector4 CameraPosition;
	};

	struct PSConstantBufferInfo
	{
		DirectX::SimpleMath::Vector4 LightDirection;
		DirectX::SimpleMath::Vector4 LightColor;
	};

	class D3DSample final : public D3DProcessor
	{
	public:
		D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name);
		~D3DSample() override;

		void OnResize() override;
		bool Init() override;
		void Update(float deltaTime) override;
		void Render() override;

		void OnMouseDown(WPARAM btnState, int x, int y);
		void OnMouseUp(WPARAM btnState, int x, int y);
		void OnMouseMove(WPARAM btnState, int x, int y);

	private:
		void initD3D(); // resterizerState, samplerState
		void initShaderResource(); // shader, layout, constant buffer
		void preRender();
		void postRender();

	private:
		ID3D11VertexShader* mVertexShader;
		ID3D11VertexShader* mSkinnedVertexShader;
		ID3D11PixelShader* mPixelShader;
		ID3D11InputLayout* mInputLayout;
		ID3D11InputLayout* mSkinnedInputLayout;

		UINT mVertexBufferStride;
		UINT mVertexBufferOffset;

		ID3D11RasterizerState* mWireFrameRasterizerState;
		ID3D11RasterizerState* mSolidRasterizerState;
		ID3D11BlendState* mBlendState;
		ID3D11SamplerState* mSamplerLinear;

		// sceneData
		Camera mCam;

		ID3D11Buffer* mVSConstnat;
		ID3D11Buffer* mPSConstnat;
		VSConstantBufferInfo mVSConstantBufferInfo;
		PSConstantBufferInfo mPSConstantBufferInfo;

		POINT mLastMousePos;

		std::vector<ModelInstance> mModelInstances;
		std::vector<SkinnedModelInstance> mSkinnedModelInstances;
	};
}