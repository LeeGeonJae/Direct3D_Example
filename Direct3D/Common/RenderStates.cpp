#include "pch.h"

#include "RenderStates.h"
#include "D3DUtil.h"

namespace common
{
	// RasterizerState
	ID3D11RasterizerState* RenderStates::WireFrameRS = nullptr;
	ID3D11RasterizerState* RenderStates::NoCullRS = nullptr;
	ID3D11RasterizerState* RenderStates::CullClockwiseRS = nullptr;
	ID3D11RasterizerState* RenderStates::DepthRS = nullptr;

	// BlendState
	ID3D11BlendState* RenderStates::AlphaToCoverageBS = nullptr;
	ID3D11BlendState* RenderStates::TransparentBS = nullptr;
	ID3D11BlendState* RenderStates::NoRenderTargetWritesBS = nullptr;
	ID3D11BlendState* RenderStates::AdditiveBlending = nullptr;

	// DepthStecilState
	ID3D11DepthStencilState* RenderStates::MarkMirrorDSS = nullptr;
	ID3D11DepthStencilState* RenderStates::DrawReflectionDSS = nullptr;
	ID3D11DepthStencilState* RenderStates::NoDoubleBlendDSS = nullptr;
	ID3D11DepthStencilState* RenderStates::LessEqualDSS = nullptr;
	ID3D11DepthStencilState* RenderStates::DisableDepthDSS = nullptr;
	ID3D11DepthStencilState* RenderStates::NoDepthWrites = nullptr;
	ID3D11DepthStencilState* RenderStates::EqualsDSS = nullptr;

