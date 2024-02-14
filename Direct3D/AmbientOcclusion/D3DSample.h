#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

#include "D3dProcessor.h"
#include "Camera.h"
#include "LightHelper.h"

namespace ambientOcclusion
{
	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct PerObject
	{
		Matrix WorldViewProj;
	};

	struct AmbientOcclusion
	{
		Vector3 Pos;
		Vector3 Normal;
		Vector2 Tex;
		float AmbientAccess;
	};

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
		void buildInit();
		void buildVertexAmbientOcclusion(
			std::vector<AmbientOcclusion>& vertices,
			const std::vector<UINT>& indices);
		void buildSkullGeometryBuffers();

	private:
		PerObject mPerObject;
		ID3D11Buffer* mObjectCB;
		ID3D11InputLayout* mInputLayout;
		ID3D11VertexShader* mVSShader;
		ID3D11PixelShader* mPSShader;

		ID3D11Buffer* mSkullVB;
		ID3D11Buffer* mSkullIB;

		XMFLOAT4X4 mSkullWorld;

		UINT mSkullIndexCount;

		Camera mCam;

		POINT mLastMousePos;
	};
}