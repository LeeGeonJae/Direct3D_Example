#include <cassert>
#include <fstream>

#include "D3DSample.h"
#include "RenderStates.h"

namespace picking
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mMeshIndexCount(0)
		, mPickedTriangle(-1)
	{
		mTitle = L"Picking Demo";

		mLastMousePos.x = 0;
		mLastMousePos.y = 0;

		mCam.SetPosition(0.0f, 2.0f, -50.0f);

		Matrix MeshScale = Matrix::CreateScale(0.5f, 0.5f, 0.5f);
		Matrix MeshOffset = Matrix::CreateTranslation(0.0f, 1.0f, 0);
		mMeshWorld = MeshScale * MeshOffset;

		mDirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		mDirLights[0].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

		mDirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
		mDirLights[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
		mDirLights[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

		mDirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
		mDirLights[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mDirLights[2].Direction = XMFLOAT3(0.0f, -0.707f, -0.707f);

		mMeshMat.Ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
		mMeshMat.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
		mMeshMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

		mPickedTriangleMat.Ambient = XMFLOAT4(0.0f, 0.8f, 0.4f, 1.0f);
		mPickedTriangleMat.Diffuse = XMFLOAT4(0.0f, 0.8f, 0.4f, 1.0f);
		mPickedTriangleMat.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);
	}
	D3DSample::~D3DSample()
	{
		ReleaseCOM(mMeshVB);
		ReleaseCOM(mMeshIB);

		RenderStates::Destroy();
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		buildShader();
		buildInputLayout();
		buildConstantBuffer();
		buildBox();

		buildMeshGeometryBuffers();
		RenderStates::Init(md3dDevice);

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

		mCam.UpdateViewMatrix();
	}

	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		md3dContext->ClearRenderTargetView(mRenderTargetView, Silver);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		// if (GetAsyncKeyState('1') & 0x8000)
		// {
		// 	md3dContext->RSSetState(RenderStates::WireFrameRS);
		// }

		// 상수버퍼 갱신
		Matrix view = mCam.GetView();
		Matrix proj = mCam.GetProj();
		Matrix viewProj = mCam.GetViewProj();

		memcpy(mCBPerFrame.DirLights, mDirLights, sizeof(mCBPerFrame.DirLights));
		mCBPerFrame.EyePosW = mCam.GetPosition();
		mCBPerFrame.LigthCount = 3;
		mCBPerFrame.FogColor = Silver;
		mCBPerFrame.FogStart = 10000.f;
		mCBPerFrame.FogRange = 20000.f;
		md3dContext->UpdateSubresource(mPerFrameCB, 0, 0, &mCBPerFrame, 0, 0);

		Matrix world = mMeshWorld;
		Matrix worldInvTranspose = MathHelper::InverseTranspose(world);
		Matrix worldViewProj = world * view * proj;

		mCBPerObject.World = world.Transpose();
		mCBPerObject.WorldInvTranspose = worldInvTranspose.Transpose();
		mCBPerObject.WorldViewProj = worldViewProj.Transpose();
		mCBPerObject.Tex = Matrix::Identity;
		mCBPerObject.Material = mMeshMat;
		md3dContext->UpdateSubresource(mPerObjectCB, 0, 0, &mCBPerObject, 0, 0);

		UINT stride = sizeof(Basic32);
		UINT offset = 0;

		md3dContext->IASetInputLayout(mBasic32); 
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		md3dContext->IASetVertexBuffers(0, 1, &mMeshVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mMeshIB, DXGI_FORMAT_R32_UINT, 0);

		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->VSSetConstantBuffers(1, 1, &mPerFrameCB);
		md3dContext->VSSetShader(mVertexShader, 0, 0);

		md3dContext->PSSetConstantBuffers(0, 1, &mPerObjectCB);
		md3dContext->PSSetConstantBuffers(1, 1, &mPerFrameCB);
		md3dContext->PSSetShader(mPixelShader, 0, 0);

		md3dContext->DrawIndexed(mMeshIndexCount, 0, 0);
		md3dContext->RSSetState(0);

		if (mPickedTriangle != -1)
		{
			md3dContext->OMSetDepthStencilState(RenderStates::LessEqualDSS, 0);

			mCBPerObject.Material = mPickedTriangleMat;
			md3dContext->UpdateSubresource(mPerObjectCB, 0, 0, &mCBPerObject, 0, 0);

			md3dContext->DrawIndexed(3, 3 * mPickedTriangle, 0);
			md3dContext->OMSetDepthStencilState(0, 0);
		}

		mSwapChain->Present(0, 0);
	}

	void D3DSample::OnMouseDown(WPARAM btnState, int x, int y)
	{
		if ((btnState & MK_LBUTTON) != 0)
		{
			mLastMousePos.x = x;
			mLastMousePos.y = y;

			SetCapture(mhWnd);
		}
		else if ((btnState & MK_RBUTTON) != 0)
		{
			pick(x, y);
		}
	}
	void D3DSample::OnMouseUp(WPARAM btnState, int x, int y)
	{
		ReleaseCapture();
	}
	void D3DSample::OnMouseMove(WPARAM btnState, int x, int y)
	{
		pick(x, y);

		if ((btnState & MK_LBUTTON) != 0)
		{
			float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			mCam.RotatePitch(dy);
			mCam.RotateAxis({ 0,1,0 }, dx);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

	void D3DSample::buildConstantBuffer()
	{
		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(CBPerObject) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(CBPerObject);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerObjectCB));

		static_assert(sizeof(CBPerFrame) % 16 == 0, "must be align");
		cbd.ByteWidth = sizeof(CBPerFrame);
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerFrameCB));
	}
	void D3DSample::buildShader()
	{
		HR(D3DHelper::CompileShaderFromFile(L"Basic.hlsl", "VS", "vs_5_0", &mVertexShaderBuffer));

		HR(md3dDevice->CreateVertexShader(
			mVertexShaderBuffer->GetBufferPointer(),
			mVertexShaderBuffer->GetBufferSize(),
			NULL,
			&mVertexShader));

		ID3DBlob* pixelShaderBuffer = nullptr;
		HR(D3DHelper::CompileShaderFromFile(L"Basic.hlsl", "PS", "ps_5_0", &pixelShaderBuffer));

		HR(md3dDevice->CreatePixelShader(
			pixelShaderBuffer->GetBufferPointer(),
			pixelShaderBuffer->GetBufferSize(),
			NULL,
			&mPixelShader));

		ReleaseCOM(pixelShaderBuffer);
	}

	void D3DSample::buildBox()
	{
		Basic32 vertices[] =
		{
			{ {-1.0f, -1.0f, -1.0f }, },
			{ {-1.0f, +1.0f, -1.0f }, },
			{ {+1.0f, +1.0f, -1.0f }, },
			{ {+1.0f, -1.0f, -1.0f }, },
			{ {-1.0f, -1.0f, +1.0f }, },
			{ {-1.0f, +1.0f, +1.0f }, },
			{ {+1.0f, +1.0f, +1.0f }, },
			{ {+1.0f, -1.0f, +1.0f }, }
		};

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE; // 변경되지 않는 버퍼
		vbd.ByteWidth = sizeof(vertices); //  sizeof(Vertex) * 8;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0; // CPU 접근 read, write할지, 0은 암것도 안한다.
		vbd.MiscFlags = 0; // 기타 플래그
		vbd.StructureByteStride = 0; // 구조화된 버퍼?
		D3D11_SUBRESOURCE_DATA vinitData; // 초기값 설정을 위한 구조체
		vinitData.pSysMem = vertices;

		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

		UINT indices[] = {
			// front face
			0, 1, 2,
			0, 2, 3,

			// back face
			4, 6, 5,
			4, 7, 6,

			// left face
			4, 5, 1,
			4, 1, 0,

			// right face
			3, 2, 6,
			3, 6, 7,

			// top face
			1, 5, 6,
			1, 6, 2,

			// bottom face
			4, 0, 3,
			4, 3, 7
		};

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(indices);
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		ibd.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = indices;
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
		mBoxIndexCount = sizeof(indices) / sizeof(indices[0]);

		D3D11_BUFFER_DESC cbd;
		cbd.Usage = D3D11_USAGE_DEFAULT;
		static_assert(sizeof(CBPerObject) % 16 == 0, "Struct : ConstantBufferMat align error");
		cbd.ByteWidth = sizeof(CBPerObject);
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = 0;
		cbd.MiscFlags = 0;
		cbd.StructureByteStride = 0;
		HR(md3dDevice->CreateBuffer(&cbd, NULL, &mPerObjectCB));
	}

	void D3DSample::buildInputLayout()
	{
		const D3D11_INPUT_ELEMENT_DESC basic32[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		HR(md3dDevice->CreateInputLayout(
			basic32,
			ARRAYSIZE(basic32),
			mVertexShaderBuffer->GetBufferPointer(),
			mVertexShaderBuffer->GetBufferSize(),
			&mBasic32
		));
	}

	void D3DSample::buildMeshGeometryBuffers()
	{
		std::ifstream fin("../Resource/Models/car.txt");

		if (!fin)
		{
			MessageBox(0, L"Models/car.txt not found.", 0, 0);
			return;
		}

		UINT vcount = 0;
		UINT tcount = 0;
		std::string ignore;

		fin >> ignore >> vcount;
		fin >> ignore >> tcount;
		fin >> ignore >> ignore >> ignore >> ignore;

		Vector3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		Vector3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

		Vector3 vMin = vMinf3;
		Vector3 vMax = vMaxf3;
		mMeshVertices.resize(vcount);
		for (UINT i = 0; i < vcount; ++i)
		{
			fin >> mMeshVertices[i].Pos.x >> mMeshVertices[i].Pos.y >> mMeshVertices[i].Pos.z;
			fin >> mMeshVertices[i].Normal.x >> mMeshVertices[i].Normal.y >> mMeshVertices[i].Normal.z;

			Vector3 P = mMeshVertices[i].Pos;

			vMin = XMVectorMin(vMin, P);
			vMax = XMVectorMax(vMax, P);
		}

		XMStoreFloat3(&mMeshBox.Center, 0.5f * (vMin + vMax));
		XMStoreFloat3(&mMeshBox.Extents, 0.5f * (vMax - vMin));

		fin >> ignore;
		fin >> ignore;
		fin >> ignore;

		mMeshIndexCount = 3 * tcount;
		mMeshIndices.resize(mMeshIndexCount);
		for (UINT i = 0; i < tcount; ++i)
		{
			fin >> mMeshIndices[i * 3 + 0] >> mMeshIndices[i * 3 + 1] >> mMeshIndices[i * 3 + 2];
		}

		fin.close();

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Basic32) * vcount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &mMeshVertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mMeshVB));

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * mMeshIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &mMeshIndices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mMeshIB));
	}

	void D3DSample::pick(int sx, int sy)
	{
		Matrix P = mCam.GetProj(); // 투영 행렬을 읽어온다.

		float vx = (+2.0f * sx / mWidth - 1.0f) *  P(0, 0) / P(1, 1); // 화면공간 -> 투영 공간 x
		float vy = (-2.0f * sy / mHeight + 1.0f);// / P(1, 1); // 화면공간 -> 투영 공간 y
		float vz = 1 / P(1, 1); // cot(@/2)

		Vector3 rayOrigin = { 0.0f, 0.0f, 0.0f }; // 투영공간의 원점
		Vector3 rayDir = { vx, vy, vz }; // 투영공간의 방향 벡터
		Ray ray(rayOrigin, rayDir); // 투영공간에서 선택 반직선

		const Matrix WV = mMeshWorld * mCam.GetView();
		Matrix InvWV;
		WV.Invert(InvWV); // View -> Local Matrix

		// 로컬공간에서 선택 반직선
		ray.position = Vector3::Transform(ray.position, InvWV);
		ray.direction = Vector3::TransformNormal(ray.direction, InvWV);
		ray.direction.Normalize();

		// 최소 거리 삼각형 찾기
		mPickedTriangle = -1;
		float tmin = 0.0f;
		if (ray.Intersects(mMeshBox, tmin)) // 바운딩 볼륨을 먼저 검사한다.
		{
			tmin = MathHelper::Infinity;
			for (UINT i = 0; i < mMeshIndices.size() / 3; ++i)
			{
				UINT i0 = mMeshIndices[i * 3 + 0];
				UINT i1 = mMeshIndices[i * 3 + 1];
				UINT i2 = mMeshIndices[i * 3 + 2];

				Vector3 v0 = mMeshVertices[i0].Pos;
				Vector3 v1 = mMeshVertices[i1].Pos;
				Vector3 v2 = mMeshVertices[i2].Pos;

				float t = 0.0f;
				if (ray.Intersects(v0, v1, v2, t))
				{
					if (t < tmin)
					{
						tmin = t;
						mPickedTriangle = i;
					}
				}
			}
		}
	}
}