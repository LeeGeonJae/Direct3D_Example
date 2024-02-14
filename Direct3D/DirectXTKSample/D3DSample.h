#pragma once

#include <directxtk/SimpleMath.h>
#include <memory>

#include "D3dProcessor.h"
#include "Camera.h"

#include "directxtk/CommonStates.h"
#include "directxtk/DDSTextureLoader.h"
#include "directxtk/DirectXHelpers.h"
#include "directxtk/Effects.h"
#include "directxtk/GamePad.h"
#include "directxtk/GeometricPrimitive.h"
#include "directxtk/Keyboard.h"
#include "directxtk/Model.h"
#include "directxtk/Mouse.h"
#include "directxtk/PrimitiveBatch.h"
#include "directxtk/SimpleMath.h"
#include "directxtk/SpriteBatch.h"
#include "directxtk/SpriteFont.h"
#include "directxtk/VertexTypes.h"

namespace initalization
{
	class Basic32;

	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	class D3DSample final : public D3DProcessor
	{
	public:
		D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name);
		~D3DSample();

		bool Init();
		void OnResize();
		void Update(float deltaTime) override;
		void Render() override;

		void OnMouseDown(WPARAM btnState, int x, int y);
		void OnMouseUp(WPARAM btnState, int x, int y);
		void OnMouseMove(WPARAM btnState, int x, int y);

	private:
		Camera mCam;
		POINT mLastMousePos;

		// DirectXTK objects.
		std::unique_ptr<DirectX::CommonStates>                                  m_states;
		std::unique_ptr<DirectX::BasicEffect>                                   m_batchEffect;
		std::unique_ptr<DirectX::EffectFactory>                                 m_fxFactory;
		std::unique_ptr<DirectX::GeometricPrimitive>                            m_shape;
		std::unique_ptr<DirectX::Model>                                         m_model;
		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_batch;
		std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
		std::unique_ptr<DirectX::SpriteFont>                                    m_font;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture1;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture2;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>                               m_batchInputLayout;
	};
}