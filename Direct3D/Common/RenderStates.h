#pragma once

namespace common
{
	class RenderStates
	{
	public:
		static void Init(ID3D11Device* device);
		static void Destroy();

	public:
		// RasterizerState
		static ID3D11RasterizerState* WireFrameRS; // 와이어프레임으로 렌더링
		static ID3D11RasterizerState* NoCullRS; // 후면제거를 하지 않음
		static ID3D11RasterizerState* CullClockwiseRS; // 반시계방향을 전면으로 판정
		static ID3D11RasterizerState* DepthRS;

		// BlendState
		static ID3D11BlendState* AlphaToCoverageBS; // 알파포괄도
		static ID3D11BlendState* TransparentBS; // 원본 픽셀의 알파값으로 혼합하겠다.
		static ID3D11BlendState* NoRenderTargetWritesBS; // 렌더링과 혼합을 하지 않는다.
		static ID3D11BlendState* AdditiveBlending; // 렌더링과 혼합을 하지 않는다.

		// DepthStecilState
		static ID3D11DepthStencilState* MarkMirrorDSS; // 깊이 판정 on, 스텐실 판정/갱신 on(항상 참/모두 통과 시 스텐실 기준값으로 쓰기)
		static ID3D11DepthStencilState* DrawReflectionDSS; // 깊이 판정/갱신 on, 스텐실 판정 on (같을 시 참)
		static ID3D11DepthStencilState* NoDoubleBlendDSS; // 깊이 판정/갱신 on, 스텐실 판정/갱신 on(같을 시 참/모두 통과 시 increase 래핑)
		static ID3D11DepthStencilState* LessEqualDSS; // 깊이 판정/갱신 on (깊이값 같아도 통과)
		static ID3D11DepthStencilState* DisableDepthDSS;
		static ID3D11DepthStencilState* NoDepthWrites;
		static ID3D11DepthStencilState* EqualsDSS;
	};
}