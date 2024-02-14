#include <cassert>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include "Model.h"
#include "d3dUtil.h"
#include "D3DSample.h"
#include "ResourceManager.h"

namespace resourceManager
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
	{
	}
	D3DSample::~D3DSample()
	{
		ResourceManager::DeleteInstance();

		ReleaseCOM(mVertexShader);
		ReleaseCOM(mSkinnedVertexShader);
		ReleaseCOM(mPixelShader);
		ReleaseCOM(mInputLayout);
		ReleaseCOM(mSkinnedInputLayout);

		ReleaseCOM(mWireFrameRasterizerState);
		ReleaseCOM(mSolidRasterizerState);
		ReleaseCOM(mBlendState);
		ReleaseCOM(mSamplerLinear);

		ReleaseCOM(mVSConstnat);
		ReleaseCOM(mPSConstnat);
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		mCam.SetPosition(0.0f, 2.0f, -500.0f);

		ResourceManager::GetInstance()->Init(md3dDevice, md3dContext);

		initD3D();
		initShaderResource();

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

		if (GetAsyncKeyState('1') & 0x8000)
		{
			Model* model = ResourceManager::GetInstance()->LoadModel("models/zeldaPosed001.fbx");

			int x = rand() % 300 - 150;
			int y = rand() % 300 - 150;
			int z = rand() % 300 - 150;

			mModelInstances.push_back({ model , Matrix::CreateTranslation(x, y,z) });
		}
		if (GetAsyncKeyState('2') & 0x8000)
		{
			SkinnedModel* model = ResourceManager::GetInstance()->LoadSkinnedModel("models/dancing.fbx");

			int x = rand() % 300 - 150;
			int y = rand() % 300 - 150;
			int z = rand() % 300 - 150;

			int animIndex = rand() % model->Animations.size();
			auto findedAnim = std::next(model->Animations.begin(), animIndex);
			std::string animationName = findedAnim->first;

			mSkinnedModelInstances.push_back({ model , Matrix::CreateTranslation(x, y,z), 0, animationName });
		}
		if (GetAsyncKeyState('3') & 0x8000)
		{
			SkinnedModel* model = ResourceManager::GetInstance()->LoadSkinnedModel("models/huesitos.fbx");

			int x = rand() % 300 - 150;
			int y = rand() % 300 - 150;
			int z = rand() % 300 - 150;

			int animIndex = rand() % model->Animations.size();
			auto findedAnim = std::next(model->Animations.begin(), animIndex);
			std::string animationName = findedAnim->first;

			mSkinnedModelInstances.push_back({ model , Matrix::CreateTranslation(x, y,z), 0, animationName });
		}
		mCam.UpdateViewMatrix();

		for (auto& skinnedmodelInstance : mSkinnedModelInstances)
		{
			skinnedmodelInstance.TimePos += deltaTime;
		}
	}

	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };

		preRender();

		mVSConstantBufferInfo.ViewTransform = mCam.GetView().Transpose();
		mVSConstantBufferInfo.ProjectionTransform = mCam.GetProj().Transpose();
		mVSConstantBufferInfo.CameraPosition = (Vector4)mCam.GetPosition();

		mPSConstantBufferInfo.LightColor = { 1, 1, 1, 1 };
		mPSConstantBufferInfo.LightDirection = { 0, 0, 1 };
		md3dContext->UpdateSubresource(mPSConstnat, 0, 0, &mPSConstantBufferInfo, 0, 0);

		md3dContext->IASetInputLayout(mInputLayout);
		md3dContext->VSSetShader(mVertexShader, nullptr, 0);

		for (auto& modelInstance : mModelInstances)
		{
			mVSConstantBufferInfo.WorldTransform = modelInstance.WorldMatrix.Transpose();
			md3dContext->UpdateSubresource(mVSConstnat, 0, 0, &mVSConstantBufferInfo, 0, 0);

			modelInstance.Model->Draw(md3dContext);
		}

		md3dContext->IASetInputLayout(mSkinnedInputLayout);
		md3dContext->VSSetShader(mSkinnedVertexShader, nullptr, 0);

		for (auto& skinnedmodelInstance : mSkinnedModelInstances)
		{
			mVSConstantBufferInfo.WorldTransform = skinnedmodelInstance.WorldMatrix.Transpose();
			md3dContext->UpdateSubresource(mVSConstnat, 0, 0, &mVSConstantBufferInfo, 0, 0);

			skinnedmodelInstance.SkinnedModel->Draw(md3dContext, skinnedmodelInstance.AnimationName, skinnedmodelInstance.TimePos);
		}

		postRender();
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
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.RotatePitch(dy);
			mCam.RotateAxis({ 0, 1, 0 }, dx);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

	void D3DSample::initD3D()
	{
		HRESULT hr = S_OK;

		// init rasterizerState
		{
			D3D11_RASTERIZER_DESC rasterizerDesc = {};

			rasterizerDesc.DepthClipEnable = true;

			rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
			rasterizerDesc.CullMode = D3D11_CULL_NONE;
			hr = md3dDevice->CreateRasterizerState(&rasterizerDesc, &mWireFrameRasterizerState);
			assert(SUCCEEDED(hr));

			rasterizerDesc.FillMode = D3D11_FILL_SOLID;
			rasterizerDesc.CullMode = D3D11_CULL_BACK;
			hr = md3dDevice->CreateRasterizerState(&rasterizerDesc, &mSolidRasterizerState);
			assert(SUCCEEDED(hr));
		}

		// init samplerState       
		{
			D3D11_SAMPLER_DESC sampDesc = {};
			sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			sampDesc.MinLOD = 0;
			sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
			hr = md3dDevice->CreateSamplerState(&sampDesc, &mSamplerLinear);
			assert(SUCCEEDED(hr));
		}

		// init blendState
		{
			D3D11_BLEND_DESC blendDesc = {};
			blendDesc.AlphaToCoverageEnable = false;
			blendDesc.IndependentBlendEnable = false;
			D3D11_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
			rtBlendDesc.BlendEnable = true;
			rtBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
			rtBlendDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
			rtBlendDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			rtBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			rtBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
			rtBlendDesc.DestBlendAlpha = D3D11_BLEND_ONE;
			rtBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[0] = rtBlendDesc;

			hr = md3dDevice->CreateBlendState(&blendDesc, &mBlendState);
		}
	}

	void D3DSample::initShaderResource()
	{
		HRESULT hr = S_OK;

		// init shader and inputLayout
		{
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TANGENT" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "UV" , 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			D3D11_INPUT_ELEMENT_DESC skinnedlayout[] =
			{
				{ "POSITION" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TANGENT" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "UV" , 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "INDICES" , 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "WEIGHTS" , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			ID3DBlob* vertexShaderBuffer = nullptr;
			hr = D3DHelper::CompileShaderFromFile(L"BasicVertexShader.hlsl", "main", "vs_5_0", &vertexShaderBuffer);
			assert(SUCCEEDED(hr));

			hr = md3dDevice->CreateInputLayout(
				layout,
				ARRAYSIZE(layout),
				vertexShaderBuffer->GetBufferPointer(),
				vertexShaderBuffer->GetBufferSize(),
				&mInputLayout);
			assert(SUCCEEDED(hr));

			hr = md3dDevice->CreateVertexShader(
				vertexShaderBuffer->GetBufferPointer(),
				vertexShaderBuffer->GetBufferSize(),
				NULL,
				&mVertexShader);
			assert(SUCCEEDED(hr));
			ReleaseCOM(vertexShaderBuffer);

			hr = D3DHelper::CompileShaderFromFile(L"SkinnedVertexShader.hlsl", "main", "vs_5_0", &vertexShaderBuffer);
			assert(SUCCEEDED(hr));

			hr = md3dDevice->CreateInputLayout(
				skinnedlayout,
				ARRAYSIZE(skinnedlayout),
				vertexShaderBuffer->GetBufferPointer(),
				vertexShaderBuffer->GetBufferSize(),
				&mSkinnedInputLayout);
			assert(SUCCEEDED(hr));

			hr = md3dDevice->CreateVertexShader(
				vertexShaderBuffer->GetBufferPointer(),
				vertexShaderBuffer->GetBufferSize(),
				NULL,
				&mSkinnedVertexShader);
			assert(SUCCEEDED(hr));
			ReleaseCOM(vertexShaderBuffer);

			ID3DBlob* pixelShaderBuffer = nullptr;
			hr = D3DHelper::CompileShaderFromFile(L"BasicPixelShader.hlsl", "main", "ps_5_0", &pixelShaderBuffer);
			assert(SUCCEEDED(hr));

			hr = md3dDevice->CreatePixelShader(
				pixelShaderBuffer->GetBufferPointer(),
				pixelShaderBuffer->GetBufferSize(),
				NULL,
				&mPixelShader);
			assert(SUCCEEDED(hr));
		}

		// constant buffer
		{
			D3D11_BUFFER_DESC bufferDesc = { 0, };
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			static_assert(sizeof(VSConstantBufferInfo) % 16 == 0, "Struct : ConstantBufferMat align error");
			bufferDesc.ByteWidth = sizeof(VSConstantBufferInfo);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			hr = md3dDevice->CreateBuffer(&bufferDesc, nullptr, &mVSConstnat);
			assert(SUCCEEDED(hr));

			bufferDesc = { 0, };
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			static_assert(sizeof(PSConstantBufferInfo) % 16 == 0, "Struct : ConstantBufferMat align error");
			bufferDesc.ByteWidth = sizeof(PSConstantBufferInfo);
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			hr = md3dDevice->CreateBuffer(&bufferDesc, nullptr, &mPSConstnat);
			assert(SUCCEEDED(hr));
		}
	}

	void D3DSample::preRender()
	{
		float color[4] = { 1, 1, 1, 1 };

		md3dContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);

		float blendFactors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		md3dContext->OMSetBlendState(mBlendState, blendFactors, 0xffffffff);

		md3dContext->ClearRenderTargetView(mRenderTargetView, color);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH, 1.f, 0.f);

		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		md3dContext->VSSetConstantBuffers(0, 1, &mVSConstnat);

		md3dContext->PSSetSamplers(0, 1, &mSamplerLinear);

		md3dContext->PSSetShader(mPixelShader, nullptr, 0);
		md3dContext->PSSetConstantBuffers(0, 1, &mPSConstnat);

		if (GetAsyncKeyState('E') & 0x8000)
		{
			md3dContext->RSSetState(mWireFrameRasterizerState);
		}
		else
		{
			md3dContext->RSSetState(mSolidRasterizerState);
		}
	}

	void D3DSample::postRender()
	{
		mSwapChain->Present(0, 0);
	}
}