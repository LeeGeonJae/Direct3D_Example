#pragma once

#include <d3d11.h>

#include "D3DUtil.h"

class RenderStates
{
public:
	static void Init(ID3D11Device* device)
	{
		Destroy();

		D3D11_RASTERIZER_DESC wireframeDesc;
		ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
		wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
		wireframeDesc.CullMode = D3D11_CULL_BACK;
		wireframeDesc.FrontCounterClockwise = false;
		wireframeDesc.DepthClipEnable = true;

		HR(device->CreateRasterizerState(&wireframeDesc, &WireFrameRS));

		D3D11_RASTERIZER_DESC solidDesc;
		ZeroMemory(&solidDesc, sizeof(D3D11_RASTERIZER_DESC));
		solidDesc.FillMode = D3D11_FILL_SOLID;
		solidDesc.CullMode = D3D11_CULL_NONE;
		solidDesc.FrontCounterClockwise = false;
		solidDesc.DepthClipEnable = true;

		HR(device->CreateRasterizerState(&solidDesc, &NoCullRS));

		// 알파포괄도? 아직 잘 모르겠음
		D3D11_BLEND_DESC alphaToCoverageDesc = { 0 };
		alphaToCoverageDesc.AlphaToCoverageEnable = true;
		alphaToCoverageDesc.IndependentBlendEnable = false;
		alphaToCoverageDesc.RenderTarget[0].BlendEnable = false;
		alphaToCoverageDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HR(device->CreateBlendState(&alphaToCoverageDesc, &AlphaToCoverageBS));

		// 요놈은 투명도를 다루기 위한 블랜드 스테이트
		D3D11_BLEND_DESC transparentDesc = { 0 };
		transparentDesc.AlphaToCoverageEnable = false;
		transparentDesc.IndependentBlendEnable = false;

		transparentDesc.RenderTarget[0].BlendEnable = true;
		transparentDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; // 원본 혼합 계수 원본 알파값
		transparentDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; // 대상 혼합 계수 (1 - src)
		transparentDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD; // 두 픽셀 색상을 더함
		transparentDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE; // 1
		transparentDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO; // 0 
		transparentDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD; // 더하기
		transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HR(device->CreateBlendState(&transparentDesc, &TransparentBS));
	}

	static void Destroy()
	{
		ReleaseCOM(WireFrameRS);
		ReleaseCOM(NoCullRS);
		ReleaseCOM(AlphaToCoverageBS);
		ReleaseCOM(TransparentBS);
	}

	static ID3D11RasterizerState* WireFrameRS;
	static ID3D11RasterizerState* NoCullRS;

	static ID3D11BlendState* AlphaToCoverageBS;
	static ID3D11BlendState* TransparentBS;
};