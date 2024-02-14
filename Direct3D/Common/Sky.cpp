#include "pch.h"

#include <directxtk/DDSTextureLoader.h>

#include "Sky.h"
#include "Camera.h"
#include "GeometryGenerator.h"

namespace common
{
	Sky::Sky(ID3D11Device* device, const std::wstring& cubemapFilename, float skySphereRadius)
	{
		using namespace DirectX::SimpleMath;

		// 기존 것과 동일한 방식으로 로딩한다.
		HR(DirectX::CreateDDSTextureFromFile(device,
			cubemapFilename.c_str(),
			NULL, 
			&mCubeMapSRV));

		GeometryGenerator::MeshData sphere;
		GeometryGenerator::CreateSphere(skySphereRadius, 30, 30, &sphere);

		std::vector<Vector3> vertices(sphere.Vertices.size());

		for (size_t i = 0; i < sphere.Vertices.size(); ++i)
		{
			vertices[i] = sphere.Vertices[i].Position;
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vector3) * vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(device->CreateBuffer(&vbd, &vinitData, &mVB));

		mIndexCount = sphere.Indices.size();

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(USHORT) * mIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.StructureByteStride = 0;
		ibd.MiscFlags = 0;
		std::vector<USHORT> indices16;
		indices16.assign(sphere.Indices.begin(), sphere.Indices.end());

		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices16[0];

		HR(device->CreateBuffer(&ibd, &iinitData, &mIB));
	}
	Sky::~Sky()
	{
		ReleaseCOM(mVB);
		ReleaseCOM(mIB);
		ReleaseCOM(mCubeMapSRV);
	}

	void Sky::Draw(ID3D11DeviceContext* dc, const Camera& camera)
	{
		using namespace DirectX::SimpleMath;

		// center Sky about eye in world space
		Vector3 eyePos = camera.GetPosition();
		Matrix T = Matrix::CreateTranslation(eyePos);
		Matrix WVP = T * camera.GetViewProj();

		// updatesubresource 
		// Effects::SkyFX->SetWorldViewProj(WVP); 일단 임시로 처리 나중에 연결 방법 구성
		// 바인딩 또한 동일하다.
		dc->VSSetShaderResources(0, 1, &mCubeMapSRV);
		dc->PSSetShaderResources(0, 1, &mCubeMapSRV);

		UINT stride = sizeof(Vector3);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
		dc->IASetIndexBuffer(mIB, DXGI_FORMAT_R16_UINT, 0);
		// dc->IASetInputLayout(InputLayouts::Pos);
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		dc->DrawIndexed(mIndexCount, 0, 0);
	}
}