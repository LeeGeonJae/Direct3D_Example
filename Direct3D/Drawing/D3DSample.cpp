#include <cassert>
#include <algorithm>
#include <fstream>

#include "D3DSample.h"
#include "D3DUtil.h"
#include "MathHelper.h"
#include "GeometryGenerator.h"

namespace drawing
{
	D3DSample::D3DSample(HINSTANCE hInstance, UINT width, UINT height, std::wstring name)
		: D3DProcessor(hInstance, width, height, name)
		, mBoxVB(nullptr)
		, mBoxIB(nullptr)
		, mPerObjectCB(nullptr)
		, mVertexShader(nullptr)
		, mVertexShaderBuffer(nullptr)
		, mPixelShader(nullptr)
		, mInputLayout(nullptr)
		, mWorld(DirectX::SimpleMath::Matrix::Identity)
		, mView(DirectX::SimpleMath::Matrix::Identity)
		, mProj(DirectX::SimpleMath::Matrix::Identity)
		, mTheta(1.5f * DirectX::XM_PI)
		, mPhi(0.25f * DirectX::XM_PI)
		, mRadius(200.f)
		, mLastMousePos({ 0, 0 })
	{
	}

	D3DSample::~D3DSample()
	{
		ReleaseCOM(mPerObjectCB);

		ReleaseCOM(mBoxVB);
		ReleaseCOM(mBoxIB);

		ReleaseCOM(mGridVB);
		ReleaseCOM(mGridIB);

		ReleaseCOM(mShapeVB);
		ReleaseCOM(mShapeIB);

		ReleaseCOM(mSkullVB);
		ReleaseCOM(mSkullIB);

		ReleaseCOM(mWavesVB);
		ReleaseCOM(mWavesIB);

		ReleaseCOM(mVertexShader);
		ReleaseCOM(mVertexShaderBuffer);
		ReleaseCOM(mPixelShader);
		ReleaseCOM(mInputLayout);
	}

	bool D3DSample::Init()
	{
		if (!D3DProcessor::Init())
		{
			return false;
		}

		buildGeometryBuffers();
		buildShader();
		buildVertexLayout();

		return true;
	}

