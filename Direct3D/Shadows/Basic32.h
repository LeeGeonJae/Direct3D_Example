#pragma once

#include <directxtk/SimpleMath.h>

#include "D3DUtil.h"
#include "LightHelper.h"

namespace shadows
{
	using DirectX::SimpleMath::Matrix;
	using DirectX::SimpleMath::Vector2;
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Vector4;
	using common::Material;
	using common::DirectionLight;

	class Basic32
	{
	public:
		struct PerObject
		{
			Matrix World;
			Matrix WorldInvTranspose;
			Matrix WorldViewProj;
			Matrix Tex;
			Material Material;
		};

		struct PerFrame
		{
			DirectionLight DirLights[3];
			Vector3 EyePosW;
			int LigthCount;
			Vector4 FogColor;
			float FogStart;
			float FogRange;
			int bUseTexutre;
			int bUseLight;
			int bUseFog;
			int unused[3];
		};

		struct Vertex
		{
			Vector3 Pos;
			Vector3 Normal;
			Vector2 Tex;
		};

	public:
		Basic32(ID3D11Device* device, const std::wstring& filename);
		~Basic32();
		Basic32(const Basic32&) = delete;
		Basic32 operator=(const Basic32&) = delete;

		void UpdateSubresource(ID3D11DeviceContext* context);
		void Bind(ID3D11DeviceContext* context, ID3D11ShaderResourceView* SRV = nullptr);

		inline PerObject& GetPerObject();
		inline const PerObject& GetPerObject() const;
		inline PerFrame& GetPerFrame();
		inline const PerFrame& GetPerFrame() const;
		inline ID3D11InputLayout* GetInputLayout() const;
		inline ID3D11SamplerState* GetSamplerState() const;
		inline ID3D11VertexShader* GetVS() const;
		inline ID3D11PixelShader* GetPS() const;

	private:
		PerObject mPerObject;
		PerFrame mPerFrame;

		ID3D11Buffer* mPerObjectCB;
		ID3D11Buffer* mPerFrameCB;

		ID3D11VertexShader* mVertexShader;
		ID3D11PixelShader* mPixelShader;
		ID3D11InputLayout* mInputLayout;
		ID3D11SamplerState* mLinearSampler;
	};

	Basic32::PerObject& Basic32::GetPerObject()
	{
		return mPerObject;
	}
	const Basic32::PerObject& Basic32::GetPerObject() const
	{
		return mPerObject;
	}
	Basic32::PerFrame& Basic32::GetPerFrame()
	{
		return mPerFrame;
	}
	const Basic32::PerFrame& Basic32::GetPerFrame() const
	{
		return mPerFrame;
	}
	ID3D11InputLayout* Basic32::GetInputLayout() const
	{
		assert(mInputLayout != nullptr);
		return mInputLayout;
	}
	ID3D11SamplerState* Basic32::GetSamplerState() const
	{
		assert(mLinearSampler != nullptr);
		return mLinearSampler;
	}
	ID3D11VertexShader* Basic32::GetVS() const
	{
		return mVertexShader;
	}
	ID3D11PixelShader* Basic32::GetPS() const
	{
		return mPixelShader;
	}
}