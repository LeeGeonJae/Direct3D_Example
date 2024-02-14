#pragma once

#include <vector>
#include <directxtk/SimpleMath.h>

#include "D3dProcessor.h"
#include "Camera.h"
#include "Octree.h"
#include "Object.h"

namespace frustumCulling
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
		void initBasic32();
		void buildSkullGeometry();

	private:
		// 10, 50 정도가 형태 보여주기에는 좋음
		// 40, 50 정도가 최적화 보여주기에 좋음
		enum { SKULL_COUNT_SQRT_3 = 10 };
		enum { INTERVAL = 50 };

		Basic32* mBasic32;
		Camera mCam;
		POINT mLastMousePos;

		ID3D11Buffer* mSkullVB;
		ID3D11Buffer* mSkullIB;
		BoundingBox mSkullBoundingBox;
		UINT mSkullIndexCount;

		std::vector<Object*> mObjects;
		bool mbIsOnCulling;

		Octree mOctree;
		bool mbUseOctree;
		ID3D11Buffer* mBoxIB;
		ID3D11Buffer* mBoxVB;
	};
}