#pragma once

#include <map>
#include <directxtk/SimpleMath.h>
#include <vector>
#include <memory>

#include "LightHelper.h"
#include "D3dProcessor.h"
#include "Camera.h"

namespace initalization
{
	using namespace common;
	using namespace DirectX;
	using namespace SimpleMath;

	struct Vertex
	{
		Vector3 Position;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;
		Vector2 Tex;
	};

	struct CBVertex
	{
		Matrix ViewProjectionMatrix;
		Matrix SkyProjectionMatrix;
		Matrix WorldMatrix;
	};

	struct CBPixel
	{
		struct {
			Vector4 Direction;
			Vector4 Radiance;
		} lights[3];
		Vector4 EyePosition;
		Vector4 UseIBL;
	};

	struct MeshBuffer
	{
		ID3D11Buffer* vertexBuffer;
		ID3D11Buffer* indexBuffer;
		UINT stride;
		UINT offset;
		UINT numElements;
	};

	struct FrameBuffer
	{
		ID3D11Texture2D* colorTexture;
		ID3D11Texture2D* depthStencilTexture;
		ID3D11RenderTargetView* rtv;
		ID3D11ShaderResourceView* srv;
		ID3D11DepthStencilView* dsv;
		UINT width, height;
		UINT samples;
	};

	struct ShaderProgram
	{
		ID3D11VertexShader* vertexShader;
		ID3D11PixelShader* pixelShader;
		ID3D11InputLayout* inputLayout;
	};

	struct ComputeProgram
	{
		ID3D11ComputeShader* computeShader;
	};

	struct Texture
	{
		ID3D11Texture2D* texture;
		ID3D11ShaderResourceView* srv;
		ID3D11UnorderedAccessView* uav;
		UINT width, height;
		UINT levels;
	};

	class D3DSample final : public D3DProcessor
	{
	public:
		D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name);

		bool Init();
		void OnResize();
		void Update(float deltaTime) override;
		void Render() override;

		void OnMouseDown(WPARAM btnState, int x, int y);
		void OnMouseUp(WPARAM btnState, int x, int y);
		void OnMouseMove(WPARAM btnState, int x, int y);

	private:
		MeshBuffer createMeshBuffer(const std::shared_ptr<class Mesh>& mesh) const;
		ShaderProgram createShaderProgram(ID3DBlob* vsBytecode, ID3DBlob* psBytecode, const std::vector<D3D11_INPUT_ELEMENT_DESC>* inputLayoutDesc) const;
		ComputeProgram createComputeProgram(ID3DBlob* csBytecode) const;
		ID3D11SamplerState* createSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode) const;

		Texture createTexture(UINT width, UINT height, DXGI_FORMAT format, UINT levels = 0) const;
		Texture createTexture(const std::shared_ptr<class Image>& image, DXGI_FORMAT format, UINT levels = 0) const;
		Texture createTextureCube(UINT width, UINT height, DXGI_FORMAT format, UINT levels = 0) const;

		void createTextureUAV(Texture& texture, UINT mipSlice) const;

		FrameBuffer createFrameBuffer(UINT width, UINT height, UINT samples, DXGI_FORMAT colorFormat, DXGI_FORMAT depthstencilFormat) const;
		void resolveFrameBuffer(const FrameBuffer& srcfb, const FrameBuffer& dstfb, DXGI_FORMAT format) const;

		ID3D11Buffer* createConstantBuffer(const void* data, UINT size) const;
		template<typename T> ID3D11Buffer* createConstantBuffer(const T* data = nullptr) const
		{
			static_assert(sizeof(T) % 16 == 0, "must be align");
			return createConstantBuffer(data, sizeof(T));
		}

		static ID3DBlob* compileShader(const std::string& filename, const std::string& entryPoint, const std::string& profile);


	private:
		// d3d
		ID3D11InputLayout* mInputLayout;

		ShaderProgram mPBRProgram;
		ShaderProgram mSkyboxProgram;
		ShaderProgram mTonemapProgram;

		MeshBuffer mPBRModel;
		MeshBuffer mSkybox;

		FrameBuffer mFramebuffer;
		FrameBuffer mResolveFramebuffer;

		ID3D11RasterizerState* mDefaultRasterizerState;
		ID3D11DepthStencilState* mDefaultDepthStencilState;
		ID3D11DepthStencilState* mSkyboxDepthStencilState;

		Texture mAlbedoTexture;
		Texture mNormalTexture;
		Texture mMetalnessTexture;
		Texture mRoughnessTexture;
		Texture mEnvTexture;
		Texture mIrmapTexture;
		Texture mSpBRDF_LUT;

		ID3D11SamplerState* mDefaultSampler;
		ID3D11SamplerState* mComputeSampler;
		ID3D11SamplerState* mBRDFSampler;

		CBVertex mCBVertex;
		CBPixel mCBPixel;
		ID3D11Buffer* mVertexCB;
		ID3D11Buffer* mPixelCB;

		Camera mCam;
		POINT mLastMousePos;
		bool bUseIBL;
		bool bUseToneMappingAndGamma;

		struct Light
		{
			Vector3 Direction;
			Vector3 Radiance;
			bool bIsUsed;
		} mLights[3];
	};
}