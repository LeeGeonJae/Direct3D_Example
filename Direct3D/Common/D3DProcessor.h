#pragma once

#include <string>
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include "Timer.h"

namespace common
{
	class D3DProcessor
	{
	public:
		D3DProcessor(HINSTANCE hInstance, UINT width, UINT height, std::wstring name);
		virtual ~D3DProcessor();

		int Run();

		virtual bool Init();
		virtual void Update(float deltaTime) = 0;
		virtual void Render() = 0;

		virtual void OnResize();
		virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
		virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
		virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

		inline HINSTANCE GetInstance() const;
		inline HWND GetHWnd() const;
		inline UINT GetWidth() const;
		inline UINT GetHeight() const;
		inline float GetAspectRatio() const;
		inline const WCHAR* GetTitle() const;

	protected:
		void CalculateFrameStats();

	protected:
		HINSTANCE mhInstance;
		HWND mhWnd;
		bool mbPaused;
		bool mbMinimized;
		bool mbMaximized;
		bool mbResizing;
		UINT m4xMsaaQuality;

		Timer mTimer;

		ID3D11Device* md3dDevice;
		ID3D11DeviceContext* md3dContext;
		IDXGISwapChain* mSwapChain;
		ID3D11Texture2D* mDepthStencilBuffer;
		ID3D11RenderTargetView* mRenderTargetView;
		ID3D11DepthStencilView* mDepthStencilView;
		D3D11_VIEWPORT mScreenViewport;

		std::wstring mTitle;
		D3D_DRIVER_TYPE md3dDriverType;
		UINT mWidth;
		UINT mHeight;
		bool mEnable4xMsaa;
	};

	HINSTANCE D3DProcessor::GetInstance() const
	{
		return mhInstance;
	}

	HWND D3DProcessor::GetHWnd() const
	{
		return mhWnd;
	}

	UINT D3DProcessor::GetWidth() const
	{
		return mWidth;
	}

	UINT D3DProcessor::GetHeight() const
	{
		return mHeight;
	}

	const WCHAR* D3DProcessor::GetTitle() const
	{
		return mTitle.c_str();
	}

	float D3DProcessor::GetAspectRatio() const
	{
		return mWidth / static_cast<float>(mHeight);
	}
}
