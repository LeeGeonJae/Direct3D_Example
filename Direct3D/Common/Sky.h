#pragma once

#include "D3DUtil.h"

namespace common
{
	class Camera;

	class Sky
	{
	public:
		Sky(ID3D11Device* device, const std::wstring& cubemapFilename, float skySphereRadius);
		~Sky();
		Sky(const Sky& rhs) = delete;
		Sky& operator=(const Sky& rhs) = delete;

		void Draw(ID3D11DeviceContext* context, const Camera& camera);

		inline ID3D11ShaderResourceView* GetCubeMapSRV();

	private:
		ID3D11Buffer* mVB;
		ID3D11Buffer* mIB;

		ID3D11ShaderResourceView* mCubeMapSRV;

		UINT mIndexCount;
	};

	ID3D11ShaderResourceView* Sky::GetCubeMapSRV()
	{
		assert(mCubeMapSRV != nullptr);
		return mCubeMapSRV;
	}

}