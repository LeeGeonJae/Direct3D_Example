#include <cassert>
#include <fstream>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "MathHelper.h"
#include "GeometryGenerator.h"
#include "RenderStates.h"
#include "D3DSample.h"
#include "Octree.h"

namespace ambientOcclusion
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
	{
		mTitle = L"Ambient Occlusion";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 5.0f, -5.0f);
		mCam.LookAt(
			Vector3(-4.0f, 4.0f, -4.0f),
			Vector3(0.0f, 2.2f, 0.0f),
			Vector3(0.0f, 1.0f, 0.0f));

		XMMATRIX skullScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
		XMMATRIX skullOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
		XMStoreFloat4x4(&mSkullWorld, XMMatrixMultiply(skullScale, skullOffset));
	}
	D3DSample::~D3DSample()
	{
		ReleaseCOM(mSkullVB);
		ReleaseCOM(mSkullIB);
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		// 필요한 리소스를 로딩한다.
		buildInit();
		buildSkullGeometryBuffers();

		return true;
	}
	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mCam.SetLens(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 10000.0f);
	}
	void D3DSample::Update(float deltaTime)
	{
		if (GetAsyncKeyState('W') & 0x8000)
			mCam.TranslateLook(10.0f * deltaTime);

		if (GetAsyncKeyState('S') & 0x8000)
			mCam.TranslateLook(-10.0f * deltaTime);

		if (GetAsyncKeyState('A') & 0x8000)
			mCam.TranslateRight(-10.0f * deltaTime);

		if (GetAsyncKeyState('D') & 0x8000)
			mCam.TranslateRight(10.0f * deltaTime);
	}

	void D3DSample::Render()
	{
		UINT stride = sizeof(AmbientOcclusion);
		UINT offset = 0;

		mCam.UpdateViewMatrix();

		XMMATRIX view = mCam.GetView();
		XMMATRIX proj = mCam.GetProj();
		XMMATRIX viewProj = mCam.GetViewProj();

		md3dContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Silver));
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		md3dContext->IASetInputLayout(mInputLayout);
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);

		md3dContext->VSSetConstantBuffers(0, 1, &mObjectCB);
		md3dContext->VSSetShader(mVSShader, 0, 0);

		md3dContext->PSSetConstantBuffers(0, 1, &mObjectCB);
		md3dContext->PSSetShader(mPSShader, 0, 0);

		// Draw the skull.
		Matrix worldViewProj = mSkullWorld * view * proj;
		mPerObject.WorldViewProj = worldViewProj.Transpose();
		md3dContext->UpdateSubresource(mObjectCB, 0, 0, &mPerObject, 0, 0);
		md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);

		HR(mSwapChain->Present(0, 0));
	}

	void D3DSample::OnMouseDown(WPARAM btnState, int x, int y)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		SetCapture(mhWnd);
	}
	void D3DSample::OnMouseUp(WPARAM btnState, int x, int y)
	{
		ReleaseCapture();
	}
	void D3DSample::OnMouseMove(WPARAM btnState, int x, int y)
	{
		if ((btnState & MK_LBUTTON) != 0)
		{
			// Make each pixel correspond to a quarter of a degree.
			float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.RotatePitch(dy);
			mCam.RotateAxis({ 0, 1, 0 }, dx);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

	void D3DSample::buildInit()
	{
		// 입력 레이아웃
		const D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"AMBIENT",  0, DXGI_FORMAT_R32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		// 쉐이더 컴파일
		ID3D10Blob* blob;
		HR(D3DHelper::CompileShaderFromFile(L"AmbientOcclusion.hlsl", "VS", "vs_5_0", &blob));
		HR(md3dDevice->CreateVertexShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mVSShader
		));
		HR(md3dDevice->CreateInputLayout(
			inputLayoutDesc,
			ARRAYSIZE(inputLayoutDesc),
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			&mInputLayout
		));
		ReleaseCOM(blob);

		HR(D3DHelper::CompileShaderFromFile(L"AmbientOcclusion.hlsl", "PS", "ps_5_0", &blob));
		HR(md3dDevice->CreatePixelShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			NULL,
			&mPSShader
		));
		ReleaseCOM(blob);

		// 상수 버퍼
		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		static_assert(sizeof(PerObject) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(PerObject);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mObjectCB));
	}

	void D3DSample::buildVertexAmbientOcclusion(
		std::vector<AmbientOcclusion>& vertices,
		const std::vector<UINT>& indices)
	{
		UINT vcount = vertices.size();
		UINT tcount = indices.size() / 3;

		std::vector<Vector3> positions(vcount);
		for (UINT i = 0; i < vcount; ++i)
			positions[i] = vertices[i].Pos;

		// 옥트리 굽기
		Octree octree;
		octree.Build(positions, indices);

		std::vector<int> vertexSharedCount(vcount);
		for (UINT i = 0; i < tcount; ++i)
		{
			UINT i0 = indices[i * 3 + 0];
			UINT i1 = indices[i * 3 + 1];
			UINT i2 = indices[i * 3 + 2];

			XMVECTOR v0 = XMLoadFloat3(&vertices[i0].Pos);
			XMVECTOR v1 = XMLoadFloat3(&vertices[i1].Pos);
			XMVECTOR v2 = XMLoadFloat3(&vertices[i2].Pos);

			XMVECTOR edge0 = v1 - v0;
			XMVECTOR edge1 = v2 - v0;

			XMVECTOR normal = XMVector3Normalize(XMVector3Cross(edge0, edge1));

			XMVECTOR centroid = (v0 + v1 + v2) / 3.0f; // 중점


			// 자체 교차를 피하기위해 무게 중심에 노말로 아주 약간 움직인다.
			centroid += 0.001f * normal;

			const int NumSampleRays = 32;
			float numUnoccluded = 0;

			// 32번 노말기준으로 랜덤한 반구 벡터를 생성한다.
			for (int j = 0; j < NumSampleRays; ++j)
			{
				XMVECTOR randomDir = MathHelper::RandHemisphereUnitVec3(normal);

				// 옥트리를 순회하며 반직선과 충돌되었음이 감지되면 un차페도 == 도달도를 증가시킨다.
				if (!octree.RayOctreeIntersect(centroid, randomDir))
				{
					numUnoccluded++;
				}
			}

			// 주변광 도달도는 차폐되지 않은 넘 / 샘플시도한 레이 갯수
			float ambientAccess = numUnoccluded / NumSampleRays;

			// 삼각형 단위로 주변광 도달도를 누적시킨다.
			vertices[i0].AmbientAccess += ambientAccess;
			vertices[i1].AmbientAccess += ambientAccess;
			vertices[i2].AmbientAccess += ambientAccess;

			// 나중에 나눠줄 횟수도 누적시킨다.
			vertexSharedCount[i0]++;
			vertexSharedCount[i1]++;
			vertexSharedCount[i2]++;
		}

		// 정점별 주변광 도달도를 만든다.
		for (UINT i = 0; i < vcount; ++i)
		{
			vertices[i].AmbientAccess /= vertexSharedCount[i];
		}
	}

	void D3DSample::buildSkullGeometryBuffers()
	{
		std::ifstream fin("../Resource/Models/skull.txt");

		if (!fin)
		{
			MessageBox(0, L"Models/skull.txt not found.", 0, 0);
			return;
		}

		UINT vcount = 0;
		UINT tcount = 0;
		std::string ignore;

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		std::vector<AmbientOcclusion> vertices(vcount);
		for (UINT i = 0; i < vcount; ++i)
		{
			fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
			fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
		}

		fin >> ignore;
		fin >> ignore;
		fin >> ignore;

		mSkullIndexCount = 3 * tcount;
		std::vector<UINT> indices(mSkullIndexCount);
		for (UINT i = 0; i < tcount; ++i)
		{
			fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
		}

		fin.close();

		// 정점으로 주변광 차폐 계산하기
		buildVertexAmbientOcclusion(vertices, indices);

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(AmbientOcclusion) * vcount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

		//
		// Pack the indices of all the meshes into one index buffer.
		//

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * mSkullIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mSkullIB));
	}
}