	void D3DSample::OnResize()
	{
		D3DProcessor::OnResize();

		mProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, GetAspectRatio(), 1.f, 1000.f);
	}

	void D3DSample::Update(float deltaTime)
	{
		float x = mRadius * sinf(mPhi) * cosf(mTheta);
		float z = mRadius * sinf(mPhi) * sinf(mTheta);
		float y = mRadius * cosf(mPhi);

		DirectX::SimpleMath::Vector4 pos = { x, y, z, 1.f };
		DirectX::SimpleMath::Vector4 target = { 0.f, 0.f, 0.f, 0.f };
		DirectX::SimpleMath::Vector4 up = { 0.f, 1.f, 0.f, 0.f };

		mView = DirectX::XMMatrixLookAtLH(pos, target, up);


		static float t_base = 0.0f;
		if ((mTimer.GetTotalTime() - t_base) >= 0.25f)
		{
			t_base += 0.25f;

			DWORD i = 5 + rand() % 190;
			DWORD j = 5 + rand() % 190;

			float r = MathHelper::RandF(1.f, 2.f);

			mWaves.Disturb(i, j, r);
		}

		mWaves.Update(deltaTime);

		D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(md3dContext->Map(mWavesVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

		Vertex* vertices = reinterpret_cast<Vertex*>(mappedData.pData);
		for (UINT i = 0; i < mWaves.GetVertexCount(); ++i)
		{
			vertices[i].Position = mWaves[i];
			vertices[i].Color = common::Silver;
		}

		md3dContext->Unmap(mWavesVB, 0);
	}

	void D3DSample::Render()
	{
		assert(md3dContext);
		assert(mSwapChain);

		float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };

		md3dContext->ClearRenderTargetView(mRenderTargetView, color);
		md3dContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		md3dContext->IASetInputLayout(mInputLayout);
		md3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		md3dContext->VSSetShader(mVertexShader, NULL, 0);
		md3dContext->VSSetConstantBuffers(0, 1, &mPerObjectCB);

		md3dContext->PSSetShader(mPixelShader, NULL, 0);

		drawObject(mSkullVB, mSkullIB, mSkullIndexCount);
		drawObject(mWavesVB, mWavesIB, 3 * mWaves.GetTriangleCount());

		for (const std::pair<eShapeType, DirectX::SimpleMath::Matrix>& pair : mWorldMatrixs)
		{
			drawShape(pair.first, pair.second);
		}

		mSwapChain->Present(0, 0);
	}

	void D3DSample::drawObject(ID3D11Buffer* vertexBuffer, ID3D11Buffer* indexBuffer, UINT indexSize, const DirectX::SimpleMath::Matrix& worldMatrix)
	{
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		md3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		md3dContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		mCBPerObject.WVPMat = worldMatrix * mView * mProj;
		mCBPerObject.WVPMat = mCBPerObject.WVPMat.Transpose();

		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);

		md3dContext->DrawIndexed(indexSize, 0, 0);
	}

	void D3DSample::drawShape(eShapeType shapeType, const DirectX::SimpleMath::Matrix& worldMatrix)
	{
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		md3dContext->IASetVertexBuffers(0, 1, &mShapeVB, &stride, &offset);
		md3dContext->IASetIndexBuffer(mShapeIB, DXGI_FORMAT_R32_UINT, 0);

		mCBPerObject.WVPMat = worldMatrix * mView * mProj;
		mCBPerObject.WVPMat = mCBPerObject.WVPMat.Transpose();

		md3dContext->UpdateSubresource(mPerObjectCB, 0, NULL, &mCBPerObject, 0, 0);

		unsigned int shapeIndex = static_cast<unsigned int>(shapeType);
		md3dContext->DrawIndexed(mIndexCounts[shapeIndex], mIndexOffsets[shapeIndex], mVertexOffsets[shapeIndex]);
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
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

			// Update angles based on input to orbit camera around box.
			mTheta += dx;
			mPhi += dy;

			// Restrict the angle mPhi.
			mPhi = common::MathHelper::Clamp(mPhi, 0.1f, DirectX::XM_PI - 0.1f);
		}
		else if ((btnState & MK_RBUTTON) != 0)
		{
			// Make each pixel correspond to 0.005 unit in the scene.
			float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
			float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

			// Update the camera radius based on input.
			mRadius += dx - dy;

			// Restrict the radius.
			mRadius = common::MathHelper::Clamp(mRadius, 3.0f, 200.f);
		}

		mLastMousePos.x = x;
		mLastMousePos.y = y;
	}

	float D3DSample::getHeight(float x, float z) const
	{
		return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
	}

	void D3DSample::buildGeometryBuffers()
	{
		buildBox();
		buildGrid();
		buildShape();
		buildSkull();
		buildWaves();
	}

	void D3DSample::buildBox()
	{
		Vertex vertices[] =
		{
			{ {-1.0f, -1.0f, -1.0f }, common::White   },
			{ {-1.0f, +1.0f, -1.0f }, common::Black   },
			{ {+1.0f, +1.0f, -1.0f }, common::Red     },
			{ {+1.0f, -1.0f, -1.0f }, common::Green   },
			{ {-1.0f, -1.0f, +1.0f }, common::Blue    },
			{ {-1.0f, +1.0f, +1.0f }, common::Yellow  },
			{ {+1.0f, +1.0f, +1.0f }, common::Cyan    },
			{ {+1.0f, -1.0f, +1.0f }, common::Magenta }
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

	void D3DSample::buildGrid()
	{
		GeometryGenerator::MeshData grid;
		GeometryGenerator::CreateGrid(160.f, 160.f, 50, 50, &grid);

		mGridIndexCount = grid.Indices.size();

		std::vector<Vertex> vertices(grid.Vertices.size());
		for (size_t i = 0; i < grid.Vertices.size(); ++i)
		{
			DirectX::SimpleMath::Vector3 p = grid.Vertices[i].Position;

			p.y = getHeight(p.x, p.z);

			vertices[i].Position = p;

			// Color the vertex based on its height.
			if (p.y < -10.0f)
			{
				// Sandy beach color.
				vertices[i].Color = DirectX::SimpleMath::Vector4(1.0f, 0.96f, 0.62f, 1.0f);
			}
			else if (p.y < 5.0f)
			{
				// Light yellow-green.
				vertices[i].Color = DirectX::SimpleMath::Vector4(0.48f, 0.77f, 0.46f, 1.0f);
			}
			else if (p.y < 12.0f)
			{
				// Dark yellow-green.
				vertices[i].Color = DirectX::SimpleMath::Vector4(0.1f, 0.48f, 0.19f, 1.0f);
			}
			else if (p.y < 20.0f)
			{
				// Dark brown.
				vertices[i].Color = DirectX::SimpleMath::Vector4(0.45f, 0.39f, 0.34f, 1.0f);
			}
			else
			{
				// White snow.
				vertices[i].Color = DirectX::SimpleMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertex) * grid.Vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mGridVB));

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * mGridIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &grid.Indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mGridIB));
	}

	void D3DSample::buildShape()
	{
		GeometryGenerator::MeshData box;
		GeometryGenerator::MeshData grid;
		GeometryGenerator::MeshData sphere;
		GeometryGenerator::MeshData cylinder;

		GeometryGenerator::CreateBox(1.f, 1.f, 1.f, &box);
		GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40, &grid);
		GeometryGenerator::CreateSphere(0.5f, 20, 20, &sphere);
		//geoGen.CreateGeosphere(0.5f, 2, sphere);
		GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, &cylinder);

		mVertexOffsets.push_back(0);
		mVertexOffsets.push_back(box.Vertices.size());
		mVertexOffsets.push_back(mVertexOffsets.back() + grid.Vertices.size());
		mVertexOffsets.push_back(mVertexOffsets.back() + sphere.Vertices.size());

		mIndexCounts.push_back(box.Indices.size());
		mIndexCounts.push_back(grid.Indices.size());
		mIndexCounts.push_back(sphere.Indices.size());
		mIndexCounts.push_back(cylinder.Indices.size());

		mIndexOffsets.push_back(0);
		mIndexOffsets.push_back(box.Indices.size());
		mIndexOffsets.push_back(mIndexOffsets.back() + grid.Indices.size());
		mIndexOffsets.push_back(mIndexOffsets.back() + sphere.Indices.size());

		UINT totalVertexCount =
			box.Vertices.size() +
			grid.Vertices.size() +
			sphere.Vertices.size() +
			cylinder.Vertices.size();

		UINT totalIndexCount =
			box.Indices.size() +
			grid.Indices.size() +
			sphere.Indices.size() +
			cylinder.Indices.size();

		std::vector<Vertex> vertices;
		vertices.reserve(totalVertexCount);

		auto pushBack = [&vertices](const GeometryGenerator::MeshData& meshData)
			{
				for (const GeometryGenerator::Vertex& geoVertex : meshData.Vertices)
				{
					Vertex vertex;
					vertex.Position = geoVertex.Position;
					vertex.Color = common::Black;

					vertices.push_back(vertex);
				}
			};

		pushBack(box);
		pushBack(grid);
		pushBack(sphere);
		pushBack(cylinder);

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertex) * totalVertexCount;
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = &vertices[0];
		HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mShapeVB));

		std::vector<UINT> indices;
		indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
		indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
		indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
		indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mShapeIB));

		using namespace DirectX::SimpleMath;
		Matrix worldMat = Matrix::CreateScale(2.f, 1.f, 2.f) * Matrix::CreateTranslation(0.f, 0.5f, 0.f);
		mWorldMatrixs.push_back({ eShapeType::Box, worldMat });
		worldMat = Matrix::CreateScale(2.f, 2.f, 2.f) * Matrix::CreateTranslation(0.f, 2.f, 0.f);
		mWorldMatrixs.push_back({ eShapeType::Sphere, worldMat });

		for (int i = 0; i < 5; ++i)
		{
			mWorldMatrixs.push_back({ eShapeType::Cyliner, Matrix::CreateTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f) });
			mWorldMatrixs.push_back({ eShapeType::Cyliner, Matrix::CreateTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f) });

			mWorldMatrixs.push_back({ eShapeType::Sphere, Matrix::CreateTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f) });
			mWorldMatrixs.push_back({ eShapeType::Sphere, Matrix::CreateTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f) });
		}
	}

	void D3DSample::buildSkull()
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

		float nx, ny, nz;

		std::vector<Vertex> vertices(vcount);
		for (UINT i = 0; i < vcount; ++i)
		{
			fin >> vertices[i].Position.x >> vertices[i].Position.y >> vertices[i].Position.z;

			vertices[i].Color = common::Black;

			// Normal not used in this demo.
			fin >> nx >> ny >> nz;
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

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(Vertex) * vcount;
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

	void D3DSample::buildWaves()
	{
		mWaves.Init(200, 200, 0.8f, 0.03f, 3.25f, 0.4f);

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_DYNAMIC; // 동적 변경
		vbd.ByteWidth = sizeof(Vertex) * mWaves.GetVertexCount();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // 쓰기 접근
		vbd.MiscFlags = 0;
		HR(md3dDevice->CreateBuffer(&vbd, 0, &mWavesVB));

		std::vector<UINT> indices;
		indices.reserve(3 * mWaves.GetTriangleCount());

		UINT m = mWaves.GetRowCount();
		UINT n = mWaves.GetColumnCount();

		for (UINT i = 0; i < m - 1; ++i)
		{
			for (UINT j = 0; j < n - 1; ++j)
			{
				indices.push_back(i * m + j);
				indices.push_back(i * m + (j + 1));
				indices.push_back((i + 1) * m + j);

				indices.push_back((i + 1) * m + j);
				indices.push_back(i * m + (j + 1));
				indices.push_back((i + 1) * m + (j + 1));
			}
		}

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * indices.size();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = &indices[0];
		HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mWavesIB));
	}

	void D3DSample::buildShader()
	{
		HR(CompileShaderFromFile(L"../Resource/Shader/DrawingVS.hlsl", "main", "vs_5_0", &mVertexShaderBuffer));

		HR(md3dDevice->CreateVertexShader(
			mVertexShaderBuffer->GetBufferPointer(),
			mVertexShaderBuffer->GetBufferSize(),
			NULL,
			&mVertexShader));

		ID3DBlob* pixelShaderBuffer = nullptr;
		HR(CompileShaderFromFile(L"../Resource/Shader/DrawingPS.hlsl", "main", "ps_5_0", &pixelShaderBuffer));

		HR(md3dDevice->CreatePixelShader(
			pixelShaderBuffer->GetBufferPointer(),
			pixelShaderBuffer->GetBufferSize(),
			NULL,
			&mPixelShader));

		ReleaseCOM(pixelShaderBuffer);
	}

	void D3DSample::buildVertexLayout()
	{
		D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		HR(md3dDevice->CreateInputLayout(
			vertexDesc,
			ARRAYSIZE(vertexDesc),
			mVertexShaderBuffer->GetBufferPointer(),
			mVertexShaderBuffer->GetBufferSize(),
			&mInputLayout));
	}
}