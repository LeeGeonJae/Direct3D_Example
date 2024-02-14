#include <cassert>
#include <fstream>
#include <sstream>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "Basic32.h"
#include "MathHelper.h"
#include "RenderStates.h"
#include "GeometryGenerator.h"

namespace frustumCulling
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mBasic32(nullptr)
	{
		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, -150.0f);
	}
	D3DSample::~D3DSample()
	{
		SafeDelete(mBasic32);

		RenderStates::Destroy();

		ReleaseCOM(mSkullVB);
		ReleaseCOM(mSkullIB);
		ReleaseCOM(mBoxIB);
		ReleaseCOM(mBoxVB);

		for (Object* object : mObjects)
		{
			delete object;
		}

		mObjects.clear();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		RenderStates::Init(md3dDevice);
		initBasic32();
		buildSkullGeometry();

		const float HALF = SKULL_COUNT_SQRT_3 * 0.5f * INTERVAL;
		for (int i = 0; i < SKULL_COUNT_SQRT_3; ++i)
		{
			for (int j = 0; j < SKULL_COUNT_SQRT_3; ++j)
			{
				for (int k = 0; k < SKULL_COUNT_SQRT_3; ++k)
				{
					Vector3 pos = { i * INTERVAL - HALF, j * INTERVAL - HALF, k * INTERVAL - HALF };

					Object* object = new Object();
					object->World = Matrix::CreateTranslation(pos);
					mObjects.push_back(object);
				}
			}
		}

		BoundingBox octreeBox({ 0,0,0 }, { HALF + INTERVAL , HALF + INTERVAL , HALF + INTERVAL });
		mOctree.Build(octreeBox, 3, mObjects, mSkullBoundingBox);

		GeometryGenerator::MeshData mesh;
		GeometryGenerator::CreateBox(1, 1, 1, &mesh);

		std::vector<Basic32::Vertex> boxVertices;
		std::vector<UINT> boxIndecices;
		boxVertices.reserve(36);

		for (auto& vertex : mesh.Vertices)
		{
			Basic32::Vertex v;
			v.Pos = vertex.Position;
			v.Normal = vertex.Normal;
			v.Tex = vertex.TexC;
			boxVertices.push_back(v);
		}

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * mesh.Indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &mesh.Indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32::Vertex) * boxVertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		vbd.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &boxVertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));
		// mbUseOctree = true;
		mbIsOnCulling = true;

		return true;
	}
	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mCam.SetLens(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	}
	void D3DSample::Update(float deltaTime)
	{
		if (GetAsyncKeyState('W') & 0x8000)
			mCam.TranslateLook(100.0f * deltaTime);

		if (GetAsyncKeyState('S') & 0x8000)
			mCam.TranslateLook(-100.0f * deltaTime);

		if (GetAsyncKeyState('A') & 0x8000)
			mCam.TranslateRight(-100.0f * deltaTime);

		if (GetAsyncKeyState('D') & 0x8000)
			mCam.TranslateRight(100.0f * deltaTime);

		mCam.UpdateViewMatrix();

		if (GetAsyncKeyState('1') & 0x8000)
		{
			mbIsOnCulling = true;
		}
		if (GetAsyncKeyState('2') & 0x8000)
		{
			mbUseOctree = true;
		}
		if (GetAsyncKeyState('3') & 0x8000)
		{
			mbIsOnCulling = false;
			mbUseOctree = false;
		}
	}
	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		md3dContext->ClearRenderTargetView(mRenderTargetView, Black);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		UINT stride = sizeof(Basic32::Vertex);
		UINT offset = 0;

		mBasic32->Bind(md3dContext);
		md3dContext->IASetIndexBuffer(mSkullIB, DXGI_FORMAT_R32_UINT, 0);
		md3dContext->IASetVertexBuffers(0, 1, &mSkullVB, &stride, &offset);
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto& perFrame = mBasic32->GetPerFrame();
		perFrame.EyePosW = mCam.GetPosition();

		size_t renderObject = 0;

		if (!mbUseOctree)
		{
			for (const Object* object : mObjects)
			{
				if (mbIsOnCulling)
				{
					Matrix WV = object->World * mCam.GetView();
					BoundingFrustum Frustum(mCam.GetProj()); // 투영 행렬로 프러스텀 생성
					BoundingFrustum localFrustum;
					Frustum.Transform(localFrustum, WV.Invert()); // 월드 * 뷰의 역행렬로 로컬 공간으로 프로스텀 변환

					BoundingFrustum debugFrustum;
					BoundingFrustum::CreateFromMatrix(debugFrustum, WV);

					if (!localFrustum.Intersects(mSkullBoundingBox)) // 로컬 공간의 바운딩 볼륨과 프러스텀 충돌 판정
					{
						continue;
					}
				}

				// 오브젝트 그리기
				auto& perObject = mBasic32->GetPerObject();
				perObject.World = object->World.Transpose();
				perObject.WorldInvTranspose = MathHelper::InverseTranspose(object->World).Transpose();
				perObject.WorldViewProj = (object->World * mCam.GetViewProj()).Transpose();

				mBasic32->UpdateSubresource(md3dContext);

				md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);
				++renderObject;
			}

			std::wostringstream outs;
			outs.precision(6);
			outs << L"renderObject" << L"    " << renderObject <<
				L"total Object" << mObjects.size();
			mTitle = outs.str();
		}
		else
		{
			std::set<Object*> findObjects;

			Matrix view = mCam.GetView();
			BoundingFrustum Frustum(mCam.GetProj());

			BoundingFrustum worldFrustum;
			Frustum.Transform(worldFrustum, view.Invert());
			mOctree.FindIntersectObject(worldFrustum, &findObjects, mSkullBoundingBox);

			for (const Object* object : findObjects)
			{
				auto& perObject = mBasic32->GetPerObject();
				perObject.World = object->World.Transpose();
				perObject.WorldInvTranspose = MathHelper::InverseTranspose(object->World).Transpose();
				perObject.WorldViewProj = (object->World * mCam.GetViewProj()).Transpose();

				mBasic32->UpdateSubresource(md3dContext);

				md3dContext->DrawIndexed(mSkullIndexCount, 0, 0);
			}

			std::wostringstream outs;
			outs.precision(6);
			outs << L"renderObject" << L"    " << findObjects.size() <<
				L"total Object" << mObjects.size();
			mTitle = outs.str();

			md3dContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);
			md3dContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
			md3dContext->RSSetState(RenderStates::WireFrameRS);
			mOctree.DebugRender(md3dContext, mBasic32, mCam.GetViewProj());
			md3dContext->RSSetState(0);
		}

		mSwapChain->Present(0, 0);
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
			float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.RotatePitch(dy);
			mCam.RotateAxis({ 0, 1, 0 }, dx);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

	void D3DSample::initBasic32()
	{
		mBasic32 = new Basic32(md3dDevice, L"Basic.hlsl");

		auto& perFrame = mBasic32->GetPerFrame();
		perFrame.bUseLight = true;
		perFrame.bUseFog = true;
		perFrame.FogStart = 1;
		perFrame.FogRange = 1000;
		perFrame.FogColor = Silver;
		perFrame.bUseTexutre = false;
		perFrame.LigthCount = 3;
		perFrame.DirLights[0].Ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
		perFrame.DirLights[0].Diffuse = { 0.5f, 0.5f, 0.5f, 1.0f };
		perFrame.DirLights[0].Specular = { 0.5f, 0.5f, 0.5f, 1.0f };
		perFrame.DirLights[0].Direction = { 0.577f, -0.57735f, 0.57735f };
		perFrame.DirLights[1].Ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
		perFrame.DirLights[1].Diffuse = { 0.20f, 0.20f, 0.20f, 1.0f };
		perFrame.DirLights[1].Specular = { 0.25f, 0.25f, 0.25f, 1.0f };
		perFrame.DirLights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
		perFrame.DirLights[2].Ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
		perFrame.DirLights[2].Diffuse = { 0.2f, 0.2f, 0.2f, 1.0f };
		perFrame.DirLights[2].Specular = { 0.0f, 0.0f, 0.0f, 1.0f };
		perFrame.DirLights[2].Direction = { 0.0f, -0.707f, -0.707f };

		auto& perObject = mBasic32->GetPerObject();
		perObject.Material.Ambient = { 0.4f, 0.4f, 0.4f, 1.0f };
		perObject.Material.Diffuse = { 0.8f, 0.8f, 0.8f, 1.0f };
		perObject.Material.Specular = { 0.8f, 0.8f, 0.8f, 16.0f };

		mBasic32->UpdateSubresource(md3dContext);
	}
	void D3DSample::buildSkullGeometry()
	{
		// 파일에서 데이터 로딩
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

		XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

		XMVECTOR vMin = XMLoadFloat3(&vMinf3);
		XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
		std::vector<Basic32::Vertex> vertices(vcount);
		for (UINT i = 0; i < vcount; ++i)
		{
			fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
			fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

			XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

			vMin = XMVectorMin(vMin, P);
			vMax = XMVectorMax(vMax, P);
		}

		// AABB 바운딩 볼륨 생성
		XMStoreFloat3(&mSkullBoundingBox.Center, 0.5f * (vMin + vMax));
		XMStoreFloat3(&mSkullBoundingBox.Extents, 0.5f * (vMax - vMin));

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

		// 버텍스 버퍼 생성
		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32::Vertex) * vcount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mSkullVB));

		// 인덱스 버퍼 생성
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