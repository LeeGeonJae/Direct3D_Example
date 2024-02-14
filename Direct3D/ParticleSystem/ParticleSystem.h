#pragma once

#include <string>
#include <vector>

#include <directxtk/SimpleMath.h>

#include "d3dUtil.h"

namespace common
{
	class Camera;
}

namespace particleSystem
{
	using namespace common;
	using namespace DirectX::SimpleMath;

	class ParticleEffect;


	class ParticleSystem
	{
	public:
		struct PerFrame
		{
			Matrix ViewProj;
			Vector3 EyePosW;
			float GameTime;
			Vector3 EmitPosW;
			float TimeStep;
			Vector3 EmitDirW;
			float unused;
		};

		struct Vertex
		{
			Vector3 InitialPos;
			Vector3 InitialVel;
			Vector2 Size;
			float Age;
			unsigned int Type;
		};

	public:
		ParticleSystem();
		~ParticleSystem();
		ParticleSystem(const ParticleSystem& rhs) = delete;
		ParticleSystem& operator=(const ParticleSystem& rhs) = delete;

		void Init(ID3D11Device* device, const  std::wstring& shaderfilename, ID3D11ShaderResourceView* texArraySRV,
			ID3D11ShaderResourceView* randomTexSRV, UINT maxParticles);

		void Reset();
		void Update(float dt, float gameTime);
		void Draw(ID3D11DeviceContext* dc, const Camera& cam);

		void SetEyePos(const Vector3& eyePosW);
		void SetEmitPos(const Vector3& emitPosW);
		void SetEmitDir(const Vector3& emitDirW);

		float GetAge()const;

	public:
		PerFrame mPerFrame;
		ID3D11InputLayout* mInputLayout;
		ID3D11VertexShader* mStreamOutVS;
		ID3D11VertexShader* mDrawVS;
		ID3D11GeometryShader* mStreamOutGS;
		ID3D11GeometryShader* mDrawGS;
		ID3D11PixelShader* mDrawPS;
		ID3D11Buffer* mFrameCB;
		ID3D11SamplerState* mSamLinear;

		UINT mMaxParticles;
		bool mFirstRun;

		float mGameTime;
		float mTimeStep;
		float mAge;

		Vector3 mEyePosW;
		Vector3 mEmitPosW;
		Vector3 mEmitDirW;

		ID3D11Buffer* mInitVB;
		ID3D11Buffer* mDrawVB;
		ID3D11Buffer* mStreamOutVB;

		ID3D11ShaderResourceView* mTexArraySRV;
		ID3D11ShaderResourceView* mRandomTexSRV;
	};
}