	void RenderStates::Init(ID3D11Device* device)
	{
		Destroy();

		// WireFrameRS
		D3D11_RASTERIZER_DESC wireframeDesc;
		ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
		wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
		wireframeDesc.CullMode = D3D11_CULL_BACK;
		wireframeDesc.FrontCounterClockwise = false;
		wireframeDesc.DepthClipEnable = true;
		HR(device->CreateRasterizerState(&wireframeDesc, &WireFrameRS));

		// NoCullRS
		D3D11_RASTERIZER_DESC solidDesc;
		ZeroMemory(&solidDesc, sizeof(D3D11_RASTERIZER_DESC));
		solidDesc.FillMode = D3D11_FILL_SOLID;
		solidDesc.CullMode = D3D11_CULL_NONE;
		solidDesc.FrontCounterClockwise = false;
		solidDesc.DepthClipEnable = true;
		HR(device->CreateRasterizerState(&solidDesc, &NoCullRS));

		// CullClockwiseRS
		D3D11_RASTERIZER_DESC cullClockwiseDesc;
		ZeroMemory(&cullClockwiseDesc, sizeof(D3D11_RASTERIZER_DESC));
		cullClockwiseDesc.FillMode = D3D11_FILL_SOLID;
		cullClockwiseDesc.CullMode = D3D11_CULL_BACK;
		cullClockwiseDesc.FrontCounterClockwise = true;
		cullClockwiseDesc.DepthClipEnable = true;
		HR(device->CreateRasterizerState(&cullClockwiseDesc, &CullClockwiseRS));

		// DepthRS
		D3D11_RASTERIZER_DESC depthDesc;
		ZeroMemory(&depthDesc, sizeof(D3D11_RASTERIZER_DESC));
		depthDesc.FillMode = D3D11_FILL_SOLID;
		depthDesc.CullMode = D3D11_CULL_BACK;
		// bias = (float)DepthBias * r + SlopeScaledDepthBias * maxDepthSlope
		// r = 1 / 2^24 // 24비트 깊이 버퍼의 경우
		depthDesc.DepthBias = 100000; // 적용할 고정된 편향치
		depthDesc.DepthBiasClamp = 0.0f; // 허용되는 최대 깊이 편향치
		depthDesc.SlopeScaledDepthBias = 1.f; // 기울기 편향치 비례 계수
		HR(device->CreateRasterizerState(&depthDesc, &DepthRS));

		// AlphaToCoverageBS
		D3D11_BLEND_DESC alphaToCoverageDesc = { 0, };
		alphaToCoverageDesc.AlphaToCoverageEnable = true; // 다중샘플링 시에 알파 포괄도 변환 여부
		alphaToCoverageDesc.IndependentBlendEnable = false; // 렌더 대상을 하나만 쓸 때 false
		alphaToCoverageDesc.RenderTarget[0].BlendEnable = false; // 혼합을 사용하지 않음
		alphaToCoverageDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL; // 모두 쓰기
		HR(device->CreateBlendState(&alphaToCoverageDesc, &AlphaToCoverageBS));

		// TransparentBS
		D3D11_BLEND_DESC transparentDesc = { 0, };
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

		// NoRenderTargetWritesBS
		D3D11_BLEND_DESC noRenderTargetWritesDesc = { 0, };
		noRenderTargetWritesDesc.AlphaToCoverageEnable = false;
		noRenderTargetWritesDesc.IndependentBlendEnable = false;
		noRenderTargetWritesDesc.RenderTarget[0].BlendEnable = false; // 혼합 비활성화
		noRenderTargetWritesDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		noRenderTargetWritesDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		noRenderTargetWritesDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		noRenderTargetWritesDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		noRenderTargetWritesDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		noRenderTargetWritesDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		noRenderTargetWritesDesc.RenderTarget[0].RenderTargetWriteMask = 0; // 렌더 대상에 아무것도 쓰지 않겠다.!
		HR(device->CreateBlendState(&noRenderTargetWritesDesc, &NoRenderTargetWritesBS));

		//AdditiveBlending
		D3D11_BLEND_DESC additiveBlendingDesc = { 0, };
		additiveBlendingDesc.AlphaToCoverageEnable = false;
		additiveBlendingDesc.IndependentBlendEnable = false;
		additiveBlendingDesc.RenderTarget[0].BlendEnable = true;
		additiveBlendingDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		additiveBlendingDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		additiveBlendingDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		additiveBlendingDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		additiveBlendingDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		additiveBlendingDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		additiveBlendingDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
		HR(device->CreateBlendState(&additiveBlendingDesc, &AdditiveBlending));

		// MarkMirrorDSS
		D3D11_DEPTH_STENCIL_DESC mirrorDesc;
		mirrorDesc.DepthEnable = true; // 깊이 판정 활성화
		mirrorDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 갱신 없이 판정만 씀
		mirrorDesc.DepthFunc = D3D11_COMPARISON_LESS; // 깊이 판정에 less 함수를 사용
		mirrorDesc.StencilEnable = true; // 스텐실 판정 활성화
		mirrorDesc.StencilReadMask = 0xff; // 비교 함수 시 모두 활성화
		mirrorDesc.StencilWriteMask = 0xff; // 스텐실에 쓰기 시 모두 활성화
		mirrorDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP; // 실패했을 경우 변경하지 않음
		mirrorDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP; // 스텐실 판정만 성공시 변경하지 않는다.
		mirrorDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE; // 성공 시 스텐실 기준 값으로 덮어쓴다.
		mirrorDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS; // 스텐실 판정은 항상 참을 반환
		mirrorDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		mirrorDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		mirrorDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		mirrorDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		HR(device->CreateDepthStencilState(&mirrorDesc, &MarkMirrorDSS));

		// DrawReflectionDSS
		D3D11_DEPTH_STENCIL_DESC drawReflectionDesc;
		drawReflectionDesc.DepthEnable = true;
		drawReflectionDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		drawReflectionDesc.DepthFunc = D3D11_COMPARISON_LESS;
		drawReflectionDesc.StencilEnable = true;
		drawReflectionDesc.StencilReadMask = 0xff;
		drawReflectionDesc.StencilWriteMask = 0xff;
		drawReflectionDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		drawReflectionDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		drawReflectionDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		drawReflectionDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
		drawReflectionDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		drawReflectionDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		drawReflectionDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		drawReflectionDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
		HR(device->CreateDepthStencilState(&drawReflectionDesc, &DrawReflectionDSS));

		// NoDoubleBlendDSS
		D3D11_DEPTH_STENCIL_DESC noDoubleBlendDesc;
		noDoubleBlendDesc.DepthEnable = true;
		noDoubleBlendDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		noDoubleBlendDesc.DepthFunc = D3D11_COMPARISON_LESS;
		noDoubleBlendDesc.StencilEnable = true;
		noDoubleBlendDesc.StencilReadMask = 0xff;
		noDoubleBlendDesc.StencilWriteMask = 0xff;
		noDoubleBlendDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		noDoubleBlendDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		noDoubleBlendDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
		noDoubleBlendDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
		noDoubleBlendDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		noDoubleBlendDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		noDoubleBlendDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
		noDoubleBlendDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
		HR(device->CreateDepthStencilState(&noDoubleBlendDesc, &NoDoubleBlendDSS));

		// LessEqualDSS
		D3D11_DEPTH_STENCIL_DESC lessEqualDesc;
		lessEqualDesc.DepthEnable = true;
		lessEqualDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		lessEqualDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; // 깊이 판정할 떄 같은 넘도 통과
		lessEqualDesc.StencilEnable = false;

		HR(device->CreateDepthStencilState(&lessEqualDesc, &LessEqualDSS));

		// DisableDepthDSS
		D3D11_DEPTH_STENCIL_DESC disableDepthDesc = {};
		disableDepthDesc.DepthEnable = false;
		disableDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		disableDepthDesc.DepthFunc = D3D11_COMPARISON_LESS;
		disableDepthDesc.StencilEnable = false;

		HR(device->CreateDepthStencilState(&disableDepthDesc, &DisableDepthDSS));

		// NoDepthWrites
		D3D11_DEPTH_STENCIL_DESC noDepthWritesDesc = {};
		noDepthWritesDesc.DepthEnable = true;
		noDepthWritesDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		noDepthWritesDesc.DepthFunc = D3D11_COMPARISON_LESS;
		noDepthWritesDesc.StencilEnable = false;
		HR(device->CreateDepthStencilState(&noDepthWritesDesc, &NoDepthWrites));

		// EqualDSS
		D3D11_DEPTH_STENCIL_DESC equalsDesc;
		ZeroMemory(&equalsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		equalsDesc.DepthEnable = true;
		equalsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		equalsDesc.DepthFunc = D3D11_COMPARISON_EQUAL;
		HR(device->CreateDepthStencilState(&equalsDesc, &EqualsDSS));
	}

	void RenderStates::Destroy()
	{
		ReleaseCOM(WireFrameRS);
		ReleaseCOM(NoCullRS);
		ReleaseCOM(CullClockwiseRS);
		ReleaseCOM(DepthRS);

		ReleaseCOM(AlphaToCoverageBS);
		ReleaseCOM(TransparentBS);
		ReleaseCOM(NoRenderTargetWritesBS);
		ReleaseCOM(AdditiveBlending);

		ReleaseCOM(MarkMirrorDSS);
		ReleaseCOM(DrawReflectionDSS);
		ReleaseCOM(NoDoubleBlendDSS);
		ReleaseCOM(LessEqualDSS);
		ReleaseCOM(DisableDepthDSS);
		ReleaseCOM(NoDepthWrites);
		ReleaseCOM(EqualsDSS);
	}
}