#include "pch.h"

#include <WindowsX.h>
#include <sstream>

#include "D3DProcessor.h"
#include "D3DUtil.h"

#pragma comment (lib, "d3d11.lib")

namespace common
{
	namespace
	{
		D3DProcessor* gd3dApp = 0;
	}

	LRESULT CALLBACK
		MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return gd3dApp->MsgProc(hwnd, msg, wParam, lParam);
	}

	D3DProcessor::D3DProcessor(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: mhInstance(hInstance)
		, mhWnd(NULL)
		, mbPaused(false)
		, mbMinimized(false)
		, mbMaximized(false)
		, mbResizing(false)
		, m4xMsaaQuality(0)

		, md3dDevice(nullptr)
		, md3dContext(nullptr)
		, mSwapChain(nullptr)
		, mDepthStencilBuffer(nullptr)
		, mRenderTargetView(nullptr)
		, mDepthStencilView(nullptr)
		, mScreenViewport{ 0, }

		, mTitle(name)
		, md3dDriverType(D3D_DRIVER_TYPE_HARDWARE)
		, mWidth(width)
		, mHeight(height)
		, mEnable4xMsaa(false)
	{
		gd3dApp = this;
	}

	D3DProcessor::~D3DProcessor()
	{
		ReleaseCOM(mRenderTargetView);
		ReleaseCOM(mDepthStencilView);
		ReleaseCOM(mSwapChain);
		ReleaseCOM(mDepthStencilBuffer);

		if (md3dContext)
			md3dContext->ClearState();

		ReleaseCOM(md3dContext);
		ReleaseCOM(md3dDevice);
	}

	int D3DProcessor::Run()
	{
		MSG msg = { 0 };

		mTimer.Reset();

		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				mTimer.Update();

				if (!mbPaused)
				{
					CalculateFrameStats();
					Update(mTimer.GetDeltaTime());
					Render();
				}
				else
				{
					Sleep(100);
				}
			}
		}

		return (int)msg.wParam;
	}

	bool D3DProcessor::Init()
	{
		// init Window
		{
			WNDCLASS wc;
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = MainWndProc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = mhInstance;
			wc.hIcon = LoadIcon(0, IDI_APPLICATION);
			wc.hCursor = LoadCursor(0, IDC_ARROW);
			wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
			wc.lpszMenuName = 0;
			wc.lpszClassName = L"D3DWndClassName";

			if (!RegisterClass(&wc))
			{
				MessageBox(0, L"RegisterClass Failed.", 0, 0);
				return false;
			}

			RECT windowRect = { 0, 0, static_cast<LONG>(GetWidth()), static_cast<LONG>(GetHeight()) };
			AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

			int width = windowRect.right - windowRect.left;
			int height = windowRect.bottom - windowRect.top;

			mhWnd = CreateWindow(
				L"D3DWndClassName",
				mTitle.c_str(),
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				width, height,
				0, 0, mhInstance, 0);

			if (!mhWnd)
			{
				MessageBox(0, L"CreateWindow Failed.", 0, 0);
				return false;
			}

			ShowWindow(mhWnd, SW_SHOW);
			UpdateWindow(mhWnd);
		}

		// init d3d 
		{
			UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
			createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

			D3D_FEATURE_LEVEL featureLevel;
			HRESULT hr = D3D11CreateDevice(
				0,                 // default adapter
				md3dDriverType,
				0,                 // no software device
				createDeviceFlags,
				0, 0,              // default feature level array
				D3D11_SDK_VERSION,
				&md3dDevice,
				&featureLevel,
				&md3dContext);

			if (FAILED(hr))
			{
				MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
				return false;
			}

			if (featureLevel != D3D_FEATURE_LEVEL_11_0)
			{
				MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
				return false;
			}

			md3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality);
			assert(m4xMsaaQuality > 0);

			DXGI_SWAP_CHAIN_DESC sd;
			sd.BufferDesc.Width = mWidth;
			sd.BufferDesc.Height = mHeight;
			sd.BufferDesc.RefreshRate.Numerator = 60;
			sd.BufferDesc.RefreshRate.Denominator = 1;
			sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

			if (mEnable4xMsaa)
			{
				sd.SampleDesc.Count = 4;
				sd.SampleDesc.Quality = m4xMsaaQuality - 1;
			}
			else
			{
				sd.SampleDesc.Count = 1;
				sd.SampleDesc.Quality = 0;
			}

			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.BufferCount = 1;
			sd.OutputWindow = mhWnd;
			sd.Windowed = true;
			sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			sd.Flags = 0;

			IDXGIDevice* dxgiDevice = 0;
			md3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

			IDXGIAdapter* dxgiAdapter = 0;
			dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

			IDXGIFactory* dxgiFactory = 0;
			dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

			dxgiFactory->CreateSwapChain(md3dDevice, &sd, &mSwapChain);

			ReleaseCOM(dxgiDevice);
			ReleaseCOM(dxgiAdapter);
			ReleaseCOM(dxgiFactory);

			OnResize();
		}

		return true;
	}

	void D3DProcessor::OnResize()
	{
		assert(md3dContext);
		assert(md3dDevice);
		assert(mSwapChain);

		ReleaseCOM(mRenderTargetView);
		ReleaseCOM(mDepthStencilView); // 랜더링파이프라인에 텍스처를 연결하기 위한 인터페이스 클래스
		ReleaseCOM(mDepthStencilBuffer); // 텍스처

		mSwapChain->ResizeBuffers(1, mWidth, mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
		ID3D11Texture2D* backBuffer;
		mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
		md3dDevice->CreateRenderTargetView(backBuffer, 0, &mRenderTargetView);
		ReleaseCOM(backBuffer);

		D3D11_TEXTURE2D_DESC depthStencilDesc;

		depthStencilDesc.Width = mWidth;
		depthStencilDesc.Height = mHeight;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		if (mEnable4xMsaa)
		{
			depthStencilDesc.SampleDesc.Count = 4;
			depthStencilDesc.SampleDesc.Quality = m4xMsaaQuality - 1;
		}
		else
		{
			depthStencilDesc.SampleDesc.Count = 1;
			depthStencilDesc.SampleDesc.Quality = 0;
		}

		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		md3dDevice->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer);
		md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, 0, &mDepthStencilView);

		md3dContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);

		mScreenViewport.TopLeftX = 0;
		mScreenViewport.TopLeftY = 0;
		mScreenViewport.Width = static_cast<float>(mWidth);
		mScreenViewport.Height = static_cast<float>(mHeight);
		mScreenViewport.MinDepth = 0.0f;
		mScreenViewport.MaxDepth = 1.0f;

		md3dContext->RSSetViewports(1, &mScreenViewport);
	}

	LRESULT D3DProcessor::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				mbPaused = true;
				mTimer.Pause();
			}
			else
			{
				mbPaused = false;
				mTimer.Start();
			}
			return 0;
		case WM_SIZE:
			mWidth = LOWORD(lParam);
			mHeight = HIWORD(lParam);
			if (md3dDevice)
			{
				if (wParam == SIZE_MINIMIZED)
				{
					mbPaused = true;
					mbMinimized = true;
					mbMaximized = false;
				}
				else if (wParam == SIZE_MAXIMIZED)
				{
					mbPaused = false;
					mbMinimized = false;
					mbMaximized = true;
					OnResize();
				}
				else if (wParam == SIZE_RESTORED)
				{
					if (mbMinimized)
					{
						mbPaused = false;
						mbMinimized = false;
						OnResize();
					}
					else if (mbMaximized)
					{
						mbPaused = false;
						mbMaximized = false;
						OnResize();
					}
					else if (mbResizing)
					{
					}
					else
					{
						OnResize();
					}
				}
			}
			return 0;

		case WM_ENTERSIZEMOVE:
			mbPaused = true;
			mbResizing = true;
			mTimer.Pause();
			return 0;
		case WM_EXITSIZEMOVE:
			mbPaused = false;
			mbResizing = false;
			mTimer.Start();
			OnResize();
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_MENUCHAR:
			return MAKELRESULT(0, MNC_CLOSE);
		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
			return 0;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_MOUSEMOVE:
			OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	void D3DProcessor::CalculateFrameStats()
	{
		// Code computes the average frames per second, and also the 
		// average time it takes to render one frame.  These stats 
		// are appended to the window caption bar.

		static int frameCnt = 0;
		static float timeElapsed = 0.0f;

		frameCnt++;

		// Compute averages over one second period.
		if ((mTimer.GetTotalTime() - timeElapsed) >= 1.0f)
		{
			float fps = (float)frameCnt; // fps = frameCnt / 1
			float mspf = 1000.0f / fps;

			std::wostringstream outs;
			outs.precision(6);
			outs << mTitle << L"    "
				<< L"FPS: " << fps << L"    "
				<< L"Frame Time: " << mspf << L" (ms)";
			SetWindowText(mhWnd, outs.str().c_str());

			// Reset for next average.
			frameCnt = 0;
			timeElapsed += 1.0f;
		}
	}
}