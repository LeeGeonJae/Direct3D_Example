#pragma once

#include <map>
#include <directxtk/SimpleMath.h>
#include <vector>

#include "D3dProcessor.h"
#include "Camera.h"
#include "LightHelper.h"
#include "ConstantBufferStruct.h"

namespace ssao
{
	using namespace common;

	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct BoundingSphere
	{
		BoundingSphere() : Center(0.0f, 0.0f, 0.0f), Radius(0.0f) {}
		XMFLOAT3 Center;
		float Radius;
	};

	struct Basic32
	{
		Vector3 Pos;
		Vector3 Normal;
		Vector2 Tex;
	};

	class Ssao;

	class D3DSample final : public D3DProcessor
	{
		friend class Ssao;

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
		void buildAll();
		void buildBuffer();
		void buidlSRV();
		void buildInputLayout();
		void buildVS(const std::vector<std::pair<std::string, std::wstring>>& keyFileNames);
		void buildPS(const std::vector<std::pair<std::string, std::wstring>>& keyFileNames);
		void buildSamplerState();

		void drawSceneToSsaoNormalDepthMap();
		void buildShapeGeometryBuffers();
		void buildSkullGeometryBuffers();
		void buildScreenQuadGeometryBuffers();

	private:
		std::map<std::string, ID3D11Buffer*> mBuffers;
		std::map<std::string, ID3D11ShaderResourceView*> mSRVs;
		std::map<std::string, ID3D11InputLayout*> mInputLayouts;
		std::map<std::string, ID3D11VertexShader*> mVSs;
		std::map<std::string, ID3D11PixelShader*> mPSs;
		std::map<std::string, ID3D11SamplerState*> mSamplerStates;
		std::map<std::string, ID3D10Blob*> mBlobs;

		// constant buffer structure
		ObjectSsaoNormalDepth mPerObjectSsaoNormalDepth;
		ObjectBasic mObjectBasic;
		FrameBasic mFrameBasic;
		FrameSsao mFrameSsao;
		FrameSsaoBlur mFrameSsaoBlur;

		BoundingSphere mSceneBounds;

		Ssao* mSsao;

		float mLightRotationAngle;
		XMFLOAT3 mOriginalLightDir[3];
		DirectionLight mDirLights[3];
		Material mGridMat;
		Material mBoxMat;
		Material mCylinderMat;
		Material mSphereMat;
		Material mSkullMat;

		// Define transformations from local spaces to world space.
		Matrix mSphereWorld[10];
		Matrix mCylWorld[10];
		Matrix mBoxWorld;
		Matrix mGridWorld;
		Matrix mSkullWorld;

		int mBoxVertexOffset;
		int mGridVertexOffset;
		int mSphereVertexOffset;
		int mCylinderVertexOffset;

		UINT mBoxIndexOffset;
		UINT mGridIndexOffset;
		UINT mSphereIndexOffset;
		UINT mCylinderIndexOffset;

		UINT mBoxIndexCount;
		UINT mGridIndexCount;
		UINT mSphereIndexCount;
		UINT mCylinderIndexCount;

		UINT mSkullIndexCount;

		Camera mCam;

		POINT mLastMousePos;
	};
}