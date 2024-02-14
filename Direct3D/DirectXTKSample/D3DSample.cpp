#include <cassert>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "Basic32.h"
#include "MathHelper.h"

namespace initalization
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
	{
	}
	D3DSample::~D3DSample()
	{
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		mCam.SetPosition(0, 0, -10);

		auto context = md3dContext;
		auto device = md3dDevice;

		m_states = std::make_unique<CommonStates>(device);

		m_fxFactory = std::make_unique<EffectFactory>(device);

		m_sprites = std::make_unique<SpriteBatch>(context);

		m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

		m_batchEffect = std::make_unique<BasicEffect>(device);
		m_batchEffect->SetVertexColorEnabled(true);

		CreateInputLayoutFromEffect<VertexPositionColor>(device,
			m_batchEffect.get(),
			m_batchInputLayout.ReleaseAndGetAddressOf());

		m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");

		m_shape = GeometricPrimitive::CreateTeapot(context, 4.f, 8);

		// SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
		m_model = Model::CreateFromSDKMESH(device, L"tiny.sdkmesh", *m_fxFactory);

		// Load textures
		CreateDDSTextureFromFile(device, L"seafloor.dds", nullptr, m_texture1.ReleaseAndGetAddressOf());
		CreateDDSTextureFromFile(device, L"windowslogo.dds", nullptr, m_texture2.ReleaseAndGetAddressOf());

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
			mCam.TranslateLook(10.0f * deltaTime);

		if (GetAsyncKeyState('S') & 0x8000)
			mCam.TranslateLook(-10.0f * deltaTime);

		if (GetAsyncKeyState('A') & 0x8000)
			mCam.TranslateRight(-10.0f * deltaTime);

		if (GetAsyncKeyState('D') & 0x8000)
			mCam.TranslateRight(10.0f * deltaTime);


		mCam.UpdateViewMatrix();
	}
	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };

		md3dContext->ClearRenderTargetView(mRenderTargetView, color);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		m_sprites->Begin();
		m_sprites->Draw(m_texture2.Get(), XMFLOAT2(10, 75), nullptr, Colors::White);
		m_font->DrawString(m_sprites.get(), L"DirectXTK Simple Sample", XMFLOAT2(100, 10), Colors::Yellow);
		m_sprites->End();

		XMMATRIX local = Matrix::Identity * Matrix::CreateTranslation(-2.f, -2.f, -4.f);
		m_shape->Draw(local, mCam.GetView(), mCam.GetProj(), Colors::White, m_texture1.Get());

		const XMVECTORF32 scale = { 0.01f, 0.01f, 0.01f };
		const XMVECTORF32 translate = { 3.f, -2.f, -4.f };
		const XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(XM_PI / 2.f, 0.f, -XM_PI / 2.f);
		local = Matrix::Identity * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);
		m_model->Draw(md3dContext, *m_states, local, mCam.GetView(), mCam.GetProj());

		mSwapChain->Present(0, 0);
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
			float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.RotatePitch(dy);
			mCam.RotateAxis({ 0, 1, 0 }, dx);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}
}