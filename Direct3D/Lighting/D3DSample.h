#pragma once

#include <directxtk/SimpleMath.h>
#include <map>

#include "D3dProcessor.h"
#include "LightHelper.h"
#include "MathHelper.h"
#include "GeometryGenerator.h"
#include "Waves.h"

namespace lighting
{
	using namespace common;
	using namespace DirectX::SimpleMath;

	struct Vertex
	{
		DirectX::SimpleMath::Vector3 Position;
		DirectX::SimpleMath::Vector3 Normal;
	};

	struct CBPerObject
	{  
		DirectX::SimpleMath::Matrix World;
		DirectX::SimpleMath::Matrix WorldInvTranspose;
		DirectX::SimpleMath::Matrix WorldViewProj;
		Material Material;
	};

	struct CBPerFrame
	{
		DirectionLight DirLight;
		PointLight PointLight;
		SpotLight SpotLight;
		DirectX::SimpleMath::Vector3 EyePosW;
		float unused;
	};

	class Model
	{
		ID3D11Buffer* VertexBuffer;
		ID3D11Buffer* IndexBuffer;
		Material Material;
		UINT IndexCount;
	};

	enum class eShapeType
	{
		Box,
		Grid,
		Sphere,
		Cyliner
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
		void releaseModel(Model* model);

		void drawModel(const Model& model, const Matrix& worldMatrix = Matrix::Identity);
		void drawObject(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, UINT indexSize, const Material& material, const DirectX::SimpleMath::Matrix& worldMatrix = DirectX::SimpleMath::Matrix::Identity);
		void drawShape(eShapeType shapeType, const DirectX::SimpleMath::Matrix& worldMatrix = DirectX::SimpleMath::Matrix::Identity);

		float getHillHeight(float x, float z) const;
		DirectX::SimpleMath::Vector3 getHillNormal(float x, float z) const;

		void buildGeometryBuffers();
		void buildLand();
		void buildWaves();
		void buildShape();
		void buildSkull();
		void buildConstantBuffer();
		void buildShader();
		void buildVertexLayout();

	private:
		ID3D11Buffer* mPerObjectCB;
		ID3D11Buffer* mPerFrameCB;
		CBPerObject mCBPerObject;
		CBPerFrame mCBPerFrame;

		Waves mWaves;

		Model mLandModel;
		Model mWavesModel;
		Model mSkullModel;

		DirectX::SimpleMath::Matrix mLandWorld;
		DirectX::SimpleMath::Matrix mWaveWorld;
		DirectX::SimpleMath::Matrix mSkullWorld;

		std::vector<UINT> mVertexOffsets;
		std::vector<UINT> mIndexOffsets;
		std::vector<UINT> mIndexCounts;
		std::vector<std::pair<eShapeType, DirectX::SimpleMath::Matrix>> mWorldMatrixs;
		std::map<eShapeType, Material> mShapeMaterials;
		ID3D11Buffer* mShapeVB;
		ID3D11Buffer* mShapeIB;

		DirectionLight mDirLight;
		PointLight mPointLight;
		SpotLight mSpotLight;
		DirectX::SimpleMath::Vector3 mEyePosW;

		ID3D11VertexShader* mVertexShader;
		ID3DBlob* mVertexShaderBuffer;
		ID3D11PixelShader* mPixelShader;
		ID3D11InputLayout* mInputLayout;

		DirectX::SimpleMath::Matrix mView;
		DirectX::SimpleMath::Matrix mProj;

		float mTheta;
		float mPhi;
		float mRadius;

		POINT mLastMousePos;
	};
}